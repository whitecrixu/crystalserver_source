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

#pragma once

#include "lua/functions/creatures/npc/npc_type_functions.hpp"
#include "lua/functions/creatures/npc/shop_functions.hpp"
class NpcFunctions {
private:
	static void init(lua_State* L);

	static int luaNpcCreate(lua_State* L);

	static int luaNpcIsNpc(lua_State* L);

	static int luaNpcSetMasterPos(lua_State* L);
	static int luaNpcGetMasterPos(lua_State* L);

	static int luaNpcGetCurrency(lua_State* L);
	static int luaNpcSetCurrency(lua_State* L);
	static int luaNpcGetSpeechBubble(lua_State* L);
	static int luaNpcSetSpeechBubble(lua_State* L);
	static int luaNpcGetId(lua_State* L);
	static int luaNpcGetName(lua_State* L);
	static int luaNpcSetName(lua_State* L);
	static int luaNpcPlace(lua_State* L);
	static int luaNpcSay(lua_State* L);
	static int luaNpcTurnToCreature(lua_State* L);
	static int luaNpcSetPlayerInteraction(lua_State* L);
	static int luaNpcRemovePlayerInteraction(lua_State* L);
	static int luaNpcIsInteractingWithPlayer(lua_State* L);
	static int luaNpcIsInTalkRange(lua_State* L);
	static int luaNpcIsPlayerInteractingOnTopic(lua_State* L);
	static int luaNpcOpenShopWindow(lua_State* L);
	static int luaNpcOpenShopWindowTable(lua_State* L);
	static int luaNpcCloseShopWindow(lua_State* L);
	static int luaNpcGetShopItem(lua_State* L);
	static int luaNpcIsMerchant(lua_State* L);

	static int luaNpcMove(lua_State* L);
	static int luaNpcTurn(lua_State* L);
	static int luaNpcFollow(lua_State* L);
	static int luaNpcSellItem(lua_State* L);

	static int luaNpcGetDistanceTo(lua_State* L);

	friend class CreatureFunctions;
};
