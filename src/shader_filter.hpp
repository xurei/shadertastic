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

#ifndef SHADERTASTIC_SHADER_FILTER_HPP
#define SHADERTASTIC_SHADER_FILTER_HPP

#define SHADERTASTIC_FILTER_NAME "shadertastic_filter"

// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
// ReSharper disable CppDFAConstantParameter

#include <obs-module.h>
#include <QApplication>
#include "effect.h"
#include "face_tracking/face_tracking.h"
#include "obs-source-custom.h"
#include "settings.h"
#include "shadertastic.hpp"
#include "shadertastic_common.hpp"
#include "util/texture_util.h"
#include "util/time_util.hpp"

obs_properties_t *shadertastic_filter_properties(void *data);
//----------------------------------------------------------------------------------------------------------------------

static shadertastic_filter *shadertastic_no_filter = nullptr;

static void *shadertastic_filter_create(obs_data_t *settings, obs_source_t *source);

void shadertastic_filter_show(void *data);
void shadertastic_filter_hide(void *data);
//----------------------------------------------------------------------------------------------------------------------

static shadertastic_filter* shadertastic_filter_cast(void *data) {
    if (data == nullptr) {
        if (shadertastic_no_filter == nullptr) {
            obs_data_t *no_filter_settings = obs_data_create();
            shadertastic_no_filter = static_cast<shadertastic_filter *>(shadertastic_filter_create(no_filter_settings, nullptr));
            obs_data_release(no_filter_settings);
        }
        return shadertastic_no_filter;
    }
    return static_cast<shadertastic_filter*>(data);
}
//----------------------------------------------------------------------------------------------------------------------

static void *shadertastic_filter_create(obs_data_t *settings, obs_source_t *source) {
    shadertastic_filter *s = static_cast<shadertastic_filter*>(bzalloc(sizeof(struct shadertastic_filter)));
    s->source = source;
    s->effects = new shadertastic_effects_map_t();
    s->rand_seed = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // NOLINT(*-msc50-cpp)
    s->frame_index = 0;

    // FIXME getting the root source doesn't work here :( it would be great for debugging, but obs_filter_get_parent() is not valid outside of video_render, filter_audio, filter_video, and filter_remove callbacks.
    //#ifdef DEV_MODE
    //    obs_source_t *root_source = obs_filter_get_parent(source);
    //    debug("%s", obs_source_get_name(root_source));
    //    while (obs_source_get_type(root_source) != OBS_SOURCE_TYPE_INPUT) {
    //        debug("%s", obs_source_get_name(root_source));
    //        root_source = obs_filter_get_parent(source);
    //    }
    //    debug("FILTER %s ON %s Settings : %s", obs_source_get_name(source), obs_source_get_name(root_source), obs_data_get_json(settings));
    //#endif
    debug("FILTER %s Settings : %s", (source==nullptr) ? "null" : obs_source_get_name(source), obs_data_get_json(settings));

    obs_enter_graphics();
    s->interm_texrender[0] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    s->interm_texrender[1] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    obs_leave_graphics();

    char *filters_dir_ = obs_module_file("effects");
    std::string filters_dir(filters_dir_);
    bfree(filters_dir_);

    load_effects(s, settings, filters_dir, "filter");
    auto effects_paths = shadertastic_settings().effects_paths;
    for (auto &effect_path : effects_paths) {
        load_effects(s, settings, effect_path, "filter");
    }

    // Set defaults for each effect
    for (auto& [effect_name, effect] : *(s->effects)) {
        // LEGACY - input_time is deprecated. Migrating it as a parameter
        if (effect.legacy_input_time) {
            obs_data_set_default_double(settings, get_full_param_name_static(effect_name, "speed").c_str(), 0.1);
            double legacy_speed = obs_data_get_double(settings, get_full_param_name_static(effect_name, "speed").c_str());
            obs_data_set_default_double(settings, get_full_param_name_static(effect_name, "time_speed").c_str(), legacy_speed);

            obs_data_set_default_bool(settings, get_full_param_name_static(effect_name, "reset_time_on_show").c_str(), false);
            bool legacy_reset_time_on_show = obs_data_get_bool(settings, get_full_param_name_static(effect_name, "reset_time_on_show").c_str());
            obs_data_set_default_bool(settings, get_full_param_name_static(effect_name, "time_reset_time_on_show").c_str(), legacy_reset_time_on_show);
        }
    }

    if (source != nullptr) {
        obs_source_update(source, settings);
    }

    return s;
}
//----------------------------------------------------------------------------------------------------------------------

