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

#include "server/network/message/networkmessage.hpp"
#include "items/containers/container.hpp"
#include <boost/locale.hpp>

int32_t NetworkMessage::decodeHeader() {
	// Ensure there are enough bytes to read the header (2 bytes)
	if (!canRead(2)) {
		g_logger().error("[{}] Not enough data to decode header. Current position: {}, Length: {}", __FUNCTION__, info.position, info.length);
		return {};
	}

	// Log the current position and buffer content before decoding
	g_logger().debug("[{}] Decoding header at position: {}", __FUNCTION__, info.position);
	g_logger().debug("[{}] Buffer content: ", __FUNCTION__);

	// Log only up to 10 bytes of the buffer or until the end of the buffer
	for (size_t i = 0; i < 10 && i < buffer.size(); ++i) {
		g_logger().debug("[{}] Buffer[{}]: {}", __FUNCTION__, i, buffer[i]);
	}

	// Check if there are enough bytes in the buffer for the header
	if (info.position + 1 < buffer.size()) {
		// Create a span view for safety
		std::span<uint8_t> bufferSpan(buffer.data(), buffer.size());
		auto decodedHeader = bufferSpan[info.position] | (bufferSpan[info.position + 1] << 8);
		// Update position after reading the header
		info.position += sizeof(decodedHeader);
		return decodedHeader;
	} else {
		g_logger().warn("Index out of bounds when trying to access buffer with position: {}", info.position);
	}

	// Handle buffer overflow error here
	g_logger().error("[{}] attempted to read beyond buffer limits at position: {}", __FUNCTION__, info.position);
	return {};
}

// Simply read functions for incoming message
uint8_t NetworkMessage::getByte(const std::source_location &location /*= std::source_location::current()*/) {
	// Check if there is at least 1 byte to read
	if (!canRead(1)) {
		g_logger().error("[{}] Not enough data to read a byte. Current position: {}, Length: {}. Called line {}:{} in {}", __FUNCTION__, info.position, info.length, location.line(), location.column(), location.function_name());
		return {};
	}

	// Ensure that position is within bounds before decrementing
	if (info.position == 0) {
		g_logger().error("[{}] Position is at the beginning of the buffer. Cannot decrement. Called line {}:{} in {}", __FUNCTION__, location.line(), location.column(), location.function_name());
		return {};
	}

	try {
		// Decrement position safely and return the byte
		return buffer.at(info.position++);
	} catch (const std::out_of_range &e) {
		g_logger().error("[{}] Out of range error: {}. Position: {}, Buffer size: {}. Called line {}:{} in {}", __FUNCTION__, e.what(), info.position, buffer.size(), location.line(), location.column(), location.function_name());
	}

	return {};
}

uint8_t NetworkMessage::getPreviousByte() {
	// Check if position is at the beginning of the buffer
	if (info.position == 0) {
		g_logger().error("[{}] Attempted to get previous byte at position 0", __FUNCTION__);
		return {}; // Return default value (0) when at the start of the buffer
	}

	try {
		// Safely decrement position and access the previous byte using 'at()'
		return buffer.at(--info.position);
	} catch (const std::out_of_range &e) {
		// Log the out-of-range exception if accessing outside buffer limits
		g_logger().error("[{}] Out of range error: {}. Position: {}, Buffer size: {}", __FUNCTION__, e.what(), info.position, buffer.size());
	}

	return {};
}

std::string NetworkMessage::getString(uint16_t stringLen /* = 0*/, const std::source_location &location) {
	if (stringLen == 0) {
		stringLen = get<uint16_t>();
	}

	if (!canRead(stringLen)) {
		g_logger().error("[{}] not enough data to read string of length: {}. Called line {}:{} in {}", __FUNCTION__, stringLen, location.line(), location.column(), location.function_name());
		return {};
	}

	if (stringLen > NETWORKMESSAGE_MAXSIZE) {
		g_logger().error("[{}] exceded NetworkMessage max size: {}, actually size: {}.  Called line '{}:{}' in '{}'", __FUNCTION__, NETWORKMESSAGE_MAXSIZE, stringLen, location.line(), location.column(), location.function_name());
		return {};
	}

	g_logger().trace("[{}] called line '{}:{}' in '{}'", __FUNCTION__, location.line(), location.column(), location.function_name());

	// Copy the string from the buffer
	auto it = buffer.data() + info.position;
	info.position += stringLen;

	// Convert the string to UTF-8 using Boost.Locale
	std::string_view latin1Str { reinterpret_cast<const char*>(it), stringLen };
	return boost::locale::conv::to_utf<char>(latin1Str.data(), latin1Str.data() + latin1Str.size(), "ISO-8859-1", boost::locale::conv::skip);
}

