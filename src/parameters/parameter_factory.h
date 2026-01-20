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

#ifndef SHADERTASTIC_PARAMETER_FACTORY_H
#define SHADERTASTIC_PARAMETER_FACTORY_H

#include "parameter.hpp"

class effect_parameter_factory {
    public:
        static effect_parameter *create(const std::string &effect_name, const std::string &effect_path, const effect_shader *main_shader, obs_data_t *param_metadata);

    private:
        static effect_param_datatype effect_parse_datatype(const char *datatype_str);
};

#endif // SHADERTASTIC_PARAMETER_FACTORY_H
