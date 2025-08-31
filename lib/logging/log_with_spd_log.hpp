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

#include "lib/logging/logger.hpp"

class LogWithSpdLog final : public Logger {
public:
	LogWithSpdLog();
	~LogWithSpdLog() override = default;

	static Logger &getInstance();

	void setLevel(const std::string &name) const override;
	std::string getLevel() const override;

	void info(const std::string &msg) const override;
	void warn(const std::string &msg) const override;
	void error(const std::string &msg) const override;
	void critical(const std::string &msg) const override;

#if defined(DEBUG_LOG)
	void debug(const std::string &msg) const override;
	void trace(const std::string &msg) const override;

	template <typename... Args>
	void debug(const fmt::format_string<Args...> &fmt, Args &&... args) const {
		debug(fmt::format(fmt, std::forward<Args>(args)...));
	}

	template <typename... Args>
	void trace(const fmt::format_string<Args...> &fmt, Args &&... args) const {
		trace(fmt::format(fmt, std::forward<Args>(args)...));
	}
#else
	void debug(const std::string &) const override { }
	void trace(const std::string &) const override { }

	template <typename... Args>
	void debug(const fmt::format_string<Args...> &, Args &&...) const { }

	template <typename... Args>
	void trace(const fmt::format_string<Args...> &, Args &&...) const { }
#endif
};

constexpr auto g_logger = LogWithSpdLog::getInstance;
