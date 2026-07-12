/******************************************************************************
    Copyright (C) 2025 by xurei <xureilab@gmail.com>

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

#ifndef SHADERTASTIC_PARAMETER_TIME_HPP
#define SHADERTASTIC_PARAMETER_TIME_HPP

#include <string>
#include "parameter.hpp"

enum effect_parameter_time_reset_t {
    CHOICE_NO = 0,
    CHOICE_YES = 1,
    CHOICE_PROMPT = 2,
};

class effect_parameter_time : public effect_parameter {
    private:
        effect_parameter_time_reset_t reset_on_show_type{CHOICE_NO};
        bool reset_on_show{false};
        bool show_speed_ui{true};
        float *time;
        float speed;
        float min_speed;
        float max_speed;
        float default_speed;
        std::string speed_label;
        obs_property_t *ui_speed_prop{nullptr};
        obs_property_t *ui_reset_prop{nullptr};

    public:
        explicit effect_parameter_time(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param), time((float*)this->data) {
            *time = 0.0;
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_TIME;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            json_t *reset_on_show_json = json_object_get(metadata, "reset_on_show");
            if (json_is_string(reset_on_show_json) && strcmp(json_string_value(reset_on_show_json), "prompt") == 0) {
                reset_on_show_type = CHOICE_PROMPT;
            }
            else {
                reset_on_show = json_is_boolean(reset_on_show_json) ? json_boolean_value(reset_on_show_json) : false;
                reset_on_show_type = reset_on_show ? CHOICE_YES : CHOICE_NO;
            }

            json_t *speed_obj = json_object_get(metadata, "speed");
            if (json_is_object(speed_obj)) {
                json_t *show_ui_json = json_object_get(speed_obj, "show_ui");
                json_t *label_json = json_object_get(speed_obj, "label");
                json_t *min_json = json_object_get(speed_obj, "min");
                json_t *max_json = json_object_get(speed_obj, "max");
                json_t *default_json = json_object_get(speed_obj, "default");

                min_speed = (float)json_number_value_or(min_json, 0.0);
                max_speed = (float)json_number_value_or(max_json, 1.0);
                default_speed = (float)json_number_value_or(default_json, 1.0);
                show_speed_ui = json_is_boolean(show_ui_json) ? json_boolean_value(show_ui_json) : true;
                speed_label = json_is_string(label_json) ? json_string_value(label_json) : "Speed";
            }
            else {
                min_speed = 0.0f;
                max_speed = 1.0f;
                default_speed = 1.0f;
                show_speed_ui = true;
                speed_label = "Speed";
            }

            // Set time to zero at init time
            *time = 0.0;
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            obs_data_set_default_double(settings, get_full_subparam_name_static(full_param_name, std::string("speed")).c_str(), default_speed);
            obs_data_set_default_bool(settings, get_full_subparam_name_static(full_param_name, std::string("reset_on_show")).c_str(), false);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            if (show_speed_ui) {
                ui_speed_prop = obs_properties_add_float_slider(props, get_full_subparam_name_static(full_param_name, "speed").c_str(), speed_label.c_str(), min_speed, max_speed, 0.001);
            }
            if (reset_on_show_type == CHOICE_PROMPT) {
                ui_reset_prop = obs_properties_add_list(
                    props, get_full_subparam_name_static(full_param_name, "reset_on_show").c_str(), "On filter visibility toggle:",
                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_BOOL
                );
                obs_property_list_add_bool(ui_reset_prop, "Do nothing", false);
                obs_property_list_add_bool(ui_reset_prop, "Reset time to zero", true);
            }
        }

        void set_visible(const bool visible) override {
            if (ui_speed_prop != nullptr) {
                obs_property_set_visible(ui_speed_prop, visible);
            }
            if (ui_reset_prop != nullptr) {
                obs_property_set_visible(ui_reset_prop, visible);
            }
        }
        [[nodiscard]] virtual bool is_visible() const override {
        [[nodiscard]] bool is_visible() const override {
            return obs_property_visible(ui_speed_prop) || obs_property_visible(ui_reset_prop);
        }

        void tick(shadertastic_common *s) override {
            uint64_t frame_interval = obs_get_frame_interval_ns();
            (*time) += (
                // Converting to double first before reconverting to float to keep precision. Might be useless
                speed < 0.0001 ? 0.0f : (float)(((double)frame_interval/1000000000.0) * speed)
            );
            if (reset_on_show && !s->was_enabled) {
                *time = 0.0;
            }
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            if (reset_on_show_type == CHOICE_PROMPT) {
                reset_on_show = obs_data_get_bool(settings, get_full_subparam_name_static(full_param_name, "reset_on_show").c_str());
            }
            speed = (float)obs_data_get_double(settings, get_full_subparam_name_static(full_param_name, "speed").c_str());
        }
};

#endif // SHADERTASTIC_PARAMETER_TIME_HPP
