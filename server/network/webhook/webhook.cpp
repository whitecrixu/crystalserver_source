////////////////////////////////////////////////////////////////////////
// Crystal Server - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////

#include "server/network/webhook/webhook.hpp"

#include "config/configmanager.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "utils/tools.hpp"
#include "lib/di/container.hpp"

Webhook::Webhook(ThreadPool &threadPool) :
	threadPool(threadPool) {
	if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
		g_logger().error("Failed to init curl, no webhook messages may be sent");
		return;
	}

	headers = curl_slist_append(headers, "content-type: application/json");
	headers = curl_slist_append(headers, "accept: application/json");

	if (headers == nullptr) {
		g_logger().error("Failed to init curl, appending request headers failed");
		return;
	}

	run();
}

Webhook &Webhook::getInstance() {
	return inject<Webhook>();
}

void Webhook::run() {
	threadPool.detach_task([this] { sendWebhook(); });
	g_dispatcher().scheduleEvent(
		g_configManager().getNumber(DISCORD_WEBHOOK_DELAY_MS), [this] { run(); }, "Webhook::run"
	);
}

void Webhook::sendPayload(const std::string &payload, const std::string &url) {
	std::scoped_lock lock { taskLock };
	webhooks.emplace_back(std::make_shared<WebhookTask>(payload, url));
}

void Webhook::sendMessage(const std::string &title, const std::string &message, int color, std::string url, bool embed) {
	if (url.empty()) {
		url = g_configManager().getString(DISCORD_WEBHOOK_URL);
	}

	if (url.empty() || title.empty() || message.empty()) {
		return;
	}

	sendPayload(getPayload(title, message, color, embed), url);
}

void Webhook::sendMessage(const std::string &message, std::string url) {
	if (url.empty()) {
		url = g_configManager().getString(DISCORD_WEBHOOK_URL);
	}

	if (url.empty() || message.empty()) {
		return;
	}

	sendPayload(getPayload("", message, -1, false), url);
}

int Webhook::sendRequest(const char* url, const char* payload, std::string* response_body) const {
	CURL* curl = curl_easy_init();
	if (!curl) {
		g_logger().error("Failed to send webhook message; curl_easy_init failed");
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Webhook::writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(response_body));
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "crystalserver (https://github.com/zimbadev/crystalserver)");

	const CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		g_logger().error("Failed to send webhook message with the error: {}", curl_easy_strerror(res));
		curl_easy_cleanup(curl);

		return -1;
	}

	int response_code;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_cleanup(curl);

	return response_code;
}

size_t Webhook::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	const size_t real_size = size * nmemb;
	auto* str = static_cast<std::string*>(userp);
	str->append(static_cast<char*>(contents), real_size);
	return real_size;
}

std::string Webhook::getPayload(const std::string &title, const std::string &message, int color, bool embed) const {
	const std::time_t now = getTimeNow();
	const std::string time_buf = formatDate(now);

	std::stringstream footer_text;
	footer_text
		<< g_configManager().getString(SERVER_NAME) << " | "
		<< time_buf;

	std::stringstream payload;
	if (embed) {
		payload << "{ \"embeds\": [{ ";
		payload << R"("title": ")" << title << "\", ";
		if (!message.empty()) {
			payload << R"("description": ")" << message << "\", ";
		}
		if (g_configManager().getBoolean(DISCORD_SEND_FOOTER)) {
			payload << R"("footer": { "text": ")" << footer_text.str() << "\" }, ";
		}
		if (color >= 0) {
			payload << "\"color\": " << color;
		}
		payload << " }] }";
	} else {
		payload << R"({ "content": ")" << (!message.empty() ? message : title) << "\" }";
	}

	return payload.str();
}

void Webhook::sendWebhook() {
	std::scoped_lock lock { taskLock };
	if (webhooks.empty()) {
		return;
	}

	const auto &task = webhooks.front();

	std::string response_body;
	auto response_code = sendRequest(task->url.c_str(), task->payload.c_str(), &response_body);

	if (response_code == -1) {
		return;
	}

	if (response_code == 429 || response_code == 504) {
		g_logger().warn("Webhook encountered error code {}, re-queueing task.", response_code);

		return;
	}

	if (response_code >= 300) {
		g_logger().error(
			"Failed to send webhook message, error code: {} response body: {} request body: {}",
			response_code,
			response_body,
			task->payload
		);

		return;
	}

	g_logger().debug("Webhook successfully sent to {}", task->url);

	// Removes the object after processing everything, avoiding memory usage after freeing
	webhooks.pop_front();
}
