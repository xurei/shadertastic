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

#include "../util/string_util.h"
#include "../util/file_util.h"

struct effect_parameter_image_value {
    std::string label;
    std::string value{};
};

class effect_parameter_image : public effect_parameter {
    private:
        std::string default_value;
        std::vector<effect_parameter_image_value> values;
        obs_data_array *default_array{};
        bool allow_custom{};

        std::string path;
        gs_texture_t * texture;

        void load_texture() {
            obs_enter_graphics();
            if (this->texture != nullptr) {
                gs_texture_destroy(this->texture);
            }

            if (starts_with(path, "bundle://")) {
                std::string temp_file_path = create_temp_file();
                extract_file_zipped_or_local(path.substr(9), temp_file_path);
                this->texture = gs_texture_create_from_file(temp_file_path.c_str());
                std::remove(temp_file_path.c_str());
            }
            else {
                this->texture = gs_texture_create_from_file(path.c_str());
            }
            obs_leave_graphics();
        }

    public:
        explicit effect_parameter_image(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param) {
            this->texture = nullptr;
        }

        ~effect_parameter_image() override {
            obs_data_array_release(default_array);

            if (this->texture != nullptr) {
                obs_enter_graphics();
                gs_texture_destroy(this->texture);
                this->texture = nullptr;
                obs_leave_graphics();
            }
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_IMAGE;
        }

        void initialize_params(obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(effect_path);

            obs_data_set_default_string(metadata, "default", "");
            obs_data_set_default_bool(metadata, "allow_custom", true);
            default_array = obs_data_array_create();
            obs_data_set_default_array(metadata, "values", default_array);

            default_value = std::string(obs_data_get_string(metadata, "default"));
            allow_custom = obs_data_get_bool(metadata, "allow_custom");

            obs_data_array_t *array = obs_data_get_array(metadata, "values");
            size_t array_count = obs_data_array_count(array);
            values.resize(array_count);
            for (size_t i=0; i<array_count; ++i) {
                obs_data_t *item = obs_data_array_item(array, i);
                values[i].label = std::string(obs_data_get_string(item, "label"));
                const char *val = obs_data_get_string(item, "value");
                if (val == nullptr || strlen(val) == 0) {
                    values[i].value = std::string("");
                }
                else {
                    values[i].value = std::string("bundle://") + std::string(effect_path) + '/' + obs_data_get_string(item, "value");
                }
                obs_data_release(item);
            }
            obs_data_array_release(array);
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(full_param_name);
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            if (!values.empty()) {
                obs_property_t *list_ui = obs_properties_add_list(
                    props, full_param_name, label.c_str(),
                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING
                );
                for (size_t i=0; i < values.size(); ++i) {
                    obs_property_list_add_string(list_ui, values[i].label.c_str(), values[i].value.c_str());
                }
            }

            if (allow_custom) {
                // Custom path
                auto prop = obs_properties_add_path(props, full_param_name, label.c_str(), OBS_PATH_FILE, "Image (*.jpg *.jpeg *.png *.bmp)", nullptr);
//                obs_property_set_modified_callback2(prop, [](void *priv, obs_properties_t *props, obs_property_t *property, obs_data_t *settings) {
//                    UNUSED_PARAMETER(props);
//                    effect_parameter_image *this_ = (effect_parameter_image *)priv;
//                    const char *full_param_name = obs_property_name(property);
//                    const char *value = obs_data_get_string(settings, full_param_name);
//                    if (!value) {
//                        this_->is_custom = false;
//                    }
//                    else {
//                        this_->is_custom = true;
//                    }
//                    return true; // TODO maybe a false is okay, to be confirmed
//                }, this);

                if (!description.empty()) {
                    obs_property_set_long_description(prop, obs_module_text(description.c_str()));
                }
            }
        }

        void set_data_from_default() override {
            this->path = default_value;
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            const char *path_ = obs_data_get_string(settings, full_param_name);
            if (path_ != nullptr) {
                std::string path_str = std::string(path_);
                if (this->path != path_str) {
                    this->path = path_str;
                    this->load_texture();
                }
            }
            else {
                if (this->texture != nullptr) {
                    obs_enter_graphics();
                    gs_texture_destroy(this->texture);
                    this->texture = nullptr;
                    obs_leave_graphics();
                    this->path = "";
                }
            }
        }

        void try_gs_set_val() override {
            try_gs_effect_set_texture(name.c_str(), shader_param, this->texture);
        }
};
