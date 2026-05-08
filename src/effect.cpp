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

#include <string>
#include <obs-module.h>
#include "effect.h"
#include "logging_functions.hpp"
#include "try_gs_effect_set.h"
#include "parameters/parameter_facetracking.hpp"
#include "parameters/parameter_factory.h"
#include "shader/shaders_library.h"
#include "util/file_util.h"

void shadertastic_effect_t::load() {
    std::string metadata_path = normalize_path(this->path + "/meta.json");
    debug(">>>>>>>>>>>>>>> load_effect %s %s %s", this->name.c_str(), this->path.c_str(), metadata_path.c_str());

    this->main_shader = shaders_library.get(this->path);

    char *meta_json = load_file_zipped_or_local(metadata_path);

    if (meta_json == nullptr) {
        // Something went wrong -> set default configuration
        warn("Unable to open file for effect %s.", name.c_str());
        label = name;
        nb_steps = 1;
        return;
    }

    obs_data_t *metadata = obs_data_create_from_json(meta_json);
    bfree(meta_json);
    if (metadata == nullptr) {
        // Something went wrong -> set default configuration
        warn("Unable to parse metadata for effect %s. Check the JSON syntax", name.c_str());
        label = name;
        nb_steps = 1;
    }
    else {
        const char *effect_label = obs_data_get_string(metadata, "label");
        if (effect_label == nullptr) {
            label = name;
        }
        else {
            label = std::string(effect_label);
        }
        obs_data_set_default_int(metadata, "steps", 1);
        nb_steps = (int)obs_data_get_int(metadata, "steps");

        obs_data_array_t *parameters = obs_data_get_array(metadata, "parameters");
        if (parameters == nullptr) {
            warn("No parameters specified for effect %s", name.c_str());
            parameters = obs_data_array_create();
        }

        // LEGACY - input_facedetection is deprecated.
        obs_data_set_default_bool(metadata, "input_facedetection", false);
        legacy_input_facedetection = obs_data_get_bool(metadata, "input_facedetection");
        if (legacy_input_facedetection) {
            obs_data_t *param_metadata = obs_data_create();
            obs_data_set_string(param_metadata, "name", "fd");
            obs_data_set_string(param_metadata, "type", "facetracking");
            obs_data_array_insert(parameters, 0, param_metadata);
            obs_data_release(param_metadata);
        }

        // LEGACY - input_time is deprecated. Migrating it as a parameter
        obs_data_set_default_bool(metadata, "input_time", false);
        legacy_input_time = obs_data_get_bool(metadata, "input_time");
        if (legacy_input_time) {
            obs_data_t *param_metadata = obs_data_create();
            obs_data_set_string(param_metadata, "name", "time");
            obs_data_set_string(param_metadata, "type", "time");
            obs_data_set_string(param_metadata, "reset_on_show", "prompt");
            obs_data_array_insert(parameters, 0, param_metadata);
            obs_data_release(param_metadata);
        }

        // Copy the effect params map to allow recycling
        params_list previous_effect_params(effect_params);
        effect_params.clear();

        size_t nb_parameters = obs_data_array_count(parameters);
        prev_frames_to_keep.resize(nb_steps);
        std::fill(prev_frames_to_keep.begin(), prev_frames_to_keep.end(), nullptr);

        for (size_t i=0; i < nb_parameters; i++) {
            obs_data_t *param_metadata = obs_data_array_item(parameters, i);
            effect_parameter *effect_param = effect_parameter_factory::create(name, this->path, main_shader.get(), param_metadata);

            if (effect_param != nullptr) {
                auto param_type = effect_param->type();
                if (param_type == PARAM_DATATYPE_PREV_FRAME) {
                    effect_parameter_prev_frame *effect_param_prev_frame = dynamic_cast<effect_parameter_prev_frame *>(effect_param);

                    int step_to_keep = effect_param_prev_frame->step();
                    if (step_to_keep < 0) {
                        step_to_keep = nb_steps - 1;
                    }
                    if (step_to_keep >= nb_steps) {
                        log_error("Trying to use a prev frame on a step higher than the maximum steps : %s\n", name.c_str());
                    }
                    else {
                        prev_frames_to_keep[step_to_keep] = effect_param_prev_frame;
                    }
                }
                else if (param_type == PARAM_DATATYPE_FACETRACKING) {
                    if (param_facetracking != nullptr) {
                        log_error("Trying to use multiple face tracking parameters. This makes no sense. Params: %s and %s", effect_param->get_name().c_str(), param_facetracking->get_name().c_str());
                    }
                    else {
                        param_facetracking = effect_param;
                    }
                }
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
        obs_data_release(metadata);
        debug("Loaded effect %s from %s", name.c_str(), metadata_path.c_str());
    }
}

void shadertastic_effect_t::reload() {
    shaders_library.reload(this->path);
    load();
}

void shadertastic_effect_t::set_params(
    gs_texture_t *a, gs_texture_t *b,
    int frame_index, bool is_studio_mode,
    float t, float delta_t,
    uint32_t cx, uint32_t cy,
    float rand_seed) {
    /* texture setters look reversed, but they aren't */
    if (gs_get_color_space() == GS_CS_SRGB) {
        /* users want nonlinear effect */
        try_gs_effect_set_texture("tex_a", main_shader->param_tex_a, a);
        try_gs_effect_set_texture("tex_b", main_shader->param_tex_b, b);
    }
    else {
        /* nonlinear effect is too wrong, so use linear effect */
        try_gs_effect_set_texture_srgb("tex_a", main_shader->param_tex_a, a);
        try_gs_effect_set_texture_srgb("tex_b", main_shader->param_tex_b, b);
    }
    //debug("input textures set");

    try_gs_effect_set_bool("is_studio_mode", main_shader->param_is_studio_mode, is_studio_mode);
    try_gs_effect_set_int("frame_index", main_shader->param_frame_index, frame_index);
    try_gs_effect_set_float("time", main_shader->param_time, t);
    try_gs_effect_set_float("delta_time", main_shader->param_delta_time, delta_t);
    try_gs_effect_set_float("upixel", main_shader->param_upixel, (float)(1.0/cx));
    try_gs_effect_set_float("vpixel", main_shader->param_vpixel, (float)(1.0/cy));
    try_gs_effect_set_float("rand_seed", main_shader->param_rand_seed, rand_seed);
    try_gs_effect_set_int("nb_steps", main_shader->param_nb_steps, nb_steps);
    //debug("common params set");

    for (auto param: effect_params) {
        param->try_gs_set_val();
    }
    //debug("all params set");
}

void shadertastic_effect_t::set_step_params(int current_step, gs_texture_t *interm) const {
    if (gs_get_color_space() == GS_CS_SRGB) {
        /* users want nonlinear fade */
        try_gs_effect_set_texture("tex_interm", main_shader->param_tex_interm, interm);
    }
    else {
        /* nonlinear fade is too wrong, so use linear fade */
        try_gs_effect_set_texture_srgb("tex_interm", main_shader->param_tex_interm, interm);
    }
    try_gs_effect_set_int("current_step", main_shader->param_current_step, current_step);
}


void shadertastic_effect_t::render_shader(uint32_t cx, uint32_t cy, bool use_linear) const {
    const char *tech_name = (!use_linear && gs_get_color_space() == GS_CS_SRGB) ? "Draw" : "DrawLinear";
    while (main_shader->loop(tech_name)) {
        gs_draw_sprite(nullptr, 0, cx, cy);
    }
}

void shadertastic_effect_t::show() {
    debug("show %s", this->name.c_str());
    for (auto param: this->effect_params) {
        if (param != nullptr) {
            param->show();
        }
    }
}

void shadertastic_effect_t::hide() {
    debug("hide %s", this->name.c_str());
    for (auto param: this->effect_params) {
        if (param != nullptr) {
            param->hide();
        }
    }
}

void shadertastic_effect_t::release() {
    for (auto effect_param: effect_params) {
        delete effect_param;
    }
}
