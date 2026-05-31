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

#ifndef SHADERTASTIC_COLLECT_STRINGS_H
#define SHADERTASTIC_COLLECT_STRINGS_H

#include <string>
#include "condition.hpp"
#include "condition_not.hpp"
#include "condition_value.h"

inline void collect_strings_from_member(const condition_member &member, std::vector<std::string> &out) {
    if (const auto *value = std::get_if<std::string>(&member)) {
        out.push_back(*value);
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
static void collect_strings_from_condition(const condition_t *cond, std::vector<std::string> &out) {
    if (cond == nullptr) {
        return;
    }

    const auto *leaf = dynamic_cast<const condition_leaf*>(cond);
    if (leaf != nullptr) {
        collect_strings_from_member(leaf->get_lval(), out);
        collect_strings_from_member(leaf->get_rval(), out);
        return;
    }

    const auto *grouped_conditions = dynamic_cast<const condition_bool_group*>(cond);
    if (grouped_conditions != nullptr) {
        for (const auto &child : *grouped_conditions) {
            collect_strings_from_condition(child.get(), out);
        }
        return;
    }

    const auto *not_condition = dynamic_cast<const condition_not*>(cond);
    if (not_condition != nullptr) {
        collect_strings_from_condition(not_condition->get_rval(), out);
        return;
    }
}
#pragma clang diagnostic pop

#endif // SHADERTASTIC_COLLECT_STRINGS_H
