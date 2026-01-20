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

#include <string>
#include "parameter.hpp"
#include "../shadertastic_common.hpp"
#include "../util/string_util.h"
#include "../util/file_util.h"

class effect_parameter_prev_frame : public effect_parameter {
    private:
        int step_{};

        gs_texrender_t *prev_texrender[2]{ nullptr, nullptr };
        int prev_texrender_buffer = 0;

    public:
        explicit effect_parameter_prev_frame(gs_eparam_t *shader_param)
        : effect_parameter(sizeof(float), shader_param) {
        }

        ~effect_parameter_prev_frame() override {
            release_texrenders();
        }

        inline void init_texrenders() {
            obs_enter_graphics();
            if (prev_texrender[0] == nullptr) {
                prev_texrender[0] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
            }
            if (prev_texrender[1] == nullptr) {
                prev_texrender[1] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
            }
            obs_leave_graphics();
        }

        inline void release_texrenders() {
            obs_enter_graphics();
            if (prev_texrender[0] != nullptr) {
                gs_texrender_destroy(prev_texrender[0]);
                prev_texrender[0] = nullptr;
            }
            if (prev_texrender[1] != nullptr) {
                gs_texrender_destroy(prev_texrender[1]);
                prev_texrender[1] = nullptr;
            }
            obs_leave_graphics();
        }

        effect_param_datatype type() override {
            return PARAM_DATATYPE_PREV_FRAME;
        }

        [[nodiscard]] int step() const {
            return step_;
        }

        void initialize_params(const effect_shader *shader, obs_data_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(shader);
            UNUSED_PARAMETER(effect_path);
            obs_data_set_default_int(metadata, "step", -1); // -1 means "the last step". This way by default we take the last step, i.e. the actually rendered frame
            step_ = (int)obs_data_get_int(metadata, "step");

            init_texrenders();
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
            //debug("try_gs_set_val %i", prev_texrender_buffer);
            if (!prev_texrender[prev_texrender_buffer]) {
                return;
            }
            try_gs_effect_set_texture(name.c_str(), shader_param, gs_texrender_get_texture(prev_texrender[prev_texrender_buffer]));
        }

        void reset() {
            release_texrenders();
            init_texrenders();
        }

        bool attach(const uint32_t cx, const uint32_t cy, const gs_color_space source_space) {
            const int next_texrender_buffer = prev_texrender_buffer ^ 1;
            //debug("attach %i", prev_texrender_buffer);

            //init_texrenders(); // Probably not required, but ensures there is a texrender

            gs_texrender_reset(prev_texrender[next_texrender_buffer]);
            bool texrender_ok = gs_texrender_begin_with_color_space(prev_texrender[next_texrender_buffer], cx, cy, source_space);
            if (texrender_ok) {
                constexpr vec4 clear_color{0,0,0,0};
                gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
                gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
            }
            else {
                debug("effect_parameter_prev_frame: cannot begin texrender");
            }
            return texrender_ok;
        }

        const gs_texrender_t * detach() {
            const int next_texrender_buffer = prev_texrender_buffer ^ 1;
            gs_texrender_end(prev_texrender[next_texrender_buffer]);
            return prev_texrender[next_texrender_buffer];
        }

        void next_frame() {
            prev_texrender_buffer = prev_texrender_buffer ^ 1;
        }
};

#endif // SHADERTASTIC_PARAMETER_PREV_FRAME_HPP
