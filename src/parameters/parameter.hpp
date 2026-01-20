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

#ifndef SHADERTASTIC_PARAMETER_HPP
#define SHADERTASTIC_PARAMETER_HPP

#include "parameter_datatype.hpp"
#include "../try_gs_effect_set.h"
#include "../shadertastic_common.hpp"
#include "../shader/shader.h"

static std::string get_full_param_name_static(const std::string &effect_name, const std::string &param_name) {
    return effect_name + '.' + param_name;
}

static std::string get_full_subparam_name_static(const std::string &effect_name, const std::string &param_name) {
    return effect_name + '_' + param_name;
}

class effect_parameter {
    protected:
        void *data;
        gs_eparam_t *shader_param;
        std::string name;
        std::string label;
        bool devmode{};
        std::string description;
        size_t data_size;

    public:
        /**
         * Single variable constructor. This is the default. It will initialize memory for one fixed-size
         * data type, typically int or float.
         * @param data_size the size of the data
         * @param shader_param
         */
        effect_parameter(const size_t data_size, gs_eparam_t *shader_param) {
            this->data_size = data_size;
            this->shader_param = shader_param;
            this->data = bmalloc(data_size);
        }

        /**
         * Multi variables constructor. This one will not initialize any memory, it's up to the
         * child class to handle its own shader variables
         */
        explicit effect_parameter(const effect_shader *shader) {
            UNUSED_PARAMETER(shader);
            /* no-op by default */
            this->data_size = 0;
            this->shader_param = nullptr;
            this->data = nullptr;
        }

        virtual ~effect_parameter() {
            if (this->data != nullptr) {
                bfree(this->data);
            }
        }

        virtual effect_param_datatype type() = 0;

        [[nodiscard]] bool is_dev_mode() const {
            return devmode;
        }

        void load_common_fields(obs_data_t *metadata) {
            name = std::string(obs_data_get_string(metadata, "name"));
            label = std::string(obs_data_get_string(metadata, "label"));
            const char *description_c_str = obs_data_get_string(metadata, "description");
            if (description_c_str != nullptr) {
                description = std::string(description_c_str);
            }
            else {
                description = std::string("");
            }

            obs_data_set_default_bool(metadata, "devmode", false);
            devmode = obs_data_get_bool(metadata, "devmode");
        }

        virtual void tick(shadertastic_common *s) {
            UNUSED_PARAMETER(s);
            /* no-op by default */
        }

        /**
         * Called in the factory, after creation of the parameter.
         * This is where you should set the attributes of the parameter and their default values.
         *
         * BE CAREFUL :
         *   they are NOT the default values that you see in the meta files,
         *   but the default values of the attributes a parameter must have.
         *   For example, a `int` parameter has a boolean "slider" attribute,
         *   to display a slider in the UI. By default it's not active.
         *   Hence, the default value of "slider" is false if it's
         *   not otherwise mentioned in the metadata.
         * @param shader
         * @param metadata
         * @param effect_path
         */
        virtual void initialize_params(const shadertastic_effect_t *effect, const effect_shader *shader, obs_data_t *metadata, const std::string &effect_path) = 0;

        /**
         * This is where you should set the defaults as explicitly specified in the metadata.
         * See also initialize_params for details about the defaults
         * @param settings
         * @param full_param_name
         */
        virtual void set_default(obs_data_t *settings, const char *full_param_name) = 0;

        /**
         * Renders the UI in the OBS view
         * @param effect_name
         * @param props
         */
        virtual void render_property_ui(const char *effect_name, obs_properties_t *props) = 0;

        /**
         * Update function of the parameter, will be called when a filter is loaded or when the
         * value is changed through the UI or an OBS internal call.
         * @param settings
         * @param full_param_name
         */
        virtual void set_data_from_settings(obs_data_t *settings, const char *full_param_name) = 0;

        [[nodiscard]] std::string get_name() {
            return name;
        }

        [[nodiscard]] std::string get_full_param_name(const char *effect_name) const {
            const std::string effect_name_str = std::string(effect_name);
            return get_full_param_name(effect_name_str);
        }
        [[nodiscard]] std::string get_full_param_name(const std::string &effect_name) const {
            return get_full_param_name_static(effect_name, this->name);
        }

        [[nodiscard]] size_t get_data_size() const {
            return data_size;
        }
        [[nodiscard]] void * get_data() const {
            return data;
        }

        /**
         * Send the data to the matching fields in the HLSL shader.
         */
        virtual void try_gs_set_val() {
            try_gs_effect_set_val(name.c_str(), shader_param, data, data_size);
        }
        /**
         * Check if the parameter needs to be re-rendered
         */
        virtual bool should_reload() {
            return false;
        }

        /**
         * Do something if the effect containing this parameter is shown
         */
        virtual void show() {
            // By default, do nothing
        }

        /**
         * Do something if the effect containing this parameter is hidden
         */
        virtual void hide() {
            // By default, do nothing
        }
};

#endif // SHADERTASTIC_PARAMETER_HPP
