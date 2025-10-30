/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include "nlohmann/json_fwd.hpp"
#include "Tools.h"


using json = nlohmann::json;


class Localization
{
public:
	Localization& get()
	{
		static Localization inst;
		return inst;
	}

	bool readFromFile(const std::string& json_locale_file);

	inline std::string str(const std::string& key) const
	{
		if (messages.contains(key))
			return messages[key];

		return {};
	}

	inline std::wstring wstr(const std::string& key) const
	{
		std::string msg = str(key);

		return MBtoWC(msg.c_str(), msg.size());
	}

private:
	Localization();

	Localization(const Localization&) = delete;
	Localization(Localization&&) = delete;

	Localization& operator=(const Localization&) = delete;

	json messages;
};
