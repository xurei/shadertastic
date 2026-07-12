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

#ifndef SHADERTASTIC_PARAMETER_COLOR_ALPHA_HPP
#define SHADERTASTIC_PARAMETER_COLOR_ALPHA_HPP

#include <string>
#include "parameter.hpp"

class effect_parameter_color_alpha : public effect_parameter {
    private:
        uint32_t default_value = (uint32_t)0xFF000000;
        vec4 selected_color{};
        obs_property_t *ui_prop{nullptr};

        // Function to convert an RGBA string to an integer
        static int rgba_string_to_int(std::string rgba) {
            if (rgba.size() == 7) {
                rgba = std::string("#FF"+rgba.substr(1));
            }

            // Check if the input string is in the correct format
            if (rgba.size() != 9 || rgba[0] != '#' || rgba.find_first_not_of("0123456789ABCDEFabcdef", 1) != std::string::npos) {
                log_error("Invalid color string '%s'. Accepted formats are #RRGGBB and #AARRGGBB", rgba.c_str());
                return (int)0xFF000000; // Return the defaut color
            }

            // Extract the hexadecimal values for R, G, B, and A
            int a = (int) std::strtol(rgba.substr(1, 2).c_str(), nullptr, 16);
            int r = (int) std::strtol(rgba.substr(3, 2).c_str(), nullptr, 16);
            int g = (int) std::strtol(rgba.substr(5, 2).c_str(), nullptr, 16);
            int b = (int) std::strtol(rgba.substr(7, 2).c_str(), nullptr, 16);

            // Combine the values into a single integer
            int result = (a << 24) | (b << 16) | (g << 8) | r;
            return result;
        }

    public:
        explicit effect_parameter_color_alpha(gs_eparam_t *shader_param) : effect_parameter(sizeof(vec4), shader_param) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_COLOR_ALPHA;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            json_t *default_json = json_object_get(metadata, "default");
            const char *default_str = json_is_string(default_json) ? json_string_value(default_json) : "#FF000000";
            default_value = rgba_string_to_int(std::string(default_str));
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            obs_data_set_default_int(settings, full_param_name.c_str(), default_value);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            ui_prop = obs_properties_add_color_alpha(props, full_param_name.c_str(), label.c_str());
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
            vec4_from_rgba(&this->selected_color, (uint32_t)obs_data_get_int(settings, full_param_name.c_str()));
        }

        void try_gs_set_val() override {
            *((vec4*) this->data) = selected_color;
            if (gs_get_color_space() != GS_CS_SRGB) {
                gs_float3_srgb_nonlinear_to_linear((float*) this->data);
            }
            effect_parameter::try_gs_set_val();
        }
};

#endif // SHADERTASTIC_PARAMETER_COLOR_ALPHA_HPP
