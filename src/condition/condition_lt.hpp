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

#ifndef SHADERTASTIC_CONDITION_LT_HPP
#define SHADERTASTIC_CONDITION_LT_HPP

#include "condition.hpp"
#include "condition_value.h"

class condition_lt : public condition_leaf {
    public:
        condition_lt(condition_member &parameter, condition_member &expected_)
            : condition_leaf(parameter, expected_) {
        }

        [[nodiscard]] bool check(obs_data_t *settings) override {
            const auto difference = condition_parameter_reader::compare_numbers(lval, rval, settings);
            if (!difference) {
                return false;
            }
            return *difference < 0.0;
        }
};

#endif // SHADERTASTIC_CONDITION_LT_HPP
