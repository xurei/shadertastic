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

#ifndef SHADERTASTIC_SHADER_TRANSITION_HPP
#define SHADERTASTIC_SHADER_TRANSITION_HPP

#include <obs-module.h>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include "effect.h"
#include "settings.h"
#include "shadertastic.hpp"
#include "shadertastic_common.hpp"
#include "util/texture_util.h"

obs_properties_t *shadertastic_transition_properties(void *data);
void shadertastic_transition_update(void *data, obs_data_t *settings);
//----------------------------------------------------------------------------------------------------------------------

static shadertastic_transition shadertastic_no_transition;

static inline shadertastic_transition* shadertastic_transition_cast(void *data) {
    if (data == nullptr) {
        if (shadertastic_no_transition.effects == nullptr) {
            shadertastic_no_transition.effects = new shadertastic_effects_map_t();
        }
        return &shadertastic_no_transition;
    }
    return static_cast<shadertastic_transition *>(data);
}
//----------------------------------------------------------------------------------------------------------------------

static void *shadertastic_transition_create(obs_data_t *settings, obs_source_t *source) {
    struct shadertastic_transition *s = static_cast<shadertastic_transition*>(bzalloc(sizeof(struct shadertastic_transition)));
    s->source = source;
    s->effects = new shadertastic_effects_map_t();
    s->rand_seed = (float)((double)rand() / (double)RAND_MAX);

    debug("TRANSITION %s Settings : %s", obs_source_get_name(source), obs_data_get_json(settings));

    char *transitions_dir_ = obs_module_file("effects");
    std::string transitions_dir(transitions_dir_);
    bfree(transitions_dir_);
    obs_enter_graphics();
    s->transition_texrender[0] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    s->transition_texrender[1] = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    s->source_a_texrender = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    s->source_b_texrender = gs_texrender_create(GS_RGBA16, GS_ZS_NONE);
    obs_leave_graphics();

    load_effects(s, settings, transitions_dir, "transition");
    auto effects_paths = shadertastic_settings().effects_paths;
    for (auto &effect_path : effects_paths) {
        load_effects(s, settings, effect_path, "transition");
    }

    //obs_source_update(source, settings);
    shadertastic_transition_update(s, settings);
    return s;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_destroy(void *data) {
    debug("Destroy...");
    struct shadertastic_transition *s = static_cast<shadertastic_transition*>(data);

    obs_enter_graphics();
    {
        release_resource(gs_texrender_destroy, s->transition_texrender[0]);
        release_resource(gs_texrender_destroy, s->transition_texrender[1]);
        release_resource(gs_texrender_destroy, s->source_a_texrender);
        release_resource(gs_texrender_destroy, s->source_b_texrender);
    }
    obs_leave_graphics();

    s->release();
    bfree(data);
    debug_trace("Destroyed");
}
//----------------------------------------------------------------------------------------------------------------------

static inline float calc_fade(float t, float mul) {
    t *= mul;
    return t > 1.0f ? 1.0f : t;
}
//----------------------------------------------------------------------------------------------------------------------

static float mix_a_fade_in_out(void *data, float t) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    return 1.0f - calc_fade(t, s->transition_a_mul);
}
//----------------------------------------------------------------------------------------------------------------------

static float mix_b_fade_in_out(void *data, float t) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    return 1.0f - calc_fade(1.0f - t, s->transition_b_mul);
}
//----------------------------------------------------------------------------------------------------------------------

static float mix_a_cross_fade(void *data, float t) {
    UNUSED_PARAMETER(data);
    return 1.0f - t;
}
//----------------------------------------------------------------------------------------------------------------------

