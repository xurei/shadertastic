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

#ifndef SHADERTASTIC_EFFECT_HPP
#define SHADERTASTIC_EFFECT_HPP

#include <memory>
#include <vector>
#include <set>

#include "params_list.hpp"
#include "shader/shader.h"
#include "parameters/parameter_prev_frame.hpp"

struct shadertastic_effect_t {
    const std::string path;
    const std::string name;
    std::string label;
    int nb_steps{};
    bool input_time = false;
    bool input_prev_frame = false;
    bool input_facedetection = false;
    std::vector<effect_parameter_prev_frame *> prev_frames_to_keep{};

    params_list effect_params;
    std::shared_ptr<effect_shader> main_shader = nullptr;

    shadertastic_effect_t(std::string name_, std::string path_): name(std::move(name_)), path(std::move(path_)) {}

    void load();

    void reload();

    void set_params(gs_texture_t *a, gs_texture_t *b, float t, float delta_t, uint32_t cx, uint32_t cy, float rand_seed);

    void set_step_params(int current_step, gs_texture_t *interm) const;

    void render_shader(uint32_t cx, uint32_t cy) const;

    void show();

    void hide();

    void release();
};

#endif /* SHADERTASTIC_EFFECT_HPP */
