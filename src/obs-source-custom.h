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

#ifndef OBS_SOURCE_CUSTOM_H
#define OBS_SOURCE_CUSTOM_H

#include <obs-module.h>
#include "src/shader/shader.h"

struct shadertastic_filter;

bool shadertastic_source_process_filter_begin_with_color_space(obs_source_t *filter, shadertastic_filter *s, enum gs_color_format format, enum gs_color_space space);

// void shadertastic_source_process_filter_end(obs_source_t *filter, shadertastic_filter *s, gs_effect_t *effect, uint32_t width, uint32_t height);

void shadertastic_source_process_filter_tech_end(obs_source_t *obs_source, gs_texrender_t *texrender, effect_shader *shader, uint32_t width, uint32_t height, const char *tech_name);

#endif // OBS_SOURCE_CUSTOM_H