static float mix_b_cross_fade(void *data, float t) {
    UNUSED_PARAMETER(data);
    return t;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_update(void *data, obs_data_t *settings) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    //debug("Update : %s", obs_data_get_json(settings));

    s->auto_reload = obs_data_get_bool(settings, "auto_reload");

    s->transition_point = (float)obs_data_get_double(settings, "transition_point") / 100.0f;
    s->transition_a_mul = (1.0f / s->transition_point);
    s->transition_b_mul = (1.0f / (1.0f - s->transition_point));

    const char *selected_effect_name = obs_data_get_string(settings, "effect");
    auto selected_effect_it = s->effects->find(selected_effect_name);
    if (selected_effect_it != s->effects->end()) {
        s->selected_effect = &(selected_effect_it->second);
    }

    if (s->selected_effect != nullptr) {
        for (auto param: s->selected_effect->effect_params) {
            std::string full_param_name = param->get_full_param_name(selected_effect_name);
            param->set_data_from_settings(settings, full_param_name.c_str());
        }
    }

    if (!obs_data_get_int(settings, "audio_fade_style")) {
        s->mix_a = mix_a_cross_fade;
        s->mix_b = mix_b_cross_fade;
    }
    else {
        s->mix_a = mix_a_fade_in_out;
        s->mix_b = mix_b_fade_in_out;
    }
}
//----------------------------------------------------------------------------------------------------------------------