static void shadertastic_filter_destroy(void *data) {
    shadertastic_filter *s = shadertastic_filter_cast(data);

    obs_enter_graphics();
    {
        release_resource(gs_texrender_destroy, s->interm_texrender[0]);
        release_resource(gs_texrender_destroy, s->interm_texrender[1]);
        release_resource(gs_texrender_destroy, s->filter_texrender);
        release_resource(gs_texrender_destroy, s->filter_texrender_pre);
    }
    obs_leave_graphics();
    release_resource(face_tracking_destroy, s->face_tracking);
    s->release();
    bfree(data);
}
//----------------------------------------------------------------------------------------------------------------------

inline uint32_t shadertastic_filter_getwidth(void *data) {
    const shadertastic_filter *s = shadertastic_filter_cast(data);
    return s->width;
}

inline uint32_t shadertastic_filter_getheight(void *data) {
    const shadertastic_filter *s = shadertastic_filter_cast(data);
    return s->height;
}
//----------------------------------------------------------------------------------------------------------------------

inline void shadertastic_filter_update(void *data, obs_data_t *settings) {
    shadertastic_filter *s = shadertastic_filter_cast(data);
    //debug("Update : %s", obs_data_get_json(settings));

    if (s->should_reload) {
        s->should_reload = false;
        obs_source_update_properties(s->source);
    }

    const char *selected_effect_name = obs_data_get_string(settings, "effect");
    auto selected_effect_it = s->effects->find(selected_effect_name);
    if (selected_effect_it != s->effects->end()) {
        s->selected_effect = &(selected_effect_it->second);
    }

    if (s->selected_effect != nullptr) {
        //obs_data_set_string(settings, (std::string(selected_effect_name) + "__compile_error").c_str(), s->selected_effect->error_str.c_str());
        for (auto param: s->selected_effect->effect_params) {
            param->set_data_from_settings(settings, selected_effect_name);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

static void shadertastic_filter_tick(void *data, float deltatime_seconds) {
    shadertastic_filter *s = shadertastic_filter_cast(data);

    obs_source_t *target = obs_filter_get_target(s->source);
    s->width = obs_source_get_base_width(target);
    s->height = obs_source_get_base_height(target);

    bool is_enabled = obs_source_enabled(s->source) && s->selected_effect != nullptr;

    if (is_enabled) {
        if (s->selected_effect->param_facetracking != nullptr) {
            if (s->face_tracking == nullptr) {
                face_tracking_create(s->face_tracking);
            }
        }

        s->time += deltatime_seconds;
        s->delta_time = deltatime_seconds;
        s->must_render = true;

        for (effect_parameter* param : s->selected_effect->effect_params) {
            if (param) {
                param->tick(s);
            }
        }
        if (!s->was_enabled) {
            s->frame_index = 0;
        }
    }
    if (is_enabled != s->was_enabled) {
        s->was_enabled = is_enabled;

        if (is_enabled) {
            shadertastic_filter_show(s);
        }
        else {
            shadertastic_filter_hide(s);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_filter_video_render(void *data, gs_effect_t *effect_unused) {
    //debug("-----------------------------------------");
    UNUSED_PARAMETER(effect_unused);
    shadertastic_filter *s = shadertastic_filter_cast(data);
    shadertastic_effect_t *selected_effect = s->selected_effect;
    if (selected_effect == nullptr || selected_effect->main_shader == nullptr) {
        //debug("%s : No effect selected", obs_source_get_name(s->source));
        obs_source_skip_video_filter(s->source);
        return;
    }

    constexpr gs_color_space preferred_spaces[] = {
        GS_CS_SRGB,
        GS_CS_SRGB_16F,
        GS_CS_709_EXTENDED,
    };
    obs_source_t *target_source = obs_filter_get_target(s->source);

    const gs_color_space source_space = obs_source_get_color_space(target_source, OBS_COUNTOF(preferred_spaces), preferred_spaces);
    const gs_color_format format = gs_get_format_from_space(source_space);

    if (!shadertastic_source_process_filter_begin_with_color_space(s->source, s, format, source_space)) {
        obs_source_skip_video_filter(s->source);
        return;
    }

    const float filter_time = s->time;
    const uint32_t cx = s->width;
    const uint32_t cy = s->height;

    if (!s->must_render) {
        gs_texture_t *final_texture = gs_texrender_get_texture(s->interm_texrender[s->interm_texrender_buffer]);
        const bool prev_linear_srgb = gs_set_linear_srgb(true);
        render_texture(final_texture, false, false);
        gs_set_linear_srgb(prev_linear_srgb);
        return;
    }
    s->must_render = false;

    const bool prev_linear_srgb = gs_set_linear_srgb(true);

    // Facetracking
    if (selected_effect->param_facetracking && s->face_tracking != nullptr) {
        #ifdef DEV_MODE
        unsigned long tic = get_time_us();
        #endif
        auto *source_tex = gs_texrender_get_texture(s->filter_texrender);
        face_tracking_tick(s->face_tracking.get(), source_tex, s->delta_time);

        debug_trace("Facetracking time %lu", get_time_us()-tic);
    }

    gs_texture_t *interm_texture = shadertastic_transparent_texture;

    gs_blend_state_push();
    gs_blend_function_separate(
        GS_BLEND_ONE, GS_BLEND_ZERO,
        GS_BLEND_ONE, GS_BLEND_ZERO
    );
    constexpr vec4 clear_color{0,0,0,0};

    bool render_ok = true;
    for (int current_step=0; current_step < selected_effect->nb_steps; ++current_step) {
        s->interm_texrender_buffer = s->interm_texrender_buffer ^ 1;
        gs_texrender_reset(s->interm_texrender[s->interm_texrender_buffer]);
        bool texrender_ok = gs_texrender_begin_with_color_space(s->interm_texrender[s->interm_texrender_buffer], cx, cy, GS_CS_SRGB_16F);

        if (!texrender_ok) {
            render_ok = false;
            break;
        }

        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
        gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f); // This line took me A WHOLE WEEK to figure out

        selected_effect->set_step_params(current_step, interm_texture);
        // You CANNOT put it above the for loop. Textures need to be rebinded every time (not that costful actually)
        selected_effect->set_params(nullptr, nullptr, s->frame_index, false, filter_time, s->delta_time, cx, cy, s->rand_seed);

        shadertastic_source_process_filter_tech_end(s->source, s->filter_texrender, selected_effect->main_shader.get(), cx, cy, "Draw");

        gs_texrender_end(s->interm_texrender[s->interm_texrender_buffer]);
        interm_texture = gs_texrender_get_texture(s->interm_texrender[s->interm_texrender_buffer]);

        if (current_step < (int)selected_effect->prev_frames_to_keep.size() && selected_effect->prev_frames_to_keep[current_step]) {
            if (selected_effect->prev_frames_to_keep[current_step]->attach(cx, cy, source_space)) {
                render_texture(interm_texture, false, true);
                selected_effect->prev_frames_to_keep[current_step]->detach();
            }
        }
    }

    s->frame_index++;
    for (auto *prev_frame : selected_effect->prev_frames_to_keep) {
        if (prev_frame != nullptr) {
            prev_frame->next_frame();
        }
    }

    if (render_ok) {
        render_texture(interm_texture, false, false);
    }
    else {
        debug("huh?");
    }

    gs_set_linear_srgb(prev_linear_srgb);
    gs_blend_state_pop();
}
//----------------------------------------------------------------------------------------------------------------------

bool shadertastic_filter_properties_change_effect_callback(void *priv, obs_properties_t *props, obs_property_t *p, obs_data_t *data) {
    UNUSED_PARAMETER(p);
    shadertastic_filter *s = shadertastic_filter_cast(priv);

    if (s->selected_effect != nullptr) {
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__params").c_str()), false);
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__warning").c_str()), false);
    }

    //shadertastic_filter_properties(priv);
    const char *select_effect_name = obs_data_get_string(data, "effect");
    debug("CALLBACK : %s", select_effect_name);
    auto selected_effect = s->effects->find(std::string(select_effect_name));
    if (selected_effect != s->effects->end()) {
        debug("CALLBACK : %s -> %s", select_effect_name, selected_effect->second.name.c_str());
        obs_property_set_visible(obs_properties_get(props, (selected_effect->second.name + "__params").c_str()), true);

        if (selected_effect->second.has_error()) {
            obs_property_set_visible(obs_properties_get(props, (selected_effect->second.name + "__warning").c_str()), true);
        }
    }

    return true;
}

bool shadertastic_filter_reload_button_click(obs_properties_t *props, obs_property_t *property, void *data) {
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    shadertastic_filter *s = shadertastic_filter_cast(data);

    if (s->selected_effect != nullptr) {
        s->selected_effect->reload();
    }
    s->should_reload = true;
    s->rand_seed = (float)rand() / (float)RAND_MAX;
    s->was_enabled = false;
    s->frame_index = 0;
    obs_source_update(s->source, nullptr);
    return true;
}

obs_properties_t *shadertastic_filter_properties(void *data) {
    shadertastic_filter *s = shadertastic_filter_cast(data);
    obs_properties_t *props = obs_properties_create();

    obs_property_t *p;

    // Dev mode settings
    if (shadertastic_settings().dev_mode_enabled) {
        obs_properties_add_button2(props, "reload_btn", "Reload", shadertastic_filter_reload_button_click, nullptr);
    }

    // Shader mode
    p = obs_properties_add_list(props, "effect", "Effect", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(p, "(Choose an effect)", "");

    std::vector<std::pair<std::string, shadertastic_effect_t*>> sorted_effects;
    sorted_effects.reserve(s->effects->size());
    for (auto& [effect_name, effect] : *(s->effects)) {
        sorted_effects.emplace_back(effect_name, &effect);
    }
    std::sort(sorted_effects.begin(), sorted_effects.end(),
        [](const auto& a, const auto& b) {
            std::string left = a.second->label;
            std::string right = b.second->label;
            std::transform(left.begin(), left.end(), left.begin(), ::tolower);
            std::transform(right.begin(), right.end(), right.begin(), ::tolower);
            return left < right;
        }
    );
    for (const auto& [effect_name, effect] : sorted_effects) {
        const char *effect_label = effect->label.c_str();
        obs_property_list_add_string(p, effect_label, effect_name.c_str());
    }
    obs_property_set_modified_callback2(p, shadertastic_filter_properties_change_effect_callback, data);

    for (auto& [effect_name, effect] : *(s->effects)) {
        const char *effect_label = effect.label.c_str();
        obs_properties_t *effect_group = obs_properties_create();
        //obs_properties_add_text(effect_group, "", effect_name, OBS_TEXT_INFO);

        obs_properties_t *error_group = obs_properties_create();
        std::string warning_group_name = (effect_name + "__warning");
        auto *warning_group = obs_properties_add_group(props, warning_group_name.c_str(), "⚠ Shader error", OBS_GROUP_NORMAL, error_group);
        obs_property_set_visible(warning_group, false);
        if (effect.has_error()) {
            auto prop = obs_properties_add_text(
                error_group,
                (effect_name + "__compile_error").c_str(),
                effect.error_str().c_str(),
                OBS_TEXT_INFO
            );
            obs_property_text_set_info_type(prop, OBS_TEXT_INFO_WARNING);

            if (s->selected_effect == &effect) {
                obs_property_set_visible(warning_group, true);
            }
        }

        for (auto param: effect.effect_params) {
            if (!param->is_dev_mode() || shadertastic_settings().dev_mode_enabled) {
                param->render_property_ui(effect_name.c_str(), effect_group);
                param->apply_visibility_condition(effect_group);
            }
        }
        obs_properties_add_group(props, (effect_name + "__params").c_str(), effect_label, OBS_GROUP_NORMAL, effect_group);
        obs_property_set_visible(obs_properties_get(props, (effect_name + "__params").c_str()), false);
    }
    if (s->selected_effect != nullptr) {
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__params").c_str()), true);
        if (s->selected_effect->has_error()) {
            obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__warning").c_str()), true);
        }
        else {
            obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__warning").c_str()), false);
        }
    }

    about_property(props);

    return props;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_filter_get_defaults(obs_data_t *settings) {
    if (shadertastic_no_filter == nullptr) {
        shadertastic_no_filter = static_cast<shadertastic_filter *>(shadertastic_filter_create(settings, nullptr));
    }
    for (auto effect : *shadertastic_no_filter->effects) {
        shadertastic_effect_set_defaults(settings, &effect.second);
    }
}

static gs_color_space shadertastic_filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces) {
    shadertastic_filter *s = shadertastic_filter_cast(data);
    const enum gs_color_space source_space = obs_source_get_color_space(
        obs_filter_get_target(s->source),
        count, preferred_spaces
    );

    return source_space;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_filter_show(void *data) {
    shadertastic_filter *s = shadertastic_filter_cast(data);
    if (!s->is_showing) {
        s->is_showing = true;
        shadertastic_effect_t *selected_effect = s->selected_effect;
        if (selected_effect != nullptr) {
            selected_effect->show();
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_filter_hide(void *data) {
    shadertastic_filter *s = shadertastic_filter_cast(data);
    if (s->is_showing) {
        s->is_showing = false;
        shadertastic_effect_t *selected_effect = s->selected_effect;
        if (selected_effect != nullptr) {
            selected_effect->hide();
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_filter_unload() {
    release_resource(shadertastic_filter_destroy, shadertastic_no_filter);
}

#endif // SHADERTASTIC_SHADER_FILTER_HPP
