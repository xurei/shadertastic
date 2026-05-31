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

#include <optional>
#include <variant>

#include "src/parameters/parameter.hpp"
#include "condition_value.h"

[[nodiscard]] std::optional<condition_value> condition_parameter_reader::read(const condition_member &member_, [[maybe_unused]] const obs_data_t *settings) {
    return std::visit([settings](auto&& member) -> std::optional<condition_value> {
        using T = std::decay_t<decltype(member)>;

        if constexpr (std::is_same_v<T, std::string>) {
            const std::string& full_param_name = member;
            auto *data_item = obs_data_item_byname((obs_data_t *)settings, full_param_name.c_str());
            auto item_type = obs_data_item_gettype(data_item);
            switch (item_type) {
                case OBS_DATA_NUMBER: {
                    double v = obs_data_item_get_double(data_item);
                    return condition_value{static_cast<double>(v)};
                }
                case OBS_DATA_BOOLEAN: {
                    bool v = obs_data_item_get_bool(data_item);
                    return condition_value{static_cast<double>(v)};
                }
                case OBS_DATA_NULL: {
                    log_error("Invalid conditional value: null. Only numbers and boolean values are allowed");
                    return std::nullopt;
                }
                case OBS_DATA_STRING: {
                    log_error("Invalid conditional value: string. Only numbers and boolean values are allowed");
                    return std::nullopt;
                }
                case OBS_DATA_OBJECT: {
                    log_error("Invalid conditional value: object. Only numbers and boolean values are allowed");
                    return std::nullopt;
                }
                case OBS_DATA_ARRAY: {
                    log_error("Invalid conditional value: array. Only numbers and boolean values are allowed");
                    return std::nullopt;
                }
                default: {
                    log_error("Invalid conditional value: unknown. Only numbers and boolean values are allowed");
                    return std::nullopt;
                }
            }
        }
        else {
            return static_cast<condition_value>(member);
        }
    }, member_);
}

[[nodiscard]] std::optional<double> condition_parameter_reader::as_number(const condition_value &value) {
    if (const auto *bool_value = std::get_if<bool>(&value)) {
        return *bool_value ? 1.0 : 0.0;
    }
    if (const auto *int_value = std::get_if<int>(&value)) {
        return static_cast<double>(*int_value);
    }
    if (const auto *double_value = std::get_if<double>(&value)) {
        return *double_value;
    }
    return std::nullopt;
}

[[nodiscard]] bool condition_parameter_reader::equal(const condition_value &current, const condition_value &expected) {
    if (current.index() == expected.index()) {
        return current == expected;
    }

    const auto current_number = as_number(current);
    const auto expected_number = as_number(expected);
    if (!current_number || !expected_number) {
        return false;
    }

    return *current_number == *expected_number;
}

[[nodiscard]] std::optional<double> condition_parameter_reader::compare_numbers(
    const condition_member &actual,
    const condition_member &expected,
    const obs_data_t *settings
) {
    const auto current = read(actual, settings);
    const auto expected_val = read(expected, settings);
    if (!current) {
        return std::nullopt;
    }
    if (!expected_val) {
        return std::nullopt;
    }

    const auto current_number = as_number(*current);
    const auto expected_number = as_number(*expected_val);
    if (!current_number || !expected_number) {
        return std::nullopt;
    }

    return *current_number - *expected_number;
}