Position NetworkMessage::getPosition() {
	Position pos;
	pos.x = get<uint16_t>();
	pos.y = get<uint16_t>();
	pos.z = getByte();
	return pos;
}

// Skips count unknown/unused bytes in an incoming message
void NetworkMessage::skipBytes(int16_t count) {
	info.position += count;
}

void NetworkMessage::addString(const std::string &value, const std::source_location &location /*= std::source_location::current()*/, const std::string &function /* = ""*/) {
	if (value.empty()) {
		if (!function.empty()) {
			g_logger().debug("[{}] attempted to add an empty string. Called line '{}'", __FUNCTION__, function);
		} else {
			g_logger().debug("[{}] attempted to add an empty string. Called line '{}:{}' in '{}'", __FUNCTION__, location.line(), location.column(), location.function_name());
		}

		// Add a 0 length string
		add<uint16_t>(0);
		return;
	}

	// Convert to ISO-8859-1 using Boost.Locale
	std::string latin1Str = boost::locale::conv::from_utf<char>(
		value.data(),
		value.data() + value.size(),
		"ISO-8859-1",
		boost::locale::conv::skip
	);

	size_t stringLen = latin1Str.size();

	if (!canAdd(stringLen + 2)) {
		if (!function.empty()) {
			g_logger().error("[{}] NetworkMessage size is wrong: {}. Called line '{}'", __FUNCTION__, stringLen, function);
		} else {
			g_logger().error("[{}] NetworkMessage size is wrong: {}. Called line '{}:{}' in '{}'", __FUNCTION__, stringLen, location.line(), location.column(), location.function_name());
		}
		return;
	}

	if (stringLen > NETWORKMESSAGE_MAXSIZE) {
		if (!function.empty()) {
			g_logger().error("[{}] exceeded NetworkMessage max size: {}, actual size: {}. Called line '{}'", __FUNCTION__, NETWORKMESSAGE_MAXSIZE, stringLen, function);
		} else {
			g_logger().error("[{}] exceeded NetworkMessage max size: {}, actual size: {}. Called line '{}:{}' in '{}'", __FUNCTION__, NETWORKMESSAGE_MAXSIZE, stringLen, location.line(), location.column(), location.function_name());
		}
		return;
	}

	if (!function.empty()) {
		g_logger().trace("[{}] called line '{}'", __FUNCTION__, function);
	} else {
		g_logger().trace("[{}] called line '{}:{}' in '{}'", __FUNCTION__, location.line(), location.column(), location.function_name());
	}

	// Add the string length to the buffer
	add<uint16_t>(static_cast<uint16_t>(stringLen));

	// Copy the Latin-1 encoded string to the buffer
	std::memcpy(buffer.data() + info.position, latin1Str.data(), stringLen);

	info.position += stringLen;
	info.length += stringLen;
}

void NetworkMessage::addDouble(double value, uint8_t precision /*= 4*/) {
	addByte(precision);
	add<uint32_t>((value * std::pow(static_cast<float>(SCALING_BASE), precision)) + std::numeric_limits<int32_t>::max());
}

double NetworkMessage::getDouble() {
	// Retrieve the precision byte from the buffer
	uint8_t precision = getByte();
	// Retrieve the scaled uint32_t value from the buffer
	auto scaledValue = get<uint32_t>();
	// Convert the scaled value back to double using the precision factor
	double adjustedValue = static_cast<double>(scaledValue) - static_cast<double>(std::numeric_limits<int32_t>::max());
	// Convert back to the original double value using the precision factor
	return adjustedValue / std::pow(static_cast<double>(SCALING_BASE), precision);
}

