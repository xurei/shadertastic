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

#ifndef SHADERTASTIC_HPP
#define SHADERTASTIC_HPP

#include "shadertastic_common.hpp"
#include "version.h"
//----------------------------------------------------------------------------------------------------------------------

void load_effects(shadertastic_common *s, obs_data_t *settings, const std::string &effects_dir, const std::string &effects_type);
//----------------------------------------------------------------------------------------------------------------------

const char *shadertastic_transition_get_name(void *type_data);
const char *shadertastic_filter_get_name(void *type_data);

void shadertastic_effect_set_defaults(obs_data_t *settings, shadertastic_effect_t *effect);

void about_property(obs_properties_t *props);

MODULE_EXPORT [[maybe_unused]] const char *obs_module_name(void);
MODULE_EXPORT [[maybe_unused]] const char *obs_module_description(void);

bool shadertastic_is_direct3d();
//----------------------------------------------------------------------------------------------------------------------

#endif // SHADERTASTIC_HPP
