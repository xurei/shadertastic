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

#ifndef SHADERTASTIC_SHADER_FILTER_H
#define SHADERTASTIC_SHADER_FILTER_H

#define SHADERTASTIC_FILTER_NAME "shadertastic_filter"

#include <obs-module.h>
#include "shadertastic_common.hpp"

struct shadertastic_filter : public shadertastic_common {
    // !!! Do not add constructors or destructors. OBS handles this with bzalloc which won't execute them

    gs_texrender_t *filter_texrender_pre{};
    gs_texrender_t *filter_texrender{};
    gs_texrender_t *interm_texrender[2]{};
    int interm_texrender_buffer = 1;
    float rand_seed{};
    uint32_t width{}, height{};
    bool should_reload = false;
    bool is_showing = false;
};
//----------------------------------------------------------------------------------------------------------------------

void *shadertastic_filter_create(obs_data_t *settings, obs_source_t *source);

void shadertastic_filter_add_callback(void *data, obs_source_t *source);

void shadertastic_filter_remove_callback(void *data, obs_source_t *source);

void shadertastic_filter_destroy(void *data);

void shadertastic_filter_update(void *data, obs_data_t *settings);

void shadertastic_filter_tick(void *data, float deltatime_seconds);

void shadertastic_filter_video_render(void *data, gs_effect_t *effect_unused);

obs_properties_t *shadertastic_filter_properties(void *data);

bool shadertastic_filter_properties_change_effect_callback(void *priv, obs_properties_t *props, obs_property_t *p, obs_data_t *data);

bool shadertastic_filter_reload_button_click(obs_properties_t *props, obs_property_t *property, void *data);

void shadertastic_filter_get_defaults(obs_data_t *settings);

gs_color_space shadertastic_filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces);

void shadertastic_filter_show(void *data);

void shadertastic_filter_hide(void *data);

void shadertastic_filter_unload();

uint32_t shadertastic_filter_getwidth(void *data);

uint32_t shadertastic_filter_getheight(void *data);

#endif // SHADERTASTIC_SHADER_FILTER_H
