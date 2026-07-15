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

#include <jansson.h>
#include "parameter_datatype.hpp"
#include "src/condition/collect_refs.h"
#include "src/condition/condition_parser.h"
#include "src/shader/shader.h"
#include "src/shadertastic_common.hpp"
#include "src/try_gs_effect_set.h"

inline static std::string get_full_param_name_static(const std::string &effect_name, const std::string &param_name) {
    return effect_name + '.' + param_name;
}

inline static std::string get_full_subparam_name_static(const std::string &parent_param_name, const std::string &param_name) {
    return parent_param_name + '_' + param_name;
}

inline static double json_number_value_or(json_t *value, double default_value) {
    if (json_is_real(value)) {
        return json_real_value(value);
    }
    if (json_is_integer(value)) {
        return (double)json_integer_value(value);
    }
    return default_value;
}

class effect_parameter {
    private:
        std::unique_ptr<condition_t> condition{};

    protected:
        void *data{};
        gs_eparam_t *shader_param{};
        std::string name{};
        std::string label{};
        bool devmode{};
        std::string description{};
        size_t data_size{};
        bool visible{true};

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
            condition.reset(nullptr);
            if (this->data != nullptr) {
                bfree(this->data);
            }
        }

        [[nodiscard]] virtual effect_param_datatype type() const = 0;

        [[nodiscard]] bool is_dev_mode() const {
            return devmode;
        }

        void load_common_fields(const std::string &effect_name, json_t *metadata) {
            json_t *name_json = json_object_get(metadata, "name");
            if (json_is_string(name_json)) {
                name = json_string_value(name_json);
            }
            else {
                name.clear();
            }

            json_t *label_json = json_object_get(metadata, "label");
            if (json_is_string(label_json)) {
                label = json_string_value(label_json);
            }
            else {
                label.clear();
            }

            json_t *description_json = json_object_get(metadata, "description");
            if (json_is_string(description_json)) {
                description = json_string_value(description_json);
            }
            else {
                description.clear();
            }

            json_t *devmode_json = json_object_get(metadata, "devmode");
            devmode = json_is_boolean(devmode_json) ? json_boolean_value(devmode_json) : false;

            json_t *if_param = json_object_get(metadata, "if");
            condition = parse_condition(effect_name, if_param);

            if (condition != nullptr) {
                debug("if node found");
            }
        }

        virtual void tick(shadertastic_common *s) {
            UNUSED_PARAMETER(s);
            /* no-op by default */
        }

        /**
         * Called in the factory, after creation of the parameter.
         * This is where you should set the attributes of the parameter and their absent values.
         *
         * BE CAREFUL :
         *   they are NOT the default values that you see in the meta files,
         *   but the value that will be used if the default attribute is not set.
         *   For example, a `int` parameter has a boolean "slider" attribute,
         *   to display a slider in the UI. By default it's not active.
         *   Hence, the absent value of "slider" is false if it's
         *   not otherwise mentioned in the metadata.
         * @param shader
         * @param metadata
         * @param effect_path
         */
        virtual void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) = 0;

        /**
         * This is where you should set the defaults as explicitly specified in the metadata.
         * See also initialize_params for details about the defaults
         * @param settings
         * @param full_param_name
         */
        virtual void set_default(obs_data_t *settings, const char *effect_name) = 0;

        /**
         * Renders the UI in the OBS view
         * @param effect_name
         * @param props
         */
        virtual void render_property_ui(const char *effect_name, obs_properties_t *props) = 0;

        /**
         * Set visibility of the parameter inputs the UI in the OBS view
         * @param visible
         */
        inline void set_visible(const bool visible_) {
            visible = visible_;
        }

        /**
         * Apply visibility to the UI through its properties
         * @param visible
         */
        virtual void apply_visible(const char *effect_name, obs_properties_t *props, bool is_visible) = 0;

        /**
         * Get visibility of the parameter inputs the UI in the OBS view
         * @param visible
         */
        inline bool is_visible() const {
            return visible;
        }

        /**
         * Update function of the parameter, will be called when a filter is loaded or when the
         * effect settings are changed through the UI or an OBS internal call.
         * This function should update the internal state of the parameter to reflect any value change for its given settings.
         * @param settings
         * @param full_param_name
         */
        virtual void set_data_from_settings(obs_data_t *settings, const char *effect_name) = 0;

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

        /**
         * @return true if the parameter has a condition
         */
        inline bool has_condition() {
            return condition != nullptr;
        }

        /**
         * Populates out with the referenced names in a condition
         */
        void collect_refs_from_condition(std::vector<std::string> &out) {
            collect_refs_from_condition_static(condition.get(), out);
        }

        inline bool condition_check(obs_data_t *settings) {
            return condition == nullptr || condition->check(settings);
        }
};

template<typename Param>
class effect_param_with_no_prop : public Param {
    public:
        using Param::Param;

        [[maybe_unused]] void apply_visible(const char *effect_name, obs_properties_t *props, const bool visible) override {
            UNUSED_PARAMETER(effect_name);
            UNUSED_PARAMETER(props);
            UNUSED_PARAMETER(visible);
            /* Nothing to do */
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            UNUSED_PARAMETER(effect_name);
            UNUSED_PARAMETER(props);
            /* Automatic parameter, no UI */
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
        }
};

template<typename Param>
class effect_param_with_unique_prop : public Param {
    public:
        using Param::Param;

        [[maybe_unused]] void apply_visible(const char *effect_name, obs_properties_t *props, const bool visible) override {
            std::string full_param_name = Param::get_full_param_name(effect_name);
            auto *ui_prop = obs_properties_get(props, full_param_name.c_str());
            if (ui_prop != nullptr) {
                obs_property_set_visible(ui_prop, visible);
            }
        }
};


#endif // SHADERTASTIC_PARAMETER_HPP
