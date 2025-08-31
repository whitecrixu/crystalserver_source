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

#include "creatures/creature.hpp"

enum Direction : uint8_t;
struct Position;
class NpcType;
class Player;
class Tile;
class Creature;
class Game;
class SpawnNpc;

class Npc final : public Creature {
public:
	static std::shared_ptr<Npc> createNpc(const std::string &name);
	static int32_t despawnRange;
	static int32_t despawnRadius;

	explicit Npc(const std::shared_ptr<NpcType> &npcType);
	Npc() = default;

	// Singleton - ensures we don't accidentally copy it
	Npc(const Npc &) = delete;
	void operator=(const std::shared_ptr<Npc> &) = delete;

	static Npc &getInstance();

	std::shared_ptr<Npc> getNpc() override;
	std::shared_ptr<const Npc> getNpc() const override;

	void setID() override;

	void removeList() override;
	void addList() override;

	const std::string &getName() const override;
	// Real npc name, set on npc creation "createNpcType(typeName)"
	const std::string &getTypeName() const override;
	const std::string &getNameDescription() const override;
	std::string getDescription(int32_t) override;

	void setName(std::string newName) const;

	CreatureType_t getType() const override;

	const Position &getMasterPos() const;
	void setMasterPos(Position pos);

	uint8_t getSpeechBubble() const override;
	void setSpeechBubble(const uint8_t bubble) const;

	uint16_t getCurrency() const;
	void setCurrency(uint16_t currency);

	const std::vector<ShopBlock> &getShopItemVector(uint32_t playerGUID) const;

	bool isPushable() override;

	bool isAttackable() const override;

	bool canInteract(const Position &pos, uint32_t range = 4);
	bool canSeeInvisibility() const override;
	RespawnType getRespawnType() const;
	void setSpawnNpc(const std::shared_ptr<SpawnNpc> &newSpawn);

	void setPlayerInteraction(uint32_t playerId, uint16_t topicId = 0);
	void removePlayerInteraction(const std::shared_ptr<Player> &player);
	void resetPlayerInteractions();

	bool isInteractingWithPlayer(uint32_t playerId);
	bool isPlayerInteractingOnTopic(uint32_t playerId, uint16_t topicId);

	void onCreatureAppear(const std::shared_ptr<Creature> &creature, bool isLogin) override;
	void onRemoveCreature(const std::shared_ptr<Creature> &creature, bool isLogout) override;
	void onCreatureMove(const std::shared_ptr<Creature> &creature, const std::shared_ptr<Tile> &newTile, const Position &newPos, const std::shared_ptr<Tile> &oldTile, const Position &oldPos, bool teleport) override;
	void onCreatureSay(const std::shared_ptr<Creature> &creature, SpeakClasses type, const std::string &text) override;
	void onThink(uint32_t interval) override;
	void onPlayerBuyItem(const std::shared_ptr<Player> &player, uint16_t itemid, uint8_t count, uint16_t amount, bool ignore, bool inBackpacks);
	void onPlayerSellAllLoot(uint32_t playerId, uint16_t itemid, bool ignore, uint64_t totalPrice);
	void onPlayerSellItem(const std::shared_ptr<Player> &player, uint16_t itemid, uint8_t count, uint16_t amount, bool ignore);
	void onPlayerSellItem(const std::shared_ptr<Player> &player, uint16_t itemid, uint8_t count, uint16_t amount, bool ignore, uint64_t &totalPrice, const std::shared_ptr<Cylinder> &parent = nullptr);
	void onPlayerCheckItem(const std::shared_ptr<Player> &player, uint16_t itemid, uint8_t count);
	void onPlayerCloseChannel(const std::shared_ptr<Creature> &creature);
	void onPlacedCreature() override;

	bool canWalkTo(const Position &fromPos, Direction dir);
	bool getNextStep(Direction &nextDirection, uint32_t &flags) override;
	bool getRandomStep(Direction &moveDirection);

	void setNormalCreatureLight() override;

	bool isShopPlayer(uint32_t playerGUID) const;

	void addShopPlayer(uint32_t playerGUID, const std::vector<ShopBlock> &shopItems);
	void removeShopPlayer(uint32_t playerGUID);
	void closeAllShopWindows();

	static uint32_t npcAutoID;

	void onCreatureWalk() override;

private:
	void onThinkYell(uint32_t interval);
	void onThinkWalk(uint32_t interval);
	void onThinkSound(uint32_t interval);

	bool isInSpawnRange(const Position &pos) const;

	std::string strDescription;

	std::vector<uint32_t> playerInteractionsOrder;

	std::map<uint32_t, uint16_t> playerInteractions;

	std::unordered_map<uint32_t, std::vector<ShopBlock>> shopPlayers;

	std::shared_ptr<NpcType> npcType;
	std::shared_ptr<SpawnNpc> spawnNpc;

	uint8_t speechBubble {};

	uint32_t yellTicks = 0;
	uint32_t walkTicks = 0;
	uint32_t soundTicks = 0;

	bool ignoreHeight {};

	phmap::flat_hash_set<std::shared_ptr<Player>> playerSpectators;
	Position masterPos;

	friend class LuaScriptInterface;
	friend class Map;

	void onPlayerAppear(const std::shared_ptr<Player> &player);
	void onPlayerDisappear(const std::shared_ptr<Player> &player);
	void manageIdle();
	void handlePlayerMove(const std::shared_ptr<Player> &player, const Position &newPos);
	void loadPlayerSpectators();
};

constexpr auto g_npc = Npc::getInstance;