void NetworkMessage::addByte(uint8_t value, std::source_location location /*= std::source_location::current()*/) {
	if (!canAdd(1)) {
		g_logger().error("[{}] cannot add byte, buffer overflow. Called line '{}:{}' in '{}'", __FUNCTION__, location.line(), location.column(), location.function_name());
		return;
	}

	g_logger().trace("[{}] called line '{}:{}' in '{}'", __FUNCTION__, location.line(), location.column(), location.function_name());
	try {
		buffer.at(info.position++) = value;
		info.length++;
	} catch (const std::out_of_range &e) {
		g_logger().error("[{}] buffer access out of range: {}. Called line '{}:{}' in '{}'", __FUNCTION__, e.what(), location.line(), location.column(), location.function_name());
	}
}

void NetworkMessage::addBytes(const char* bytes, size_t size) {
	if (bytes == nullptr) {
		g_logger().error("[NetworkMessage::addBytes] - Bytes is nullptr");
		return;
	}
	if (!canAdd(size)) {
		g_logger().error("[NetworkMessage::addBytes] - NetworkMessage size is wrong: {}", size);
		return;
	}
	if (size > NETWORKMESSAGE_MAXSIZE) {
		g_logger().error("[NetworkMessage::addBytes] - Exceded NetworkMessage max size: {}, actually size: {}", NETWORKMESSAGE_MAXSIZE, size);
		return;
	}

	std::ranges::copy(std::span(bytes, size), buffer.begin() + info.position);
	info.position += size;
	info.length += size;
}

void NetworkMessage::addPaddingBytes(size_t n) {
	if (!canAdd(n)) {
		g_logger().error("[NetworkMessage::addPaddingBytes] - Cannot add padding bytes, buffer overflow");
		return;
	}

	std::fill(buffer.begin() + info.position, buffer.begin() + info.position + n, 0x33);
	info.position += n;
	info.length += n;
}

void NetworkMessage::addPosition(const Position &pos) {
	add<uint16_t>(pos.x);
	add<uint16_t>(pos.y);
	addByte(pos.z);
}

NetworkMessage::MsgSize_t NetworkMessage::getLength() const {
	return info.length;
}

void NetworkMessage::setLength(NetworkMessage::MsgSize_t newLength) {
	info.length = newLength;
}

NetworkMessage::MsgSize_t NetworkMessage::getBufferPosition() const {
	return info.position;
}

void NetworkMessage::setBufferPosition(NetworkMessage::MsgSize_t newPosition) {
	info.position = newPosition;
}

uint16_t NetworkMessage::getLengthHeader() const {
	return static_cast<uint16_t>(buffer[0] | buffer[1] << 8);
}

bool NetworkMessage::isOverrun() const {
	return info.overrun;
}

uint8_t* NetworkMessage::getBuffer() {
	return buffer.data();
}

const uint8_t* NetworkMessage::getBuffer() const {
	return buffer.data();
}

uint8_t* NetworkMessage::getBodyBuffer() {
	info.position = 2;
	// Return the pointer to the body of the buffer starting after the header
	// Convert HEADER_LENGTH to uintptr_t to ensure safe pointer arithmetic with enum type
	return buffer.data() + static_cast<std::uintptr_t>(HEADER_LENGTH);
}

bool NetworkMessage::canAdd(size_t size) const {
	return (size + info.position) < MAX_BODY_LENGTH;
}

bool NetworkMessage::canRead(int32_t size) const {
	return size <= (info.length - (info.position - INITIAL_BUFFER_POSITION));
}

void NetworkMessage::append(const NetworkMessage &other) {
	size_t otherLength = other.getLength();
	size_t otherStartPos = NetworkMessage::INITIAL_BUFFER_POSITION; // Always start copying from the initial buffer position

	g_logger().debug("[{}] appending message, other Length = {}, current length = {}, current position = {}, other start position = {}", __FUNCTION__, otherLength, info.length, info.position, otherStartPos);

	// Ensure there is enough space in the buffer to append the new data
	if (!canAdd(otherLength)) {
		std::cerr << "Cannot append message: not enough space in buffer.\n";
		return;
	}

	std::ranges::copy(
		std::span<const unsigned char>(other.getBuffer() + otherStartPos, otherLength),
		buffer.data() + info.position
	);

	// Update the buffer information
	info.length += otherLength;
	info.position += otherLength;
	// Debugging output after appending
	g_logger().debug("After append: New Length = {}, New Position = {}", info.length, info.position);
}
