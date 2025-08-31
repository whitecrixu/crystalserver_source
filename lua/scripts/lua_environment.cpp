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

#include "lua/scripts/lua_environment.hpp"

#include "declarations.hpp"
#include "lua/functions/lua_functions_loader.hpp"
#include "lua/scripts/script_environment.hpp"
#include "lua/global/lua_timer_event_descr.hpp"
#include "lib/di/container.hpp"

bool LuaEnvironment::shuttingDown = false;

LuaEnvironment &LuaEnvironment::getInstance() {
	return inject<LuaEnvironment>();
}

static const std::unique_ptr<AreaCombat> &AreaCombatNull {};

LuaEnvironment::LuaEnvironment() :
	LuaScriptInterface("Main Interface") { }

LuaEnvironment::~LuaEnvironment() {
	if (!testInterface) {
	}

	LuaEnvironment::shuttingDown = true;
	closeState();
}

lua_State* LuaEnvironment::getLuaState() {
	if (LuaEnvironment::isShuttingDown()) {
		return luaState;
	}

	if (luaState == nullptr) {
		initState();
	}

	return luaState;
}

bool LuaEnvironment::initState() {
	luaState = luaL_newstate();
	Lua::load(luaState);
	runningEventId = EVENT_ID_USER;

	return true;
}

bool LuaEnvironment::reInitState() {
	// TODO(lgrossi): get children, reload children
	closeState();
	return initState();
}

bool LuaEnvironment::closeState() {
	if (!luaState) {
		return false;
	}

	for (const auto &areaEntry : areaIdMap) {
		clearAreaObjects(areaEntry.first);
	}

	for (auto &timerEntry : timerEvents) {
		LuaTimerEventDesc timerEventDesc = std::move(timerEntry.second);
		for (const int32_t parameter : timerEventDesc.parameters) {
			luaL_unref(luaState, LUA_REGISTRYINDEX, parameter);
		}
		luaL_unref(luaState, LUA_REGISTRYINDEX, timerEventDesc.function);
	}

	areaIdMap.clear();
	timerEvents.clear();
	cacheFiles.clear();

	lua_close(luaState);
	luaState = nullptr;
	return true;
}

LuaScriptInterface* LuaEnvironment::getTestInterface() {
	if (!testInterface) {
		testInterface = new LuaScriptInterface("Test Interface");
		testInterface->initState();
	}
	return testInterface;
}

const std::unique_ptr<AreaCombat> &LuaEnvironment::getAreaObject(uint32_t id) const {
	const auto it = areaMap.find(id);
	if (it == areaMap.end()) {
		return AreaCombatNull;
	}
	return it->second;
}

uint32_t LuaEnvironment::createAreaObject(LuaScriptInterface* interface) {
	areaMap[++lastAreaId] = std::make_unique<AreaCombat>();
	areaIdMap[interface].push_back(lastAreaId);
	return lastAreaId;
}

void LuaEnvironment::clearAreaObjects(LuaScriptInterface* interface) {
	const auto it = areaIdMap.find(interface);
	if (it == areaIdMap.end()) {
		return;
	}

	for (uint32_t id : it->second) {
		auto itt = areaMap.find(id);
		if (itt != areaMap.end()) {
			areaMap.erase(itt);
		}
	}
	it->second.clear();
}

void LuaEnvironment::executeTimerEvent(uint32_t eventIndex) {
	const auto it = timerEvents.find(eventIndex);
	if (it == timerEvents.end()) {
		return;
	}

	LuaTimerEventDesc timerEventDesc = std::move(it->second);
	timerEvents.erase(it);

	// push function
	lua_rawgeti(luaState, LUA_REGISTRYINDEX, timerEventDesc.function);

	// push parameters
	for (const auto parameter : std::views::reverse(timerEventDesc.parameters)) {
		lua_rawgeti(luaState, LUA_REGISTRYINDEX, parameter);
	}

	// call the function
	if (reserveScriptEnv()) {
		ScriptEnvironment* env = getScriptEnv();
		env->setTimerEvent();
		env->setScriptId(timerEventDesc.scriptId, this);
		callFunction(timerEventDesc.parameters.size());
	} else {
		g_logger().error("[LuaEnvironment::executeTimerEvent - Lua file {}] "
		                 "Call stack overflow. Too many lua script calls being nested",
		                 getLoadingFile());
	}

	// free resources
	luaL_unref(luaState, LUA_REGISTRYINDEX, timerEventDesc.function);
	for (const auto parameter : timerEventDesc.parameters) {
		luaL_unref(luaState, LUA_REGISTRYINDEX, parameter);
	}
}

void LuaEnvironment::collectGarbage() const {
	// prevents recursive collects
	static bool collecting = false;
	if (!collecting) {
		collecting = true;

		// we must collect two times because __gc metamethod
		// is called on uservalues only the second time
		for (int i = -1; ++i < 2;) {
			lua_gc(luaState, LUA_GCCOLLECT, 0);
		}

		collecting = false;
	}
}
