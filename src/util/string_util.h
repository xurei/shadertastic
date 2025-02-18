/******************************************************************************
Copyright (C) 2023 by xurei <xureilab@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef SHADERTASTIC_STRING_UTIL_H
#define SHADERTASTIC_STRING_UTIL_H

#include <string>

bool ends_with(const std::string &input, const std::string &suffix);
bool starts_with(const std::string &input, const std::string &prefix);

#endif //SHADERTASTIC_STRING_UTIL_H
