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

#ifndef SHADERTASTIC_CONDITION_NOT_HPP
#define SHADERTASTIC_CONDITION_NOT_HPP

#include <memory>
#include <utility>
#include <vector>

#include "condition.hpp"

class condition_not : public condition_bool_group {
    private:
        std::unique_ptr<condition_t> sub_condition;

    public:
        explicit condition_not(std::unique_ptr<condition_t> sub_condition_):
            sub_condition(std::move(sub_condition_)) {};

        [[nodiscard]] inline condition_t* get_rval() const { return sub_condition.get(); }

        [[nodiscard]] bool check(obs_data_t *settings) override {
            if (sub_condition == nullptr) {
                return false;
            }
            if (!sub_condition->check(settings)) {
                return false;
            }
            return true;
        }
};

#endif // SHADERTASTIC_CONDITION_NOT_HPP
