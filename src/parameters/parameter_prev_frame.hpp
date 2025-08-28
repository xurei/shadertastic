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

#ifndef SHADERTASTIC_PARAMETER_PREV_FRAME_HPP
#define SHADERTASTIC_PARAMETER_PREV_FRAME_HPP

#include "../util/string_util.h"
#include "../util/file_util.h"
#include "parameter.hpp"

class effect_parameter_prev_frame : public effect_parameter {
    private:
        int step_{};

        gs_texrender_t *prev_texrender[2]{};
        int prev_texrender_buffer = 0;
        gs_texture_t *prev_texture = nullptr;
        gs_texture_t *cur_texture = nullptr;

    public:
        explicit effect_parameter_prev_frame(gs_eparam_t *shader_param) : effect_parameter(sizeof(float), shader_param) {
            this->prev_texture = nullptr;
        }

        ~effect_parameter_prev_frame() override {

            if (prev_texrender[0] != nullptr) {
                obs_enter_graphics();
                gs_texrender_destroy(prev_texrender[0]);
                prev_texrender[0] = nullptr;
                obs_leave_graphics();
            }
            if (prev_texrender[1] != nullptr) {
                obs_enter_graphics();
                gs_texrender_destroy(prev_texrender[1]);
                prev_texrender[1] = nullptr;
                obs_leave_graphics();
            }
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_PREV_FRAME;
        }

        [[nodiscard]] int step() const {
            return step_;
        }

        void initialize_params(obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(effect_path);
            obs_data_set_default_int(metadata, "step", -1); // -1 means "the last step". This way by default we take the last step, i.e. the actually rendered frame
            step_ = (int)obs_data_get_int(metadata, "step");

            // FIXME faudrait ptetre pas laisser ça là, ca va en créer 10k pour rien
            prev_texrender[0] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            prev_texrender[1] = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
        }

        void set_default(obs_data_t *settings, const char *full_param_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(full_param_name);
        }

        void render_property_ui(const char *full_param_name, obs_properties_t *props) override {
            UNUSED_PARAMETER(full_param_name);
            UNUSED_PARAMETER(props);
            /* Automatic parameter, no UI */
        }

        void set_data_from_settings(obs_data_t *settings, const char *full_param_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(full_param_name);
            /* Automatic parameter, no UI */
        }

        void try_gs_set_val() override {
            try_gs_effect_set_texture(name.c_str(), shader_param, this->prev_texture);
        }

        bool attach(const uint32_t cx, const uint32_t cy, const gs_color_space source_space) {
            prev_texture = cur_texture;
            prev_texrender_buffer = (prev_texrender_buffer + 1) & 1;
            gs_texrender_reset(prev_texrender[prev_texrender_buffer]);
            bool texrender_ok = gs_texrender_begin_with_color_space(prev_texrender[prev_texrender_buffer], cx, cy, source_space);
            if (texrender_ok) {
                struct vec4 clear_color{0,0,0,0};
                gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
                gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
            }
            else {
                debug("effect_parameter_prev_frame: cannot begin texrender");
            }
            return texrender_ok;
        }

        void detach(obs_source_t *source, const uint32_t cx, const uint32_t cy, const bool is_srgb) {
            UNUSED_PARAMETER(source);
            UNUSED_PARAMETER(cx);
            UNUSED_PARAMETER(cy);
            UNUSED_PARAMETER(is_srgb);
            gs_texrender_end(prev_texrender[prev_texrender_buffer]);
            cur_texture = gs_texrender_get_texture(prev_texrender[prev_texrender_buffer]);

            if (cur_texture) {
                gs_blend_state_push();
                gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO); // copy exact*/

                {
                    gs_effect_t *default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
                    gs_technique_t *tech = gs_effect_get_technique(default_effect, is_srgb ? "DrawSrgbDecompress" : "Draw");
                    gs_eparam_t *image = gs_effect_get_param_by_name(default_effect, "image");
                    gs_effect_set_texture(image, cur_texture);

                    if (gs_technique_begin(tech)) {
                        if (gs_technique_begin_pass(tech, 0)) {
                            gs_draw_sprite(nullptr, 0, cx, cy);
                            gs_technique_end_pass(tech);
                        }
                        gs_technique_end(tech);
                    }
                }
                gs_blend_state_pop();
            }
        }
};

#endif /* SHADERTASTIC_PARAMETER_PREV_FRAME_HPP */