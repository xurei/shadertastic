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

#ifndef SHADERTASTIC_PARAMETER_INT_HPP
#define SHADERTASTIC_PARAMETER_INT_HPP

#include <string>
#include "parameter.hpp"

class effect_parameter_int : public effect_parameter {
    private:
        int default_value{};
        bool is_slider{};
        int param_min{};
        int param_max{};
        int param_step{};

    public:
        explicit effect_parameter_int(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_INT;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);
            json_t *slider_json = json_object_get(metadata, "slider");
            json_t *min_json = json_object_get(metadata, "min");
            json_t *max_json = json_object_get(metadata, "max");
            json_t *step_json = json_object_get(metadata, "step");
            json_t *default_json = json_object_get(metadata, "default");

            default_value = json_is_integer(default_json) ? (int)json_integer_value(default_json) : 50;
            is_slider = json_is_boolean(slider_json) ? json_boolean_value(slider_json) : false;
            param_min = json_is_integer(min_json) ? (int)json_integer_value(min_json) : 0;
            param_max = json_is_integer(max_json) ? (int)json_integer_value(max_json) : 100;
            param_step = json_is_integer(step_json) ? (int)json_integer_value(step_json) : 1;
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            obs_data_set_default_int(settings, full_param_name, default_value);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            obs_property_t *prop;
            std::string full_param_name = get_full_param_name(effect_name);
            if (is_slider) {
                prop = obs_properties_add_int_slider(props, full_param_name.c_str(), label.c_str(), param_min, param_max, param_step);
            }
            else {
                prop = obs_properties_add_int(props, full_param_name.c_str(), label.c_str(), param_min, param_max, param_step);
            }
            if (!description.empty()) {
                obs_property_set_long_description(prop, obs_module_text(description.c_str()));
            }
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            *((int*)this->data) = (int)obs_data_get_int(settings, full_param_name.c_str());
            //debug("%s = %d", full_param_name, *((int*)this->data));
        }
};

#endif // SHADERTASTIC_PARAMETER_INT_HPP
