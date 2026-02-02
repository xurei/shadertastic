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
#include "effect.h"
#include "version.h"
//----------------------------------------------------------------------------------------------------------------------

void load_effects(shadertastic_common *s, obs_data_t *settings, const std::string &effects_dir, const std::string &effects_type);
//----------------------------------------------------------------------------------------------------------------------

struct shadertastic_transition : public shadertastic_common {
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

    void release() {
        if (this->effects != nullptr) {
            for (auto& [key, effect] : *this->effects) {
                effect.release();
            }
            delete this->effects;
        }
    }
};
//----------------------------------------------------------------------------------------------------------------------

struct shadertastic_filter : public shadertastic_common {
    gs_texrender_t *filter_texrender_pre{};
    gs_texrender_t *filter_texrender{};
    gs_texrender_t *interm_texrender[2]{};
    int interm_texrender_buffer = 1;
    float rand_seed{};
    uint32_t width{}, height{};
    bool should_reload = false;
    bool is_showing = false;

    void release() {
        if (this->effects != nullptr) {
            for (auto &[key, effect]: *this->effects) {
                effect.release();
            }
            delete this->effects;
        }
    }
};
//----------------------------------------------------------------------------------------------------------------------

const char *shadertastic_transition_get_name(void *type_data);
const char *shadertastic_filter_get_name(void *type_data);

void shadertastic_effect_set_defaults(obs_data_t *settings, shadertastic_effect_t *effect);

void about_property(obs_properties_t *props);

MODULE_EXPORT [[maybe_unused]] const char *obs_module_name(void);
MODULE_EXPORT [[maybe_unused]] const char *obs_module_description(void);
//----------------------------------------------------------------------------------------------------------------------

#endif // SHADERTASTIC_HPP