static void shadertastic_transition_tick(void *data, float deltatime_seconds) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    if (s->transitioning) {
        s->time += deltatime_seconds;
        s->delta_time = deltatime_seconds;
        debug_trace("shadertastic_transition_tick %i", s->frame_index);
        s->frame_index++;

        if (s->selected_effect) {
            for (effect_parameter* param : s->selected_effect->effect_params) {
                if (param) {
                    param->tick(s);
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_shader_render(void *data, gs_texture_t *a, gs_texture_t *b, float t, uint32_t cx, uint32_t cy) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    shadertastic_effect_t *effect = s->selected_effect;

    bool is_studio_mode = false;
    if (s->prev_t == t) {
        is_studio_mode = true;
        s->frame_index--;
    }
    debug_trace("shadertastic_transition_shader_render %i", s->frame_index);

    if (effect != nullptr) {
        gs_texture_t *interm_texture = shadertastic_transparent_texture;
        constexpr vec4 clear_color{ 0.0, 0.0, 0.0, 0.0 };

        const gs_color_space preferred_spaces[] = {
            GS_CS_SRGB,
            GS_CS_SRGB_16F,
            GS_CS_709_EXTENDED,
        };
        obs_source_t *target_source = obs_filter_get_target(s->source);
        const gs_color_space source_space = obs_source_get_color_space(target_source, OBS_COUNTOF(preferred_spaces), preferred_spaces);

//        if (s->frame_index == 0) {
//            auto matA = extractImage(a);
//            auto matB = extractImage(b);
//            saveMat(matA, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_a.png");
//            saveMat(matB, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_b.png");
//        }

        bool render_ok = true;

        const bool prev_linear_srgb = gs_set_linear_srgb(true);
        gs_texrender_reset(s->source_a_texrender);
        const uint32_t a_cx = gs_texture_get_width(a), a_cy = gs_texture_get_height(a);
        gs_texrender_begin_with_color_space(s->source_a_texrender, a_cx, a_cy, source_space);
        gs_ortho(0.0f, (float)a_cx, 0.0f, (float)a_cy, -100.0f, 100.0f);
        render_texture(a, true, true);
        gs_texrender_end(s->source_a_texrender);

        gs_texrender_reset(s->source_b_texrender);
        const uint32_t b_cx = gs_texture_get_width(b), b_cy = gs_texture_get_height(b);
        gs_texrender_begin_with_color_space(s->source_b_texrender, b_cx, b_cy, source_space);
        gs_ortho(0.0f, (float)b_cx, 0.0f, (float)b_cy, -100.0f, 100.0f);
        render_texture(b, true, true);
        gs_texrender_end(s->source_b_texrender);
        gs_set_linear_srgb(prev_linear_srgb);

//        auto matA0 = extractImage(a);
//        auto matB0 = extractImage(b);
//        auto matA = extractImage(gs_texrender_get_texture(s->source_a_texrender));
//        auto matB = extractImage(gs_texrender_get_texture(s->source_b_texrender));

        for (int current_step=0; current_step < effect->nb_steps; ++current_step) {
            bool is_saved_step = effect->prev_frames_to_keep[current_step] != nullptr;

            if (is_saved_step) {
                bool texrender_ok = effect->prev_frames_to_keep[current_step]->attach(cx, cy, source_space);
                if (!texrender_ok) {
                    render_ok = false;
                    break;
                }
            }
            else {
                s->transition_texrender_buffer = (s->transition_texrender_buffer + 1) & 1;
                gs_texrender_reset(s->transition_texrender[s->transition_texrender_buffer]);
                bool texrender_ok = gs_texrender_begin_with_color_space(s->transition_texrender[s->transition_texrender_buffer], cx, cy, source_space);
                if (!texrender_ok) {
                    render_ok = false;
                    break;
                }
            }

            gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
            gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f); // This line took me A WHOLE WEEK to figure out

            gs_blend_state_push();
            gs_blend_function_separate(
                GS_BLEND_ONE, GS_BLEND_INVSRCALPHA,
                GS_BLEND_ONE, GS_BLEND_INVSRCALPHA
            );
            effect->set_params(
                gs_texrender_get_texture(s->source_a_texrender),
                gs_texrender_get_texture(s->source_b_texrender),
                s->frame_index, is_studio_mode, t, s->delta_time, cx, cy, s->rand_seed
            );
            effect->set_step_params(current_step, interm_texture);
            effect->main_shader->render(nullptr, "DrawLinear", cx, cy);

            if (is_saved_step) {
                const gs_texrender_t *next_texrender = effect->prev_frames_to_keep[current_step]->detach();
                interm_texture = gs_texrender_get_texture(next_texrender);
            }
            else {
                gs_texrender_end(s->transition_texrender[s->transition_texrender_buffer]);
                interm_texture = gs_texrender_get_texture(s->transition_texrender[s->transition_texrender_buffer]);
            }

            gs_blend_state_pop();
        }

//        if (s->frame_index == 0) {
//            auto mat = extractImage(interm_texture);
//            saveMat(mat, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_interm0.png");
//        }
//        if (s->frame_index == 1) {
//            auto mat = extractImage(interm_texture);
//            saveMat(mat, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_interm1.png");
//        }

        if (render_ok) {
            gs_set_linear_srgb(true);
            render_texture(interm_texture, false, false);
            gs_set_linear_srgb(prev_linear_srgb);
        }

        for (auto *prev_frame : effect->prev_frames_to_keep) {
            if (prev_frame != nullptr) {
                prev_frame->next_frame();
            }
        }
    }

    s->prev_t = t;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_video_render(void *data, gs_effect_t *effect) {
    debug_trace("########################### FRAME ###########################");
    UNUSED_PARAMETER(effect);
    struct shadertastic_transition *s = shadertastic_transition_cast(data);

    if (s->transition_started) {
        obs_source_t *scene_a = obs_transition_get_source(s->source, OBS_TRANSITION_SOURCE_A);
        obs_source_t *scene_b = obs_transition_get_source(s->source, OBS_TRANSITION_SOURCE_B);

        if (s->auto_reload && s->selected_effect != nullptr && shadertastic_settings().dev_mode_enabled) {
            debug("AUTO RELOAD");
            s->selected_effect->reload();
            obs_data *settings = obs_source_get_settings(s->source);
            for (auto *param: s->selected_effect->effect_params) {
                std::string full_param_name = param->get_full_param_name(s->selected_effect->name);
                param->set_data_from_settings(settings, full_param_name.c_str());
            }
            obs_source_update(s->source, nullptr);
        }

        //obs_transition_video_render(s->source, shadertastic_transition_render_init);

        info("Started transition: %s -> %s",
            obs_source_get_name(scene_a),
            obs_source_get_name(scene_b)
        );
        obs_source_release(scene_a);
        obs_source_release(scene_b);
        s->transitioning = true;
        s->transition_started = false;
    }

    float t = obs_transition_get_time(s->source);
    if (t >= 1.0f) {
        s->transitioning = false;
    }

    if (s->transitioning) {
        obs_transition_video_render2(s->source, shadertastic_transition_shader_render, shadertastic_transparent_texture);
    }
    else {
        enum obs_transition_target target = t < s->transition_point ? OBS_TRANSITION_SOURCE_A : OBS_TRANSITION_SOURCE_B;
        //debug("render direct");
        obs_transition_video_render_direct(s->source, target);
    }
}
//----------------------------------------------------------------------------------------------------------------------

static bool shadertastic_transition_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio, uint32_t mixers, size_t channels, size_t sample_rate) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    if (!s) {
        return false;
    }
    return obs_transition_audio_render(s->source, ts_out, audio, mixers, channels, sample_rate, s->mix_a, s->mix_b);
}
//----------------------------------------------------------------------------------------------------------------------

bool shadertastic_transition_properties_change_effect_callback(void *priv, obs_properties_t *props, obs_property_t *p, obs_data_t *data) {
    UNUSED_PARAMETER(p);
    shadertastic_transition *s = shadertastic_transition_cast(priv);

    if (s->selected_effect != nullptr) {
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__params").c_str()), false);
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__warning").c_str()), false);
    }

    const std::string select_effect_name = obs_data_get_string(data, "effect");
    debug("CALLBACK : %s", select_effect_name.c_str());
    auto selected_effect_it = s->effects->find(std::string(select_effect_name));
    if (selected_effect_it != s->effects->end()) {
        auto selected_effect = &selected_effect_it->second;
        obs_property_set_visible(obs_properties_get(props, (selected_effect->name + "__params").c_str()), true);
        obs_property_set_visible(obs_properties_get(props, (selected_effect->name + "__warning").c_str()), selected_effect->has_error());
        obs_source_update(s->source, nullptr);
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

bool shadertastic_transition_export_button_click(obs_properties_t *props, obs_property_t *property, void *data) {
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    auto *settings = obs_source_get_settings(s->source);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(obs_data_get_json(settings), QClipboard::Clipboard);
    QMessageBox::information(nullptr, obs_module_text("TransitionExport"), obs_module_text("TransitionExportConfirm"));

    obs_data_release(settings);
    return true;
}

bool shadertastic_transition_import_button_click(obs_properties_t *props, obs_property_t *property, void *data) {
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    auto *settings = obs_source_get_settings(s->source);

    QClipboard *clipboard = QApplication::clipboard();
    auto new_data = obs_data_create_from_json(clipboard->text().toStdString().c_str());

    if (!new_data) {
        QMessageBox::information(nullptr, obs_module_text("TransitionImport"), obs_module_text("TransitionImportError"));
        return false;
    }

    obs_source_update(s->source, obs_data_get_defaults(settings));
    obs_source_update(s->source, new_data);
    QMessageBox::information(nullptr, obs_module_text("TransitionImport"), obs_module_text("TransitionImportConfirm"));

    obs_data_release(settings);
    obs_data_release(new_data);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------

bool shadertastic_transition_reload_button_click(obs_properties_t *props, obs_property_t *property, void *data) {
    UNUSED_PARAMETER(property);
    shadertastic_transition *s = shadertastic_transition_cast(data);
    if (s->selected_effect != nullptr) {
        s->selected_effect->reload();
        obs_property_set_description(obs_properties_get(props,
            (s->selected_effect->name + "__compile_error").c_str()),
            s->selected_effect->error_str().c_str()
        );
    }
    return true;
}
//----------------------------------------------------------------------------------------------------------------------

obs_properties_t *shadertastic_transition_properties(void *data) {
    shadertastic_transition *s = shadertastic_transition_cast(data);
    obs_properties_t *props = obs_properties_create();

    obs_property_t *p;

    // auto reload settings (for development)
    if (shadertastic_settings().dev_mode_enabled) {
        //obs_property_t *auto_reload =
        obs_properties_add_bool(props, "auto_reload", obs_module_text("AutoReload"));
    }

    // audio fade settings
    obs_property_t *audio_fade_style = obs_properties_add_list(
        props, "audio_fade_style", obs_module_text("AudioFadeStyle"),
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT
    );
    obs_property_list_add_int(audio_fade_style, obs_module_text("CrossFade"), 0);
    obs_property_list_add_int(audio_fade_style, obs_module_text("FadeOutFadeIn"), 1);

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
    obs_property_set_modified_callback2(p, shadertastic_transition_properties_change_effect_callback, data);

    for (auto& [effect_name, effect] : *(s->effects)) {
        const char *effect_label = effect.label.c_str();
        obs_properties_t *effect_group = obs_properties_create();

        obs_properties_t *error_group = obs_properties_create();
        obs_properties_add_group(effect_group, (effect_name + "__warning").c_str(), "⚠ Shader error", OBS_GROUP_NORMAL, error_group);
        auto prop = obs_properties_add_text(
            error_group,
            (effect_name + "__compile_error").c_str(),
            effect.error_str().c_str(),
            OBS_TEXT_INFO
        );
        obs_property_text_set_info_type(prop, OBS_TEXT_INFO_WARNING);

        // Reload settings (for development)
        if (shadertastic_settings().dev_mode_enabled) {
            obs_properties_add_button(error_group, "reload_btn", "Refresh error message", shadertastic_transition_reload_button_click);
        }

        // Hidin error group by default. It will be shown in the update() function if required
        obs_property_set_visible(obs_properties_get(props, (effect_name + "__warning").c_str()), false);

        for (auto param: effect.effect_params) {
            std::string full_param_name = param->get_full_param_name(effect_name);
            param->render_property_ui(full_param_name.c_str(), effect_group);
        }
        obs_properties_add_group(props, (effect_name + "__params").c_str(), effect_label, OBS_GROUP_NORMAL, effect_group);
        obs_property_set_visible(obs_properties_get(props, (effect_name + "__params").c_str()), false);
    }
    if (s->selected_effect != nullptr) {
        obs_property_set_visible(obs_properties_get(props, (s->selected_effect->name + "__params").c_str()), true);
    }

    // Import/Export preferences
    obs_properties_add_button(props, "export_btn", obs_module_text("TransitionExport"), shadertastic_transition_export_button_click);
    obs_properties_add_button(props, "import_btn", obs_module_text("TransitionImport"), shadertastic_transition_import_button_click);

    about_property(props);

    return props;
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_get_defaults(obs_data_t *settings) {
    obs_data_set_default_double(settings, "transition_point", 50.0);
    obs_data_set_default_bool(settings, "auto_reload", false);

    shadertastic_transition *tmp_transition = shadertastic_transition_cast(nullptr);
    for (auto effect : *tmp_transition->effects) {
        shadertastic_effect_set_defaults(settings, &effect.second);
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_start(void *data) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    if (!s->transition_started) {
        s->rand_seed = (float)((double)rand() / (double)RAND_MAX);
        s->transition_started = true;
        s->frame_index = 0;
        debug_trace("shadertastic_transition_start %i", s->frame_index);

        if (s->selected_effect != nullptr) {
            for (auto prev_frame_param : s->selected_effect->prev_frames_to_keep) {
                if (prev_frame_param != nullptr) {
                    prev_frame_param->reset();
                }
            }
        }

        debug("Started transition of %s", obs_source_get_name(s->source));
    }
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_transition_stop(void *data) {
    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    if (s->transitioning) {
        debug("End of transition %s", obs_source_get_name(s->source));
        s->transitioning = false;
    }
}
//----------------------------------------------------------------------------------------------------------------------

static enum gs_color_space shadertastic_transition_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces) {
    UNUSED_PARAMETER(count);
    UNUSED_PARAMETER(preferred_spaces);

    struct shadertastic_transition *s = shadertastic_transition_cast(data);
    return obs_transition_video_get_color_space(s->source);
}
//----------------------------------------------------------------------------------------------------------------------

#endif // SHADERTASTIC_SHADER_TRANSITION_HPP
