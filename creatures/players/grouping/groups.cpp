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

#include "creatures/players/grouping/groups.hpp"

#include "config/configmanager.hpp"
#include "game/game.hpp"
#include "utils/pugicast.hpp"
#include "utils/tools.hpp"

namespace ParsePlayerFlagMap {
	// Initialize the map with all the values from the PlayerFlags_t enumeration
	phmap::flat_hash_map<std::string, PlayerFlags_t> initParsePlayerFlagMap() {
		phmap::flat_hash_map<std::string, PlayerFlags_t> map;
		// Iterate through all values of the PlayerFlags_t enumeration
		for (const auto &value : magic_enum::enum_values<PlayerFlags_t>()) {
			// Get the string representation of the current enumeration value
			std::string name(magic_enum::enum_name(value).data());
			// Convert the string to lowercase
			std::ranges::transform(name.begin(), name.end(), name.begin(), ::tolower);
			// Add the current value to the map with its lowercase string representation as the key
			map[name] = value;
		}

		return map;
	}

	const phmap::flat_hash_map<std::string, PlayerFlags_t> parsePlayerFlagMap = initParsePlayerFlagMap();
}

uint8_t Groups::getFlagNumber(PlayerFlags_t playerFlags) {
	return magic_enum::enum_integer(playerFlags);
}

PlayerFlags_t Groups::getFlagFromNumber(uint8_t value) {
	return magic_enum::enum_value<PlayerFlags_t>(value);
}

bool Groups::reload() {
	// Clear groups
	g_game().groups.getGroups().clear();
	return g_game().groups.load();
}

void parseGroupFlags(Group &group, const pugi::xml_node &groupNode) {
	if (const pugi::xml_node node = groupNode.child("flags")) {
		for (const auto &flagNode : node.children()) {
			pugi::xml_attribute attr = flagNode.first_attribute();
			if (!attr || !attr.as_bool()) {
				continue;
			}

			// Ensure always send the string completely in lower case
			std::string string = asLowerCaseString(attr.name());
			auto parseFlag = ParsePlayerFlagMap::parsePlayerFlagMap.find(string);
			if (parseFlag != ParsePlayerFlagMap::parsePlayerFlagMap.end()) {
				group.flags[Groups::getFlagNumber(parseFlag->second)] = true;
			}
		}
	}
}

bool Groups::load() {
	pugi::xml_document doc;
	const auto folder = g_configManager().getString(CORE_DIRECTORY) + "/XML/groups.xml";
	const pugi::xml_parse_result result = doc.load_file(folder.c_str());
	if (!result) {
		printXMLError(__FUNCTION__, folder, result);
		return false;
	}

	for (const auto &groupNode : doc.child("groups").children()) {
		Group group;
		group.id = pugi::cast<uint32_t>(groupNode.attribute("id").value());
		group.name = groupNode.attribute("name").as_string();
		group.access = groupNode.attribute("access").as_bool();
		group.maxDepotItems = pugi::cast<uint32_t>(groupNode.attribute("maxdepotitems").value());
		group.maxVipEntries = pugi::cast<uint32_t>(groupNode.attribute("maxvipentries").value());

		// Make outfit optional flag
		group.outfit = groupNode.attribute("outfit") ? pugi::cast<uint32_t>(groupNode.attribute("outfit").value()) : 0;

		const auto flagsInt = static_cast<uint8_t>(groupNode.attribute("flags").as_uint());
		std::bitset<magic_enum::enum_integer(PlayerFlags_t::FlagLast)> flags(flagsInt);
		for (uint8_t i = 0; i < getFlagNumber(PlayerFlags_t::FlagLast); i++) {
			const PlayerFlags_t flag = getFlagFromNumber(i);
			group.flags[i] = flags[Groups::getFlagNumber(flag)];
		}

		// Parsing group flags
		parseGroupFlags(group, groupNode);

		groups_vector.emplace_back(std::make_shared<Group>(group));
	}
	groups_vector.shrink_to_fit();
	return true;
}

std::shared_ptr<Group> Groups::getGroup(uint16_t id) const {
	if (auto it = std::ranges::find_if(groups_vector, [id](auto group_it) {
			return group_it->id == id;
		});
	    it != groups_vector.end()) {
		return *it;
	}
	return nullptr;
}

std::vector<std::shared_ptr<Group>> &Groups::getGroups() {
	return groups_vector;
}
