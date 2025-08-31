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

#include "items/item.hpp"
#include "items/cylinder.hpp"
#include "utils/utils_definitions.hpp"

class TrashHolder final : public Item, public Cylinder {
public:
	explicit TrashHolder(uint16_t itemId, MagicEffectClasses _effect = CONST_ME_NONE) :
		Item(itemId), effect(_effect) { }

	std::shared_ptr<TrashHolder> shared_from_this() {
		return std::static_pointer_cast<TrashHolder>(Item::shared_from_this());
	}

	std::shared_ptr<TrashHolder> getTrashHolder() override {
		return static_self_cast<TrashHolder>();
	}

	std::shared_ptr<Cylinder> getCylinder() override {
		return getTrashHolder();
	}

	std::shared_ptr<Tile> getTile() override {
		return Item::getTile();
	}

	// cylinder implementations
	ReturnValue queryAdd(int32_t index, const std::shared_ptr<Thing> &thing, uint32_t count, uint32_t flags, const std::shared_ptr<Creature> &actor = nullptr) override;
	ReturnValue queryMaxCount(int32_t index, const std::shared_ptr<Thing> &thing, uint32_t count, uint32_t &maxQueryCount, uint32_t flags) override;
	ReturnValue queryRemove(const std::shared_ptr<Thing> &thing, uint32_t count, uint32_t flags, const std::shared_ptr<Creature> &actor = nullptr) override;
	std::shared_ptr<Cylinder> queryDestination(int32_t &index, const std::shared_ptr<Thing> &thing, std::shared_ptr<Item> &destItem, uint32_t &flags) override;

	void addThing(const std::shared_ptr<Thing> &thing) override;
	void addThing(int32_t index, const std::shared_ptr<Thing> &thing) override;

	void updateThing(const std::shared_ptr<Thing> &thing, uint16_t itemId, uint32_t count) override;
	void replaceThing(uint32_t index, const std::shared_ptr<Thing> &thing) override;

	void removeThing(const std::shared_ptr<Thing> &thing, uint32_t count) override;

	void postAddNotification(const std::shared_ptr<Thing> &thing, const std::shared_ptr<Cylinder> &oldParent, int32_t index, CylinderLink_t link = LINK_OWNER) override;
	void postRemoveNotification(const std::shared_ptr<Thing> &thing, const std::shared_ptr<Cylinder> &newParent, int32_t index, CylinderLink_t link = LINK_OWNER) override;

	MagicEffectClasses getEffect() const {
		return effect;
	}

private:
	MagicEffectClasses effect;
};
