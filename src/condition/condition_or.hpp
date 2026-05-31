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

#ifndef SHADERTASTIC_CONDITION_OR_HPP
#define SHADERTASTIC_CONDITION_OR_HPP

#include <memory>
#include <utility>
#include <vector>

#include "condition.hpp"

class condition_or : public condition_bool_group {
    public:
        condition_or() = default;

        void add(std::unique_ptr<condition_t> condition) {
            conditions.push_back(std::move(condition));
        }

        [[nodiscard]] bool check(obs_data_t *settings) override {
            for (const auto &condition : conditions) {
                if (condition == nullptr) {
                    continue;
                }
                if (condition->check(settings)) {
                    return true;
                }
            }
            return false;
        }
};

#endif // SHADERTASTIC_CONDITION_OR_HPP
