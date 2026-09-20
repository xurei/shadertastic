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

#ifndef SHADERTASTIC_SHADER_TRANSITION_H
#define SHADERTASTIC_SHADER_TRANSITION_H

#define SHADERTASTIC_TRANSITION_NAME "shadertastic_transition"

#include <obs-module.h>
#include "shadertastic_common.hpp"

struct shadertastic_transition : public shadertastic_common {
    // !!! Do not add constructors or destructors. OBS handles this with bzalloc which won't execute them

    bool transition_started{};
    bool transitioning{};
    float transition_point{};
    gs_texrender_t *source_a_texrender;
    gs_texrender_t *source_b_texrender;
    gs_texrender_t *transition_texrender[2]{};
    int transition_texrender_buffer = 0;
    float rand_seed{};

    bool auto_reload = false;

    obs_transition_audio_mix_callback_t mix_a{};
    obs_transition_audio_mix_callback_t mix_b{};
    float transition_a_mul{};
    float transition_b_mul{};

    float prev_t = 0.0;
};
//----------------------------------------------------------------------------------------------------------------------

obs_properties_t *shadertastic_transition_properties(void *data);

void *shadertastic_transition_create(obs_data_t *settings, obs_source_t *source);

void shadertastic_transition_destroy(void *data);

void shadertastic_transition_update(void *data, obs_data_t *settings);

void shadertastic_transition_tick(void *data, float deltatime_seconds);

void shadertastic_transition_shader_render(void *data, gs_texture_t *a, gs_texture_t *b, float t, uint32_t cx, uint32_t cy);

void shadertastic_transition_video_render(void *data, gs_effect_t *effect);

bool shadertastic_transition_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio, uint32_t mixers, size_t channels, size_t sample_rate);

bool shadertastic_transition_properties_change_effect_callback(void *priv, obs_properties_t *props, obs_property_t *p, obs_data_t *data);

bool shadertastic_transition_export_button_click(obs_properties_t *props, obs_property_t *property, void *data);

bool shadertastic_transition_import_button_click(obs_properties_t *props, obs_property_t *property, void *data);

void shadertastic_transition_get_defaults(obs_data_t *settings);

void shadertastic_transition_start(void *data);

void shadertastic_transition_stop(void *data);

enum gs_color_space shadertastic_transition_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces);

void shadertastic_transition_unload();
//----------------------------------------------------------------------------------------------------------------------

#endif // SHADERTASTIC_SHADER_TRANSITION_H
