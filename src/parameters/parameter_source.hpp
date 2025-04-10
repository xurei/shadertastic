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

#include "../util/compare_nocase.hpp"

static bool effect_parameter_source_add(void *data, obs_source_t *source) {
    std::list<std::string> *sources_list = (std::list<std::string>*)(data);

    uint32_t flags = obs_source_get_output_flags(source);
    obs_source_type type = obs_source_get_type(source);

    if ((flags & OBS_SOURCE_VIDEO) && ((type == OBS_SOURCE_TYPE_INPUT) || (type == OBS_SOURCE_TYPE_SCENE))) {
        const char *name = obs_source_get_name(source);
        if (name != nullptr) {
            sources_list->push_back(std::string(name));
        }
    }
    return true;
}

class effect_parameter_source : public effect_parameter {
    private:
        gs_texrender_t *source_texrender = nullptr;
        obs_weak_source_t *source = nullptr;
        struct vec4 clear_color{0,0,0,0};

    public:
        explicit effect_parameter_source(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param) {
            this->source_texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
        }

        ~effect_parameter_source() override {
            if (this->source != nullptr) {
                obs_weak_source_release(this->source);
                this->source = nullptr;
            }
            if (this->source_texrender != nullptr) {
                obs_enter_graphics();
                gs_texrender_destroy(this->source_texrender);
                this->source_texrender = nullptr;
                obs_leave_graphics();
            }
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_SOURCE;
        }

        void initialize_params(obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(metadata);
            UNUSED_PARAMETER(effect_path);
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(full_param_name);
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            obs_property_t *p = obs_properties_add_list(props, full_param_name, label.c_str(), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
            std::list<std::string> sources_list;
            obs_enum_sources(effect_parameter_source_add, &sources_list);
            obs_enum_scenes(effect_parameter_source_add, &sources_list);
            sources_list.sort(compare_nocase);
            for (const std::string &str: sources_list) {
                obs_property_list_add_string(p, str.c_str(), str.c_str());
            }
            if (!description.empty()) {
                obs_property_set_long_description(p, obs_module_text(description.c_str()));
            }
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            if (this->source != nullptr) {
                this->hide();
                #ifdef DEV_MODE
                    obs_source_t *ref_source2 = obs_weak_source_get_source(this->source);
                    debug("Release source %s", obs_source_get_name(ref_source2));
                    obs_source_release(ref_source2);
                #endif
                obs_weak_source_release(this->source);
                this->source = nullptr;
            }

            obs_source_t *ref_source = obs_get_source_by_name(obs_data_get_string(settings, full_param_name));
            if (ref_source != nullptr) {
                this->source = obs_source_get_weak_source(ref_source);
                obs_source_release(ref_source);
            }

            if (this->source != nullptr) {
                debug("Acquired source %s", obs_data_get_string(settings, full_param_name));
                this->show();
            }
            else {
                debug("Cannot Acquire source %s", obs_data_get_string(settings, full_param_name));
            }
        }

        void try_gs_set_val() override {
            if (this->source != nullptr) {
                gs_texrender_reset(this->source_texrender);
                obs_source_t *ref_source = obs_weak_source_get_source(this->source);
                if (ref_source != nullptr) {
                    uint32_t cx = obs_source_get_width(ref_source);
                    uint32_t cy = obs_source_get_height(ref_source);
                    if (gs_texrender_begin(this->source_texrender, cx, cy)) {
                        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
                        gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f); // This line took me A WHOLE WEEK to figure out
                        obs_source_video_render(ref_source);
                        gs_texrender_end(this->source_texrender);
                        gs_texture_t *texture = gs_texrender_get_texture(this->source_texrender);
                        try_gs_effect_set_texture(name.c_str(), shader_param, texture);
                    }
                    obs_source_release(ref_source);
                }
            }
        }

        void show() override {
            if (this->source == nullptr) {
                return;
            }
            obs_source_t *ref_source = obs_weak_source_get_source(this->source);
            if (ref_source == nullptr) {
                return;
            }
            debug("Inc showing %s", obs_source_get_name(ref_source));
            obs_source_inc_showing(ref_source);
            obs_source_release(ref_source);
        }

        void hide() override {
            if (this->source == nullptr) {
                return;
            }
            obs_source_t *ref_source = obs_weak_source_get_source(this->source);
            if (ref_source == nullptr) {
                return;
            }
            debug("Dec showing %s", obs_source_get_name(ref_source));
            obs_source_dec_showing(ref_source);
            obs_source_release(ref_source);
        }
};
