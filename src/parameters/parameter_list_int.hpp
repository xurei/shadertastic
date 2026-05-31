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

#ifndef SHADERTASTIC_PARAMETER_LIST_INT_HPP
#define SHADERTASTIC_PARAMETER_LIST_INT_HPP

#include <vector>
#include <string>
#include "parameter.hpp"

struct effect_parameter_list_int_value {
    std::string label;
    int value{};
};

class effect_parameter_list_int : public effect_parameter {
    private:
        int default_value{};
        std::vector<effect_parameter_list_int_value> values;
        obs_property_t *ui_prop{nullptr};

    public:
        explicit effect_parameter_list_int(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_LIST_INT;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            json_t *default_json = json_object_get(metadata, "default");
            default_value = json_is_integer(default_json) ? (int)json_integer_value(default_json) : 0;

            json_t *array = json_object_get(metadata, "values");
            size_t array_count = json_is_array(array) ? json_array_size(array) : 0;
            values.resize(array_count);
            for (size_t i=0; i<array_count; ++i) {
                json_t *item = json_array_get(array, i);
                json_t *label_json = json_object_get(item, "label");
                json_t *value_json = json_object_get(item, "value");
                values[i].label = json_is_string(label_json) ? json_string_value(label_json) : "";
                values[i].value = json_is_integer(value_json) ? (int)json_integer_value(value_json) : 0;
            }
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            obs_data_set_default_int(settings, full_param_name, default_value);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            ui_prop = obs_properties_add_list(
                props, full_param_name.c_str(), label.c_str(),
                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
            );
            for (size_t i=0; i < values.size(); ++i) {
                obs_property_list_add_int(ui_prop, values[i].label.c_str(), values[i].value);
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
        [[nodiscard]] virtual bool is_visible() const override {
            return obs_property_visible(ui_prop);
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            *((int*)this->data) = (int)obs_data_get_int(settings, full_param_name.c_str());
            //debug("%s = %d", full_param_name, *((int*)this->data));
        }
};

#endif // SHADERTASTIC_PARAMETER_LIST_INT_HPP
