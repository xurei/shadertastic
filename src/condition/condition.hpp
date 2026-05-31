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

#ifndef SHADERTASTIC_CONDITION_HPP
#define SHADERTASTIC_CONDITION_HPP

#include <vector>
#include <string>
#include <memory>
#include <obs-data.h>
#include <variant>

using condition_value = std::variant<bool, int, double>;
using condition_member = std::variant<bool, int, double, std::string>;

class condition_t {
    public:
        virtual ~condition_t() = default;

        [[nodiscard]] virtual bool check(obs_data_t *settings) = 0;
};

class condition_bool_group: public condition_t {
    protected:
        std::vector<std::unique_ptr<condition_t>> conditions;
    public:
        #pragma clang diagnostic push
        #pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
        [[nodiscard]] inline auto begin() const {
            return conditions.begin();
        }
        [[nodiscard]] inline auto end() const {
            return conditions.end();
        }
        #pragma clang diagnostic pop
};

class condition_leaf: public condition_t {
    protected:
        condition_member lval;
        condition_member rval;
        condition_leaf(condition_member &lval_, condition_member &rval_)
        : lval(lval_), rval(rval_) {}
    public:
        [[nodiscard]] inline condition_member get_lval() const { return lval; }
        [[nodiscard]] inline condition_member get_rval() const { return rval; }
};

#endif // SHADERTASTIC_CONDITION_HPP
