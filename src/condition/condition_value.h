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

#ifndef SHADERTASTIC_CONDITION_VALUE_H
#define SHADERTASTIC_CONDITION_VALUE_H

#include <optional>
#include <variant>

#include "condition.hpp"

struct condition_parameter_reader {
    [[nodiscard]] static std::optional<condition_value> read(const condition_member &member_, [[maybe_unused]] const obs_data_t *settings);

    [[nodiscard]] static std::optional<double> as_number(const condition_value &value);

    [[nodiscard]] static bool equal(const condition_value &current, const condition_value &expected);

    [[nodiscard]] static std::optional<double> compare_numbers(
        const condition_member &actual,
        const condition_member &expected,
        const obs_data_t *settings
    );
};

#endif // SHADERTASTIC_CONDITION_VALUE_H
