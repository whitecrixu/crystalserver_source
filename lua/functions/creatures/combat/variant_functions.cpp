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

#include "lua/functions/creatures/combat/variant_functions.hpp"

#include "items/cylinder.hpp"
#include "lua/global/lua_variant.hpp"
#include "lua/functions/lua_functions_loader.hpp"

void VariantFunctions::init(lua_State* L) {
	Lua::registerClass(L, "Variant", "", VariantFunctions::luaVariantCreate);

	Lua::registerMethod(L, "Variant", "getNumber", VariantFunctions::luaVariantGetNumber);
	Lua::registerMethod(L, "Variant", "getString", VariantFunctions::luaVariantGetString);
	Lua::registerMethod(L, "Variant", "getPosition", VariantFunctions::luaVariantGetPosition);
}

int VariantFunctions::luaVariantCreate(lua_State* L) {
	// Variant(number or string or position or thing)
	LuaVariant variant;
	if (Lua::isUserdata(L, 2)) {
		if (const auto &thing = Lua::getThing(L, 2)) {
			variant.type = VARIANT_TARGETPOSITION;
			variant.pos = thing->getPosition();
		}
	} else if (Lua::isTable(L, 2)) {
		variant.type = VARIANT_POSITION;
		variant.pos = Lua::getPosition(L, 2);
	} else if (Lua::isNumber(L, 2)) {
		variant.type = VARIANT_NUMBER;
		variant.number = Lua::getNumber<uint32_t>(L, 2);
	} else if (Lua::isString(L, 2)) {
		variant.type = VARIANT_STRING;
		variant.text = Lua::getString(L, 2);
	}
	Lua::pushVariant(L, variant);
	return 1;
}

int VariantFunctions::luaVariantGetNumber(lua_State* L) {
	// Variant:Lua::getNumber()
	const LuaVariant &variant = Lua::getVariant(L, 1);
	if (variant.type == VARIANT_NUMBER) {
		lua_pushnumber(L, variant.number);
	} else {
		lua_pushnumber(L, 0);
	}
	return 1;
}

int VariantFunctions::luaVariantGetString(lua_State* L) {
	// Variant:Lua::getString()
	const LuaVariant &variant = Lua::getVariant(L, 1);
	if (variant.type == VARIANT_STRING) {
		Lua::pushString(L, variant.text);
	} else {
		Lua::pushString(L, std::string());
	}
	return 1;
}

int VariantFunctions::luaVariantGetPosition(lua_State* L) {
	// Variant:Lua::getPosition()
	const LuaVariant &variant = Lua::getVariant(L, 1);
	if (variant.type == VARIANT_POSITION || variant.type == VARIANT_TARGETPOSITION) {
		Lua::pushPosition(L, variant.pos);
	} else {
		Lua::pushPosition(L, Position());
	}
	return 1;
}
