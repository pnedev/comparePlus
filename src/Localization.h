/*
 * This file is part of ComparePlus plugin for Notepad++
 * Author: Kristjan ESPERANTO <35647502+KristjanESPERANTO@users.noreply.github.com>
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
#include <vector>
#include <unordered_map>

// Localization functions
extern std::string getUILanguage(bool forceRefresh = false);
extern std::wstring getLocalizedString(const std::string& lang, const std::string& section, const std::string& key);
