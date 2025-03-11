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

#ifndef SHADERTASTIC_COMPARE_NOCASE_HPP
#define SHADERTASTIC_COMPARE_NOCASE_HPP

// String comparison, not case sensitive.
static bool compare_nocase(const std::string& first, const std::string& second) {
    unsigned int i=0;
    while ( (i<first.length()) && (i<second.length()) )
    {
        if (tolower(first[i]) < tolower(second[i])) {
            return true;
        }
        else if (tolower(first[i]) > tolower(second[i])) {
            return false;
        }
        ++i;
    }
    return ( first.length() < second.length() );
}
#endif /* SHADERTASTIC_COMPARE_NOCASE_HPP */
