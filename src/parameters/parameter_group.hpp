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

#ifndef SHADERTASTIC_PARAMETER_GROUP_HPP
#define SHADERTASTIC_PARAMETER_GROUP_HPP

#include <string>
#include <variant>
#include "parameter.hpp"
#include "src/condition/collect_strings.h"
#include "src/condition/condition_parser.h"
#include "src/effect.h"
#include "src/params_list.hpp"
#include "src/settings.h"
#include "src/util/obs_property_add_modified_callback2.h"
#include "parameter_factory.h"

class effect_parameter_group : public effect_parameter {
    private:
        double value{};
        params_list effect_params;
        obs_property_t *ui_prop{nullptr};

    public:
        explicit effect_parameter_group(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        ~effect_parameter_group() override {
            for (auto param: effect_params) {
                delete param;
            }
            effect_params.clear();
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_GROUP;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            json_t *parameters = json_object_get(metadata, "parameters");
            if (!json_is_array(parameters)) {
                warn("No parameters specified for effect %s", name.c_str());
            }

            // Copy the effect params map to allow recycling
            params_list previous_effect_params(effect_params);

            effect_params.clear();

            size_t nb_parameters = json_is_array(parameters) ? json_array_size(parameters) : 0;

            for (size_t i=0; i < nb_parameters; i++) {
                json_t *param_metadata = json_array_get(parameters, i);
                if (!json_is_object(param_metadata)) {
                    continue;
                }
                effect_parameter *effect_param = effect_parameter_factory::create(name, effect_path, shader, param_metadata);

                if (effect_param != nullptr) {
                    std::string param_name_str;
                    json_t *name_json = json_object_get(param_metadata, "name");
                    if (json_is_string(name_json)) {
                        param_name_str = json_string_value(name_json);
                    }
                    effect_parameter *previous_param = previous_effect_params.get(param_name_str);
                    if (previous_param != nullptr) {
                        if (previous_param->get_data_size() == effect_param->get_data_size()) {
                            debug("Recycling data for %s (size: %i)", param_name_str.c_str(), (int)effect_param->get_data_size());
                            memcpy(effect_param->get_data(), previous_param->get_data(), effect_param->get_data_size());
                        }
                    }

                    effect_params.put(param_name_str, effect_param);
                }
            }

            // Clear memory of removed params
            for (auto param: previous_effect_params) {
                debug ("Free removed param %s", param->get_name().c_str());
                delete param;
            }
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            for (auto param: effect_params) {
                param->set_default(settings, effect_name);
            }
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            obs_properties_t *params_group = obs_properties_create();

            for (auto param: effect_params) {
                if (!param->is_dev_mode() || shadertastic_settings().dev_mode_enabled) {
                    std::string sub_full_param_name = param->get_full_param_name(full_param_name);
                    param->render_property_ui(effect_name, params_group);
                }
            }

            ui_prop = obs_properties_add_group(props, full_param_name.c_str(), label.c_str(), OBS_GROUP_NORMAL, params_group);
            std::string param_val1 = get_full_param_name_static(effect_name, "val1");
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
            for (auto param: effect_params) {
                param->set_data_from_settings(settings, effect_name);
            }
        }

        void tick(shadertastic_common *s) override {
            for (auto param: effect_params) {
                param->tick(s);
            }
        }

        void try_gs_set_val() override {
            for (auto param: effect_params) {
                param->try_gs_set_val();
            }
        }
};

#endif // SHADERTASTIC_PARAMETER_GROUP_HPP
