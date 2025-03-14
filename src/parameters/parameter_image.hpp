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
        bool hidden{};

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
            obs_data_set_default_string(metadata, "default", "");
            obs_data_set_default_bool(metadata, "allow_custom", true);
            default_array = obs_data_array_create();
            obs_data_set_default_array(metadata, "values", default_array);

            auto default_file = std::string(obs_data_get_string(metadata, "default"));
            if (!default_file.empty()) {
                default_value = std::string("bundle://") + std::string(effect_path) + '/' + std::string(obs_data_get_string(metadata, "default"));
            }
            else {
                default_value = "";
            }

            allow_custom = obs_data_get_bool(metadata, "allow_custom");
            hidden = obs_data_get_bool(metadata, "hidden");

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
            if (values.empty()) {
                allow_custom = true;
            }
            obs_data_array_release(array);
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            std::string full_param_name_list = std::string(full_param_name) + "__list";
            obs_data_set_default_string(settings, full_param_name, default_value.c_str());
            obs_data_set_default_string(settings, full_param_name_list.c_str(), default_value.c_str());
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            bool with_values = !values.empty();

            // If values are present, show a list widget to select them
            if (with_values) {
                obs_property_t *list_ui = obs_properties_add_list(
                    props,
                    full_param_name,
                    label.c_str(),
                    OBS_COMBO_TYPE_LIST,
                    OBS_COMBO_FORMAT_STRING
                );
                obs_property_set_visible(list_ui, !hidden);

                for (size_t i=0; i < values.size(); ++i) {
                    obs_property_list_add_string(list_ui, values[i].label.c_str(), values[i].value.c_str());
                }
                if (allow_custom) {
                    obs_property_list_add_string(list_ui, "Use custom file", "__CUSTOM__");
                    obs_property_set_modified_callback(list_ui, [](obs_properties_t *props, obs_property_t *property, obs_data_t *settings) {
                        const char *full_param_name = obs_property_name(property);

                        std::string new_value = std::string(obs_data_get_string(settings, full_param_name));

                        std::string full_param_name_filepicker = std::string(full_param_name) + "__custom";
                        obs_property_t *filepicker = obs_properties_get(props, full_param_name_filepicker.c_str());

                        if (new_value == "__CUSTOM__") {
                            obs_property_set_visible(filepicker, true);
                        }
                        else {
                            obs_property_set_visible(filepicker, false);
                        }
                        return true; // TODO maybe a false is okay, to be confirmed
                    });
                }
            }

            // If custom value is allowed, add a file picker widget
            // It will be masked if values are present and the "Select File..." item is not selected
            if (allow_custom) {
                std::string full_param_name_filepicker = std::string(full_param_name) + "__custom";
                obs_properties_add_path(
                    props,
                    with_values ? full_param_name_filepicker.c_str() : full_param_name,
                    with_values ? "∟ Custom file" : label.c_str(),
                    OBS_PATH_FILE,
                    "Image (*.jpg *.jpeg *.png *.bmp)",
                    nullptr
                );
            }

            // TODO description si not user-friendly in the UI now, commented-out for now
            //if (!description.empty()) {
            //    obs_property_set_long_description(filepicker, obs_module_text(description.c_str()));
            //}
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            bool with_values = !values.empty();

            if (with_values) {
                const char *path_ = obs_data_get_string(settings, full_param_name);
                if (strcmp(path_, "__CUSTOM__") == 0) {
                    std::string full_param_name_filepicker = std::string(full_param_name) + "__custom";
                    set_data_from_settings_sub(settings, full_param_name_filepicker.c_str());
                    return;
                }
            }

            set_data_from_settings_sub(settings, full_param_name);
        }

        void set_data_from_settings_sub(obs_data_t *settings, const char *full_param_name) {
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
