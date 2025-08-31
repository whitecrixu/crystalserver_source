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

#include "items/functions/item/attribute.hpp"

#include "utils/tools.hpp"

/*
=============================
* ItemAttribute class (Attributes methods)
=============================
*/
const std::string &ItemAttribute::getAttributeString(ItemAttribute_t type) const {
	static std::string emptyString;
	if (!isAttributeString(type)) {
		return emptyString;
	}

	const auto attribute = getAttribute(type);
	if (!attribute) {
		return emptyString;
	}

	return *attribute->getString();
}

const int64_t &ItemAttribute::getAttributeValue(ItemAttribute_t type) const {
	static int64_t emptyInt;
	if (!isAttributeInteger(type)) {
		return emptyInt;
	}

	const auto attribute = getAttribute(type);
	if (!attribute) {
		return emptyInt;
	}

	return attribute->getInteger();
}

const Attributes* ItemAttribute::getAttribute(ItemAttribute_t type) const {
	if (hasAttribute(type)) {
		for (const Attributes &attribute : attributeVector) {
			if (attribute.getAttributeType() == type) {
				return &attribute;
			}
		}
	}
	return nullptr;
}

Attributes &ItemAttribute::getAttributesByType(ItemAttribute_t type) {
	for (Attributes &attribute : attributeVector) {
		if (attribute.getAttributeType() == type) {
			return attribute;
		}
	}

	attributeVector.emplace_back(type);
	return attributeVector.back();
}

void ItemAttribute::setAttribute(ItemAttribute_t type, int64_t value) {
	if (!isAttributeInteger(type)) {
		return;
	}

	getAttributesByType(type).setValue(value);
}

void ItemAttribute::setAttribute(ItemAttribute_t type, const std::string &value) {
	if (!isAttributeString(type)) {
		return;
	}

	if (value.empty()) {
		return;
	}

	getAttributesByType(type).setValue(value);
}

bool ItemAttribute::removeAttribute(ItemAttribute_t type) {
	for (auto it = attributeVector.begin(); it != attributeVector.end(); ++it) {
		if (it->getAttributeType() == type) {
			*it = std::move(attributeVector.back());
			attributeVector.pop_back();
			return true;
		}
	}
	return false;
}

/*
=============================
* CustomAttribute map methods
=============================
*/
const std::map<std::string, CustomAttribute, std::less<>> &ItemAttribute::getCustomAttributeMap() const {
	return customAttributeMap;
}

/*
=============================
* CustomAttribute object methods
=============================
*/
const CustomAttribute* ItemAttribute::getCustomAttribute(const std::string &attributeName) const {
	if (customAttributeMap.contains(asLowerCaseString(attributeName))) {
		return &customAttributeMap.at(asLowerCaseString(attributeName));
	}
	return nullptr;
}

void ItemAttribute::setCustomAttribute(const std::string &key, const int64_t value) {
	const CustomAttribute attribute(key, value);
	customAttributeMap[asLowerCaseString(key)] = attribute;
}

void ItemAttribute::setCustomAttribute(const std::string &key, const std::string &value) {
	const CustomAttribute attribute(key, value);
	customAttributeMap[asLowerCaseString(key)] = attribute;
}

void ItemAttribute::setCustomAttribute(const std::string &key, const double value) {
	const CustomAttribute attribute(key, value);
	customAttributeMap[asLowerCaseString(key)] = attribute;
}

void ItemAttribute::setCustomAttribute(const std::string &key, const bool value) {
	const CustomAttribute attribute(key, value);
	customAttributeMap[asLowerCaseString(key)] = attribute;
}

void ItemAttribute::addCustomAttribute(const std::string &key, const CustomAttribute &customAttribute) {
	customAttributeMap[asLowerCaseString(key)] = customAttribute;
}

bool ItemAttribute::removeCustomAttribute(const std::string &attributeName) {
	const auto it = customAttributeMap.find(asLowerCaseString(attributeName));
	if (it == customAttributeMap.end()) {
		return false;
	}

	customAttributeMap.erase(it);
	return true;
}
