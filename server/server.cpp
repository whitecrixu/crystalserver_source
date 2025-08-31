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

#include "server/server.hpp"

#include "server/network/message/outputmessage.hpp"
#include "config/configmanager.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "creatures/players/management/ban.hpp"

ServiceManager::~ServiceManager() {
	try {
		stop();
	} catch (std::exception &exception) {
		g_logger().error("{} - Catch exception error: {}", __FUNCTION__, exception.what());
	}
}

void ServiceManager::die() {
	io_service.stop();
}

void ServiceManager::run() {
	if (running) {
		g_logger().error("ServiceManager is already running!", __FUNCTION__);
		return;
	}

	assert(!running);
	running = true;
	io_service.run();
}

void ServiceManager::stop() {
	if (!running) {
		return;
	}

	running = false;

	for (auto &servicePortIt : acceptors) {
		try {
			io_service.post([servicePort = servicePortIt.second] { servicePort->onStopServer(); });
		} catch (const std::system_error &e) {
			g_logger().warn("[ServiceManager::stop] - Network error: {}", e.what());
		}
	}

	acceptors.clear();

	death_timer.expires_from_now(std::chrono::seconds(3));
	death_timer.async_wait([this](const std::error_code &err) {
		die();
	});
}

ServicePort::~ServicePort() {
	close();
}

bool ServicePort::is_single_socket() const {
	return !services.empty() && services.front()->is_single_socket();
}

std::string ServicePort::get_protocol_names() const {
	if (services.empty()) {
		return {};
	}

	std::string str = services.front()->get_protocol_name();
	for (size_t i = 1; i < services.size(); ++i) {
		str.push_back(',');
		str.push_back(' ');
		str.append(services[i]->get_protocol_name());
	}
	return str;
}

void ServicePort::accept() {
	if (!acceptor) {
		return;
	}

	auto connection = ConnectionManager::getInstance().createConnection(io_service, shared_from_this());
	acceptor->async_accept(connection->getSocket(), [self = shared_from_this(), connection](const std::error_code &error) { self->onAccept(connection, error); });
}

void ServicePort::onAccept(const Connection_ptr &connection, const std::error_code &error) {
	if (!error) {
		if (services.empty()) {
			return;
		}

		const auto remote_ip = connection->getIP();
		if (remote_ip != 0 && inject<Ban>().acceptConnection(remote_ip)) {
			const Service_ptr service = services.front();
			if (service->is_single_socket()) {
				connection->accept(service->make_protocol(connection));
			} else {
				connection->acceptInternal();
			}
		} else {
			connection->close(FORCE_CLOSE);
		}

		accept();
	} else if (error != asio::error::operation_aborted) {
		if (!pendingStart) {
			close();
			pendingStart = true;
			g_dispatcher().scheduleEvent(
				15000, [self = shared_from_this(), serverPort = serverPort] { ServicePort::openAcceptor(std::weak_ptr<ServicePort>(self), serverPort); }, "ServicePort::openAcceptor"
			);
		}
	}
}

Protocol_ptr ServicePort::make_protocol(bool checksummed, NetworkMessage &msg, const Connection_ptr &connection) const {
	const uint8_t protocolID = msg.getByte();
	for (auto &service : services) {
		if (protocolID != service->get_protocol_identifier()) {
			continue;
		}

		if ((checksummed && service->is_checksummed()) || !service->is_checksummed()) {
			return service->make_protocol(connection);
		}
	}
	return nullptr;
}

void ServicePort::onStopServer() const {
	close();
}

void ServicePort::openAcceptor(const std::weak_ptr<ServicePort> &weak_service, uint16_t port) {
	if (const auto service = weak_service.lock()) {
		service->open(port);
	}
}

void ServicePort::open(uint16_t port) {
	close();

	serverPort = port;
	pendingStart = false;

	try {
		std::string ipString = g_configManager().getString(IP);
		asio::ip::address ipAddress;
		bool resolved = false;

		try {
			ipAddress = asio::ip::address_v4::from_string(ipString);
			resolved = true;
		} catch (const std::exception &) {
			try {
				ipAddress = asio::ip::address_v6::from_string(ipString);
				resolved = true;
			} catch (const std::exception &) {
				asio::ip::tcp::resolver resolver(io_service);
				asio::ip::tcp::resolver::results_type results = resolver.resolve(ipString, std::to_string(port));

				for (const auto &entry : results) {
					if (entry.endpoint().address().is_v6()) {
						ipAddress = entry.endpoint().address().to_v6();
						resolved = true;
						break;
					}
				}
				if (!resolved) {
					for (const auto &entry : results) {
						if (entry.endpoint().address().is_v4()) {
							ipAddress = entry.endpoint().address().to_v4();
							resolved = true;
							break;
						}
					}
				}
			}
		}

		if (!resolved) {
			throw std::runtime_error("Could not resolve any IP address for: " + ipString);
		}

		asio::ip::tcp::endpoint endpoint;
		if (g_configManager().getBoolean(BIND_ONLY_GLOBAL_ADDRESS)) {
			endpoint = asio::ip::tcp::endpoint(ipAddress, serverPort);
		} else {
			if (ipAddress.is_v6()) {
				endpoint = asio::ip::tcp::endpoint(asio::ip::address_v6::any(), serverPort);
			} else {
				endpoint = asio::ip::tcp::endpoint(asio::ip::address_v4::any(), serverPort);
			}
		}

		acceptor = std::make_unique<asio::ip::tcp::acceptor>(io_service, endpoint);
		acceptor->set_option(asio::ip::tcp::no_delay(true));

		accept();
	} catch (const std::exception &e) {
		g_logger().warn("[ServicePort::open] - Error: {}", e.what());

		pendingStart = true;
		g_dispatcher().scheduleEvent(
			15000,
			[self = shared_from_this(), port] {
				ServicePort::openAcceptor(std::weak_ptr<ServicePort>(self), port);
			},
			"ServicePort::openAcceptor"
		);
	}
}

void ServicePort::close() const {
	if (acceptor && acceptor->is_open()) {
		std::error_code error;
		acceptor->close(error);
	}
}

bool ServicePort::add_service(const Service_ptr &new_svc) {
	if (std::ranges::any_of(services, [](const Service_ptr &svc) { return svc->is_single_socket(); })) {
		return false;
	}

	services.emplace_back(new_svc);
	return true;
}
