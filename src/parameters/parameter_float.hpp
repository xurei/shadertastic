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

#ifndef SHADERTASTIC_PARAMETER_FLOAT_HPP
#define SHADERTASTIC_PARAMETER_FLOAT_HPP

#include <string>
#include "parameter.hpp"

class effect_parameter_float : public effect_parameter {
    private:
        double default_value{};
        bool is_slider{};
        double param_min{};
        double param_max{};
        double param_step{};
        obs_property_t *ui_prop{nullptr};

    public:
        explicit effect_parameter_float(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_DOUBLE;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);
            json_t *slider_json = json_object_get(metadata, "slider");
            json_t *min_json = json_object_get(metadata, "min");
            json_t *max_json = json_object_get(metadata, "max");
            json_t *step_json = json_object_get(metadata, "step");
            json_t *default_json = json_object_get(metadata, "default");

            is_slider = json_is_boolean(slider_json) ? json_boolean_value(slider_json) : false;
            param_min = json_number_value_or(min_json, 0.0);
            param_max = json_number_value_or(max_json, 100.0);
            param_step = json_number_value_or(step_json, 0.01);
            default_value = json_number_value_or(default_json, 50.0);
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            obs_data_set_default_double(settings, full_param_name.c_str(), default_value);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            if (is_slider) {
                ui_prop = obs_properties_add_float_slider(props, full_param_name.c_str(), label.c_str(), param_min, param_max, param_step);
            }
            else {
                ui_prop = obs_properties_add_float(props, full_param_name.c_str(), label.c_str(), param_min, param_max, param_step);
            }
            if (!description.empty()) {
                obs_property_set_long_description(ui_prop, obs_module_text(description.c_str()));
            }
        }

        void set_visible(const bool visible) override {
            if (ui_prop != nullptr) {
                obs_property_set_visible(ui_prop, visible);
            }
        }
        [[nodiscard]] bool is_visible() const override {
            return obs_property_visible(ui_prop);
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            //debug("%s", obs_data_get_json(settings));
            *((float*)this->data) = (float)obs_data_get_double(settings, full_param_name.c_str());
            //debug("%s = %f", full_param_name, *((float*)this->data));
        }
};

#endif // SHADERTASTIC_PARAMETER_FLOAT_HPP
