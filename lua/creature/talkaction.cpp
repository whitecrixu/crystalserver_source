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

#include "lua/creature/talkaction.hpp"

#include "utils/tools.hpp"
#include "creatures/players/grouping/groups.hpp"
#include "creatures/players/player.hpp"
#include "lua/scripts/scripts.hpp"
#include "lib/di/container.hpp"

TalkActions::TalkActions() = default;
TalkActions::~TalkActions() = default;

TalkActions &TalkActions::getInstance() {
	return inject<TalkActions>();
}

void TalkActions::clear() {
	talkActions.clear();
}

bool TalkActions::registerLuaEvent(const TalkAction_ptr &talkAction) {
	auto [iterator, inserted] = talkActions.try_emplace(talkAction->getWords(), talkAction);
	return inserted;
}

bool TalkActions::checkWord(const std::shared_ptr<Player> &player, SpeakClasses type, const std::string &words, std::string_view word, const TalkAction_ptr &talkActionPtr) const {
	const auto spacePos = std::ranges::find_if(words.begin(), words.end(), ::isspace);
	const std::string firstWord = words.substr(0, spacePos - words.begin());

	// Check for exact equality from saying word and talkaction stored word
	if (firstWord != word) {
		return false;
	}

	const auto groupId = player->getGroup()->id;
	if (groupId < talkActionPtr->getGroupType()) {
		return false;
	}

	std::string param;
	const size_t wordPos = words.find(word);
	const size_t talkactionLength = word.length();
	if (wordPos != std::string::npos && wordPos + talkactionLength < words.length()) {
		param = words.substr(wordPos + talkactionLength);
		trim_left(param, ' ');
	}

	const std::string separator = talkActionPtr->getSeparator();
	if (separator != " ") {
		if (!param.empty()) {
			if (param != separator) {
				return false;
			} else {
				param.erase(param.begin());
			}
		}
	}

	return talkActionPtr->executeSay(player, words, param, type);
}

TalkActionResult_t TalkActions::checkPlayerCanSayTalkAction(const std::shared_ptr<Player> &player, SpeakClasses type, const std::string &words) const {
	for (const auto &[talkactionWords, talkActionPtr] : talkActions) {
		if (talkactionWords.find(',') != std::string::npos) {
			auto wordsList = split(talkactionWords);
			for (const auto &word : wordsList) {
				if (checkWord(player, type, words, word, talkActionPtr)) {
					return TALKACTION_BREAK;
				}
			}
		} else {
			if (checkWord(player, type, words, talkactionWords, talkActionPtr)) {
				return TALKACTION_BREAK;
			}
		}
	}
	return TALKACTION_CONTINUE;
}

LuaScriptInterface* TalkAction::getScriptInterface() const {
	return &g_scripts().getScriptInterface();
}

bool TalkAction::loadScriptId() {
	LuaScriptInterface &luaInterface = g_scripts().getScriptInterface();
	m_scriptId = luaInterface.getEvent();
	if (m_scriptId == -1) {
		g_logger().error("[MoveEvent::loadScriptId] Failed to load event. Script name: '{}', Module: '{}'", luaInterface.getLoadingScriptName(), luaInterface.getInterfaceName());
		return false;
	}

	return true;
}

int32_t TalkAction::getScriptId() const {
	return m_scriptId;
}

void TalkAction::setScriptId(int32_t newScriptId) {
	m_scriptId = newScriptId;
}

bool TalkAction::isLoadedScriptId() const {
	return m_scriptId != 0;
}

bool TalkAction::executeSay(const std::shared_ptr<Player> &player, const std::string &words, const std::string &param, SpeakClasses type) const {
	// onSay(player, words, param, type)
	if (!LuaScriptInterface::reserveScriptEnv()) {
		g_logger().error("[TalkAction::executeSay - Player {} words {}] "
		                 "Call stack overflow. Too many lua script calls being nested. Script name {}",
		                 player->getName(), getWords(), getScriptInterface()->getLoadingScriptName());
		return false;
	}

	ScriptEnvironment* scriptEnvironment = LuaScriptInterface::getScriptEnv();
	scriptEnvironment->setScriptId(getScriptId(), getScriptInterface());

	lua_State* L = getScriptInterface()->getLuaState();

	getScriptInterface()->pushFunction(getScriptId());

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushString(L, words);
	LuaScriptInterface::pushString(L, param);
	LuaScriptInterface::pushNumber(L, static_cast<lua_Number>(type));

	return getScriptInterface()->callFunction(4);
}

void TalkAction::setGroupType(uint8_t newGroupType) {
	m_groupType = newGroupType;
}

const uint8_t &TalkAction::getGroupType() const {
	return m_groupType;
}
