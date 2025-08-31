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

#include "items/decay/decay.hpp"

#include "creatures/players/player.hpp"
#include "game/game.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "lib/di/container.hpp"

Decay &Decay::getInstance() {
	return inject<Decay>();
}

void Decay::startDecay(const std::shared_ptr<Item> &item, int32_t duration) {
	if (!item || duration <= 0) {
		return;
	}

	if (item->hasAttribute(ItemAttribute_t::DURATION_TIMESTAMP)) {
		stopDecay(item, item->getAttribute<int64_t>(ItemAttribute_t::DURATION_TIMESTAMP));
	}

	int64_t timestamp = OTSYS_TIME() + static_cast<int64_t>(duration);
	if (decayMap.empty()) {
		eventId = g_dispatcher().scheduleEvent(
			std::max<int32_t>(SCHEDULER_MINTICKS, duration), [this] { checkDecay(); }, "Decay::checkDecay"
		);
	} else {
		if (timestamp < decayMap.begin()->first) {
			eventId = g_dispatcher().scheduleEvent(
				std::max<int32_t>(SCHEDULER_MINTICKS, duration), [this] { checkDecay(); }, "Decay::checkDecay"
			);
		}
	}

	// item->incrementReferenceCounter();
	item->setDecaying(DECAYING_TRUE);
	item->setDurationTimestamp(timestamp);
	decayMap[timestamp].push_back(item);
}

void Decay::stopDecay(const std::shared_ptr<Item> &item, int64_t timestamp) {
	if (!item || timestamp <= 0) {
		return;
	}

	auto it = decayMap.find(timestamp);
	if (it != decayMap.end()) {
		std::vector<std::shared_ptr<Item>> &decayItems = it->second;

		size_t i = 0, end = decayItems.size();
		if (end == 1) {
			if (item == decayItems[i]) {
				if (item->hasAttribute(ItemAttribute_t::DURATION)) {
					// Incase we removed duration attribute don't assign new duration
					item->setDuration(item->getDuration());
				}
				item->removeAttribute(ItemAttribute_t::DECAYSTATE);
				decayMap.erase(it);
			}
			return;
		}
		while (i < end) {
			if (item == decayItems[i]) {
				if (item->hasAttribute(ItemAttribute_t::DURATION)) {
					// Incase we removed duration attribute don't assign new duration
					item->setDuration(item->getDuration());
				}
				item->removeAttribute(ItemAttribute_t::DECAYSTATE);
				decayItems[i] = decayItems.back();
				decayItems.pop_back();
				return;
			}
			++i;
		}
	}
}

void Decay::checkDecay() {
	int64_t timestamp = OTSYS_TIME();

	std::vector<std::shared_ptr<Item>> tempItems;
	tempItems.reserve(32); // Small preallocation

	auto it = decayMap.begin(), end = decayMap.end();
	while (it != end) {
		if (it->first > timestamp) {
			break;
		}

		// Iterating here is unsafe so let's copy our items into temporary vector
		std::vector<std::shared_ptr<Item>> &decayItems = it->second;
		tempItems.insert(tempItems.end(), decayItems.begin(), decayItems.end());
		it = decayMap.erase(it);
	}

	for (const auto item : tempItems) {
		if (!item->canDecay()) {
			item->setDuration(item->getDuration());
			item->setDecaying(DECAYING_FALSE);
		} else {
			item->setDecaying(DECAYING_FALSE);
			internalDecayItem(item);
		}
	}

	if (it != end) {
		eventId = g_dispatcher().scheduleEvent(
			std::max<int32_t>(SCHEDULER_MINTICKS, static_cast<int32_t>(it->first - timestamp)), [this] { checkDecay(); }, "Decay::checkDecay"
		);
	}
}

void Decay::internalDecayItem(const std::shared_ptr<Item> &item) {
	if (!item) {
		return;
	}

	const ItemType &it = Item::items[item->getID()];
	// Remove the item and halt the decay process if a player triggers a bug where the item's decay ID matches its equip or de-equip transformation ID
	if (it.id == it.transformEquipTo || it.id == it.transformDeEquipTo) {
		g_game().internalRemoveItem(item);
		const auto &player = item->getHoldingPlayer();
		if (player) {
			g_logger().error("[{}] - internalDecayItem failed to player {}, item id is same from transform equip/deequip, "
			                 " item id: {}, equip to id: '{}', deequip to id '{}'",
			                 __FUNCTION__, player->getName(), it.id, it.transformEquipTo, it.transformDeEquipTo);
		}
		return;
	}

	if (it.decayTo != 0) {
		const auto &player = item->getHoldingPlayer();
		if (player) {
			bool needUpdateSkills = false;
			for (int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) {
				if (it.abilities && item->getSkill(static_cast<skills_t>(i)) != 0) {
					needUpdateSkills = true;
					player->setVarSkill(static_cast<skills_t>(i), -item->getSkill(static_cast<skills_t>(i)));
				}
			}

			if (needUpdateSkills) {
				player->sendSkills();
			}

			bool needUpdateStats = false;
			for (int32_t s = STAT_FIRST; s <= STAT_LAST; ++s) {
				if (item->getStat(static_cast<stats_t>(s)) != 0) {
					needUpdateStats = true;
					needUpdateSkills = true;
					player->setVarStats(static_cast<stats_t>(s), -item->getStat(static_cast<stats_t>(s)));
				}
				if (it.abilities && it.abilities->statsPercent[s] != 0) {
					needUpdateStats = true;
					player->setVarStats(static_cast<stats_t>(s), -static_cast<int32_t>(player->getDefaultStats(static_cast<stats_t>(s)) * ((it.abilities->statsPercent[s] - 100) / 100.f)));
				}
			}

			if (needUpdateStats) {
				player->sendStats();
			}

			if (needUpdateSkills) {
				player->sendSkills();
			}
		}
		g_game().transformItem(item, static_cast<uint16_t>(it.decayTo));
	} else {
		if (item->isLoadedFromMap()) {
			return;
		}

		ReturnValue ret = g_game().internalRemoveItem(item);
		if (ret != RETURNVALUE_NOERROR) {
			g_logger().error("[Decay::internalDecayItem] - internalDecayItem failed, "
			                 "error code: {}, item id: {}",
			                 static_cast<uint32_t>(ret), item->getID());
		}
	}
}
