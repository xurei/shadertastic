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
#include "parameter_factory.h"
#include "parameter_group_condition.hpp"
#include "parameter_group_if.hpp"
#include "../params_list.hpp"
#include "../settings.h"
#include "../effect.h"

class effect_parameter_group : public effect_parameter {
    private:
        std::string param_name{};
        effect_parameter* param{};
        effect_parameter_group_condition condition{};
        effect_parameter_group_if if_mode{};
        double value{};
        params_list effect_params;
        bool condition_met{};

        effect_parameter_group_if parse_if(const char *if_str) {
            if (strcmp(if_str, "bool") == 0) {
                return PARAM_GROUP_IF_BOOL;
            } else if (strcmp(if_str, "int") == 0) {
                return PARAM_GROUP_IF_INT;
            } else if (strcmp(if_str, "float") == 0) {
                return PARAM_GROUP_IF_FLOAT;
            } else {
                return PARAM_GROUP_IF_UNKNOWN;
            }
        }
        effect_parameter_group_condition parse_condition(const char *condition_str) {
            if (strcmp(condition_str, "not") == 0) {
                return PARAM_GROUP_CONDITION_NOT_EQUAL;
            } else if (strcmp(condition_str, "greater") == 0) {
                return PARAM_GROUP_CONDITION_GREATER;
            } else if (strcmp(condition_str, "greater_equal") == 0) {
                return PARAM_GROUP_CONDITION_GREATER_OR_EQUAL;
            } else if (strcmp(condition_str, "less") == 0) {
                return PARAM_GROUP_CONDITION_LESS;
            } else if (strcmp(condition_str, "less_equal") == 0) {
                return PARAM_GROUP_CONDITION_LESS_OR_EQUAL;
            } else {
                return PARAM_GROUP_CONDITION_EQUAL;
            }
        }

        bool check_condition() {
            if (param) {
                void* param_value_ptr = param->get_data();
                double param_value = -1;
                switch (if_mode) {
                    case PARAM_GROUP_IF_BOOL:
                        param_value = *(bool*)param_value_ptr ? 1 : 0;
                        break;
                    case PARAM_GROUP_IF_INT:
                        param_value = (double)*(int*)param_value_ptr;
                        break;
                    case PARAM_GROUP_IF_FLOAT:
                        param_value = *(double*)param_value_ptr;
                        break;
                }
                switch (condition) {
                    case PARAM_GROUP_CONDITION_EQUAL:
                        return param_value == value;
                        break;
                    case PARAM_GROUP_CONDITION_NOT_EQUAL:
                        return param_value != value;
                        break;
                    case PARAM_GROUP_CONDITION_GREATER:
                        return param_value > value;
                        break;
                    case PARAM_GROUP_CONDITION_GREATER_OR_EQUAL:
                        return param_value >= value;
                        break;
                    case PARAM_GROUP_CONDITION_LESS:
                        return param_value < value;
                        break;
                    case PARAM_GROUP_CONDITION_LESS_OR_EQUAL:
                        return param_value <= value;
                        break;
                    default:
                        return true;
                        break;
                }
            }

            return true;
        }

    public:
        explicit effect_parameter_group(gs_eparam_t *shader_param) : effect_parameter(sizeof(int), shader_param) {
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_GROUP;
        }

        void initialize_params(const effect_shader *shader, obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);
            obs_data_set_default_string(metadata, "param", "");
            obs_data_set_default_string(metadata, "condition", "");
            obs_data_set_default_string(metadata, "if", "");

            if_mode = parse_if(obs_data_get_string(metadata, "if"));
            if (if_mode != PARAM_GROUP_IF_UNKNOWN) {
                param_name = obs_data_get_string(metadata, "param");
                condition = parse_condition(obs_data_get_string(metadata, "condition"));
                switch (if_mode) {
                    case PARAM_GROUP_IF_BOOL:
                        value = obs_data_get_bool(metadata, "value") ? 1 : 0;
                        break;
                    case PARAM_GROUP_IF_INT:
                        value = (double)obs_data_get_int(metadata, "value");
                        break;
                    case PARAM_GROUP_IF_FLOAT:
                        value = obs_data_get_double(metadata, "value");
                        break;
                }
            }

            obs_data_array_t *parameters = obs_data_get_array(metadata, "parameters");
            if (parameters == nullptr) {
                warn("No parameters specified for effect %s", name.c_str());
                parameters = obs_data_array_create();
            }

            effect_params.clear();

            // Copy the effect params map to allow recycling
            params_list previous_effect_params(effect_params);

            size_t nb_parameters = obs_data_array_count(parameters);

            for (size_t i=0; i < nb_parameters; i++) {
                obs_data_t *param_metadata = obs_data_array_item(parameters, i);
                effect_parameter *effect_param = effect_parameter_factory::create(name, effect_path, shader, param_metadata);


                if (effect_param != nullptr) {
                    auto param_type = effect_param->type();

                    std::string param_name_str = obs_data_get_string(param_metadata, "name");
                    effect_parameter *previous_param = previous_effect_params.get(param_name_str);
                    if (previous_param != nullptr) {
                        if (previous_param->get_data_size() == effect_param->get_data_size()) {
                            debug("Recycling data for %s (size: %i)", param_name_str.c_str(), (int)effect_param->get_data_size());
                            memcpy(effect_param->get_data(), previous_param->get_data(), effect_param->get_data_size());
                        }
                    }

                    effect_params.put(param_name_str, effect_param);
                }

                obs_data_release(param_metadata);
            }

            // Clear memory of removed params
            for (auto param: previous_effect_params) {
                debug ("Free removed param %s", param->get_name().c_str());
                delete param;
            }

            obs_data_array_release(parameters);
        }

        void initialize_params_post(const shadertastic_effect_t *effect, const effect_shader *shader, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);

            param = effect->effect_params.get(param_name);

            for (auto param: effect_params) {
                param->initialize_params_post(effect, shader, effect_path);
            }
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            for (auto param: effect_params) {
                std::string sub_full_param_name = param->get_full_param_name(full_param_name);
                param->set_default(settings, sub_full_param_name.c_str());
            }
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            if (check_condition()) {
                condition_met = true;

                obs_properties_t *params_group = obs_properties_create();

                for (auto param: effect_params) {
                    if (!param->is_dev_mode() || shadertastic_settings().dev_mode_enabled) {
                        std::string sub_full_param_name = param->get_full_param_name(full_param_name);
                        param->render_property_ui(sub_full_param_name.c_str(), params_group);
                    }
                }

                obs_properties_add_group(props, full_param_name, label.c_str(), OBS_GROUP_NORMAL, params_group);
            }
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            //*((int*)this->data) = (int)obs_data_get_int(settings, full_param_name);
            //debug("%s = %d", full_param_name, *((int*)this->data));
            for (auto param: effect_params) {
                std::string sub_full_param_name = param->get_full_param_name(full_param_name);
                param->set_data_from_settings(settings, sub_full_param_name.c_str());
            }
        }


        void try_gs_set_val() override {
            
                for (auto param: effect_params) {
                    param->try_gs_set_val();
                }
        }
        bool should_reload() override {

            if (check_condition() != condition_met) return true;
            
            for (auto param: effect_params) {
                if (param->should_reload()) {
                    return true;
                }
            }

            return false;
        }
        effect_parameter* get_subparam(std::string param_name) override {
            info("Searching subparam %s in group %s", param_name.c_str(), name.c_str());
            return effect_params.get(param_name);
        }
};

#endif // SHADERTASTIC_PARAMETER_GROUP_HPP
