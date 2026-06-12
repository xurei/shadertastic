/******************************************************************************
    Copyright (C) 2024 by xurei <xureilab@gmail.com>

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

#ifndef SHADERTASTIC_PARAMETER_TEXT_HPP
#define SHADERTASTIC_PARAMETER_TEXT_HPP

#include <string>
#include "parameter.hpp"

class effect_parameter_text : public effect_parameter {
    private:
        obs_property_t *ui_prop{nullptr};
    public:
        std::string value;

        explicit effect_parameter_text(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_TEXT;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            json_t *value_json = json_object_get(metadata, "value");
            value = json_is_string(value_json) ? json_string_value(value_json) : "";
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            ui_prop = obs_properties_add_text(
                props,
                full_param_name.c_str(),
                value.c_str(),
                OBS_TEXT_INFO
            );
        }

        void set_visible(const bool visible) override {
            if (ui_prop != nullptr) {
                obs_property_set_visible(ui_prop, visible);
            }
        }
        [[nodiscard]] virtual bool is_visible() const override {
            return obs_property_visible(ui_prop);
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
            *((int*)this->data) = 0;
        }
};

#endif // SHADERTASTIC_PARAMETER_TEXT_HPP
