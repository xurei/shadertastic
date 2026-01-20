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

#ifndef SHADERTASTIC_PARAMETER_BOOL_HPP
#define SHADERTASTIC_PARAMETER_BOOL_HPP

#include <string>
#include "parameter.hpp"

class effect_parameter_bool : public effect_parameter {
    private:
        bool default_value{};

    public:
        // Note: the shaders need a 4-byte data for booleans, this is NOT a mistake to use sizeof(int).
        explicit effect_parameter_bool(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_BOOL;
        }

        void initialize_params(const effect_shader *shader, obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            obs_data_set_default_bool(metadata, "default", false);

            default_value = obs_data_get_bool(metadata, "default");
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            obs_data_set_default_bool(settings, full_param_name, default_value);
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            obs_property_t *list_ui = obs_properties_add_list(
                props, full_param_name, label.c_str(),
                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_BOOL
            );
            obs_property_list_add_bool(list_ui, "✓ Yes", true);
            obs_property_list_add_bool(list_ui, "✕ No", false);

            if (!description.empty()) {
                obs_property_set_long_description(list_ui, obs_module_text(description.c_str()));
            }
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            *((int*)this->data) = obs_data_get_bool(settings, full_param_name) ? 1 : 0;
        }
};

#endif // SHADERTASTIC_PARAMETER_BOOL_HPP
