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

#include "items/containers/mailbox/mailbox.hpp"

#include "creatures/players/player.hpp"
#include "game/game.hpp"
#include "game/scheduling/save_manager.hpp"
#include "items/containers/inbox/inbox.hpp"
#include "map/spectators.hpp"

ReturnValue Mailbox::queryAdd(int32_t, const std::shared_ptr<Thing> &thing, uint32_t, uint32_t, const std::shared_ptr<Creature> &) {
	const auto &item = thing->getItem();
	if (item && Mailbox::canSend(item)) {
		return RETURNVALUE_NOERROR;
	}
	return RETURNVALUE_NOTPOSSIBLE;
}

ReturnValue Mailbox::queryMaxCount(int32_t, const std::shared_ptr<Thing> &, uint32_t count, uint32_t &maxQueryCount, uint32_t) {
	maxQueryCount = std::max<uint32_t>(1, count);
	return RETURNVALUE_NOERROR;
}

ReturnValue Mailbox::queryRemove(const std::shared_ptr<Thing> &, uint32_t, uint32_t, const std::shared_ptr<Creature> & /*= nullptr */) {
	return RETURNVALUE_NOTPOSSIBLE;
}

std::shared_ptr<Cylinder> Mailbox::queryDestination(int32_t &, const std::shared_ptr<Thing> &, std::shared_ptr<Item> &, uint32_t &) {
	return getMailbox();
}

void Mailbox::addThing(const std::shared_ptr<Thing> &thing) {
	return addThing(0, thing);
}

void Mailbox::addThing(int32_t, const std::shared_ptr<Thing> &thing) {
	if (!thing) {
		return;
	}

	const auto &item = thing->getItem();
	if (item && Mailbox::canSend(item)) {
		sendItem(item);
	}
}

void Mailbox::updateThing(const std::shared_ptr<Thing> &, uint16_t, uint32_t) {
	//
}

void Mailbox::replaceThing(uint32_t, const std::shared_ptr<Thing> &) {
	//
}

void Mailbox::removeThing(const std::shared_ptr<Thing> &, uint32_t) {
	//
}

void Mailbox::postAddNotification(const std::shared_ptr<Thing> &thing, const std::shared_ptr<Cylinder> &oldParent, int32_t index, CylinderLink_t) {
	getParent()->postAddNotification(thing, oldParent, index, LINK_PARENT);
}

void Mailbox::postRemoveNotification(const std::shared_ptr<Thing> &thing, const std::shared_ptr<Cylinder> &newParent, int32_t index, CylinderLink_t) {
	getParent()->postRemoveNotification(thing, newParent, index, LINK_PARENT);
}

bool Mailbox::sendItem(const std::shared_ptr<Item> &item) const {
	std::string receiver;
	if (!getReceiver(item, receiver)) {
		return false;
	}

	/**No need to continue if its still empty**/
	if (receiver.empty()) {
		return false;
	}

	if (item && item->getContainer() && item->getTile()) {
		for (const auto &spectator : Spectators().find<Player>(item->getTile()->getPosition())) {
			spectator->getPlayer()->autoCloseContainers(item->getContainer());
		}
	}

	const auto &player = g_game().getPlayerByName(receiver, true);
	std::string writer;
	time_t date = getTimeNow();
	std::string text;
	if (item && item->getID() == ITEM_LETTER && !item->getAttribute<std::string>(ItemAttribute_t::WRITER).empty()) {
		writer = item->getAttribute<std::string>(ItemAttribute_t::WRITER);
		date = item->getAttribute<time_t>(ItemAttribute_t::DATE);
		text = item->getAttribute<std::string>(ItemAttribute_t::TEXT);
	}
	if (player && item) {
		const auto &playerInbox = player->getInbox();
		const auto &itemParent = item->getParent();
		if (g_game().internalMoveItem(itemParent, playerInbox, INDEX_WHEREEVER, item, item->getItemCount(), nullptr, FLAG_NOLIMIT) == RETURNVALUE_NOERROR) {
			const auto &newItem = g_game().transformItem(item, item->getID() + 1);
			if (newItem && newItem->getID() == ITEM_LETTER_STAMPED && !writer.empty()) {
				newItem->setAttribute(ItemAttribute_t::WRITER, writer);
				newItem->setAttribute(ItemAttribute_t::DATE, date);
				newItem->setAttribute(ItemAttribute_t::TEXT, text);
			}
			if (player->isOnline()) {
				player->onReceiveMail();
			} else {
				g_saveManager().savePlayer(player);
			}
			return true;
		}
	}
	return false;
}

bool Mailbox::getReceiver(const std::shared_ptr<Item> &item, std::string &name) const {
	const std::shared_ptr<Container> &container = item->getContainer();
	if (container) {
		for (const std::shared_ptr<Item> &containerItem : container->getItemList()) {
			if (containerItem->getID() == ITEM_LABEL && getReceiver(containerItem, name)) {
				return true;
			}
		}
		return false;
	}

	const std::string &text = item->getAttribute<std::string>(ItemAttribute_t::TEXT);
	if (text.empty()) {
		return false;
	}

	name = getFirstLine(text);
	trimString(name);
	return true;
}

bool Mailbox::canSend(const std::shared_ptr<Item> &item) {
	return !item->hasOwner() && (item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER);
}
