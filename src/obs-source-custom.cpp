/******************************************************************************
	Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
	Copyright (C) 2025 by xurei <xureilab@gmail.com>

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

	Modifications:
		- 2025-12-18: Adapted for Shadertastic
******************************************************************************/

/**
 * This file contains modified version of the OBS source pipeline, for a faster integration.
 */

#include "obs-source-custom.h"
#include "shadertastic.hpp"
#include "src/util/texture_util.h"

inline void render_filter_tex(gs_texture_t *tex, const gs_effect_t *effect, const uint32_t width, const uint32_t height, const char *tech_name) {
	gs_technique_t *tech = gs_effect_get_technique(effect, tech_name);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");

	const bool linear_srgb = gs_get_linear_srgb();

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(linear_srgb);

	if (linear_srgb) {
		gs_effect_set_texture_srgb(image, tex);
	}
	else {
		gs_effect_set_texture(image, tex);
	}

	size_t passes = gs_technique_begin(tech);
	for (size_t i = 0; i < passes; i++) {
		gs_technique_begin_pass(tech, i);
		gs_draw_sprite(tex, 0, width, height);
		gs_technique_end_pass(tech);
	}
	gs_technique_end(tech);

	gs_enable_framebuffer_srgb(previous);
}

bool shadertastic_source_process_filter_begin_with_color_space(obs_source_t *filter, shadertastic_filter *s, gs_color_format format, gs_color_space space) {
	if (!filter || !s) {
		return false;
	}

	obs_source_t *target = obs_filter_get_target(filter);
	obs_source_t *parent = obs_filter_get_parent(filter);

	if (!target) {
		blog(LOG_INFO, "filter '%s' being processed with no target!", obs_source_get_name(filter));
		return false;
	}
	if (!parent) {
		blog(LOG_INFO, "filter '%s' being processed with no parent!", obs_source_get_name(filter));
		return false;
	}

	uint32_t parent_flags = obs_source_get_output_flags(parent);
	uint32_t cx = obs_source_get_base_width(target);
	uint32_t cy = obs_source_get_base_height(target);

	if (!cx || !cy) {
		obs_source_skip_video_filter(filter);
		return false;
	}

	if (s->filter_texrender_pre && gs_texrender_get_format(s->filter_texrender_pre) != format) {
		gs_texrender_destroy(s->filter_texrender_pre);
		s->filter_texrender_pre = nullptr;
	}

	if (!s->filter_texrender_pre) {
		s->filter_texrender_pre = gs_texrender_create(format, GS_ZS_NONE);
	}

	if (gs_texrender_begin_with_color_space(s->filter_texrender_pre, cx, cy, space)) {
		bool custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
		bool async = (parent_flags & OBS_SOURCE_ASYNC) != 0;
		constexpr vec4 clear_color{0,0,0,0};

        gs_blend_state_push();
        gs_blend_function_separate(
            GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA,
            GS_BLEND_ONE, GS_BLEND_INVSRCALPHA
        );
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

		if (target == parent && !custom_draw && !async) {
			obs_source_default_render(target);
		}
		else {
			obs_source_video_render(target);
		}

		gs_blend_state_pop();
		gs_texrender_end(s->filter_texrender_pre);
        //auto mat = extractImage(gs_texrender_get_texture(s->filter_texrender_pre));

		if (!s->filter_texrender) {
			s->filter_texrender = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
		}

		gs_texrender_reset(s->filter_texrender);
		if (gs_texrender_begin_with_color_space(s->filter_texrender, cx, cy, space)) {
			gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
            const bool prev_linear_srgb = gs_get_linear_srgb();
            gs_set_linear_srgb(true);
			render_texture(gs_texrender_get_texture(s->filter_texrender_pre), true, true);
            gs_set_linear_srgb(prev_linear_srgb);
			gs_texrender_end(s->filter_texrender);
            //auto mat2 = extractImage(gs_texrender_get_texture(s->filter_texrender_pre));
			gs_texrender_reset(s->filter_texrender_pre);
		}
	}

	return true;
}

void shadertastic_source_process_filter_tech_end(obs_source_t *filter, gs_texrender_t *filter_texrender, gs_effect_t *effect, uint32_t width, uint32_t height, const char *tech_name) {
	if (!filter)
		return;

	obs_source_t *target = obs_filter_get_target(filter);
	obs_source_t *parent = obs_filter_get_parent(filter);

	if (!target || !parent)
		return;

	uint32_t filter_flags = obs_source_get_output_flags(filter);

	const bool previous = gs_set_linear_srgb((filter_flags & OBS_SOURCE_SRGB) != 0);

	const char *tech = tech_name ? tech_name : "Draw";

	gs_texture_t *texture = gs_texrender_get_texture(filter_texrender);
	if (texture) {
		render_filter_tex(texture, effect, width, height, tech);
	}

	gs_set_linear_srgb(previous);
}