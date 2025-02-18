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

#ifndef SHADERTASTIC_PARAMETER_FACTORY_HPP
#define SHADERTASTIC_PARAMETER_FACTORY_HPP

class effect_parameter_factory {
    public:
        effect_parameter *create(const std::string &effect_name, const std::string &effect_path, gs_eparam_t *shader_param, obs_data_t *param_metadata);

    private:
        effect_param_datatype effect_parse_datatype(const char *datatype_str);
};

inline effect_parameter_factory parameter_factory;

#endif /* SHADERTASTIC_PARAMETER_FACTORY_HPP */
