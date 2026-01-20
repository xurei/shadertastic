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

    public:
        explicit effect_parameter_time(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param), time((float*)this->data) {
            *time = 0.0;
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_TIME;
        }

        void initialize_params(const effect_shader *shader, obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            // reset_on_show field
            obs_data_set_default_bool(metadata, "reset_on_show", false);
            const char *reset_on_show_str = obs_data_get_string(metadata, "reset_on_show");
            if (reset_on_show_str && strcmp(reset_on_show_str, "prompt") == 0) {
                reset_on_show_type = CHOICE_PROMPT;
            }
            else {
                reset_on_show = obs_data_get_bool(metadata, "reset_on_show");
                reset_on_show_type = reset_on_show ? CHOICE_YES : CHOICE_NO;
            }

            // speed field
            obs_data_t *speed_obj = obs_data_create();
            obs_data_set_default_obj(metadata, "speed", speed_obj);
            obs_data_release(speed_obj);

            speed_obj = obs_data_get_obj(metadata, "speed");
            obs_data_set_default_bool(speed_obj, "show_ui", true);
            obs_data_set_default_string(speed_obj, "label", "Speed");
            obs_data_set_default_double(speed_obj, "min", 0.0);
            obs_data_set_default_double(speed_obj, "max", 1.0);
            obs_data_set_default_double(speed_obj, "default", 1.0);
            min_speed = (float)obs_data_get_double(speed_obj, "min");
            max_speed = (float)obs_data_get_double(speed_obj, "max");
            default_speed = (float)obs_data_get_double(speed_obj, "default");
            show_speed_ui = obs_data_get_bool(speed_obj, "show_ui");
            speed_label = std::string(obs_data_get_string(speed_obj, "label"));
            obs_data_release(speed_obj);

            // Set time to zero at init time
            *time = 0.0;
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            obs_data_set_default_double(settings, get_full_subparam_name_static(full_param_name, std::string("speed")).c_str(), default_speed);
            obs_data_set_default_bool(settings, get_full_subparam_name_static(full_param_name, std::string("reset_on_show")).c_str(), false);
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            if (show_speed_ui) {
                obs_properties_add_float_slider(props, get_full_subparam_name_static(full_param_name, "speed").c_str(), speed_label.c_str(),
                    min_speed, max_speed, 0.001);
            }
            if (reset_on_show_type == CHOICE_PROMPT) {
                obs_property_t *list_ui = obs_properties_add_list(
                    props, get_full_subparam_name_static(full_param_name, "reset_on_show").c_str(), "On filter visibility toggle:",
                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_BOOL
                );
                obs_property_list_add_bool(list_ui, "Do nothing", false);
                obs_property_list_add_bool(list_ui, "Reset time to zero", true);
            }
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

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            if (reset_on_show_type == CHOICE_PROMPT) {
                reset_on_show = obs_data_get_bool(settings, get_full_subparam_name_static(full_param_name, "reset_on_show").c_str());
            }
            speed = (float)obs_data_get_double(settings, get_full_subparam_name_static(full_param_name, "speed").c_str());
        }
};

#endif // SHADERTASTIC_PARAMETER_TIME_HPP
