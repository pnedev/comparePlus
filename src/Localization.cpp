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

#include <fstream>
#include "nlohmann/json.hpp"
#include "Localization.h"


Localization::Localization()
{
	//Default messages in English
	messages = {
		{ "key1", "Message1" },
		{ "key2", "Message2 %s" }
	};
}


bool Localization::readFromFile(const std::string& json_locale_file)
{
	if (json_locale_file.empty())
		return false;

	try
	{
		std::ifstream file(json_locale_file);

		if (file.is_open())
			file >> messages;
	}
	catch(std::exception& e)
	{
		return false;
	}

	return true;
}
