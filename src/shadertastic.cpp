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
#include <vector>
#include <list>
#include <map>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QAction>
#include <QVBoxLayout>
#include <util/platform.h>

#include "version.h"

#ifdef DEV_MODE
// ReSharper disable once CppUnusedIncludeDirective
#include "util/debug_util.hpp"
#endif

#include "effect.h"
#include "is_module_loaded.h"
#include "logging_functions.hpp"
#include "settings.h"
#include "shader/shaders_library.h"
#include "shadertastic.hpp"
#include "util/file_util.h"
#include "shader_transition.hpp"
// ReSharper disable once CppUnusedIncludeDirective
#include "util/time_util.hpp"
//----------------------------------------------------------------------------------------------------------------------

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("xurei")
OBS_MODULE_USE_DEFAULT_LOCALE("shadertastic", "en-US")
//----------------------------------------------------------------------------------------------------------------------

bool module_loaded = false;
bool is_direct3d = false;
gs_texture_t *shadertastic_transparent_texture{};

bool shadertastic_is_direct3d() {
    return is_direct3d;
}
//----------------------------------------------------------------------------------------------------------------------

obs_source_t *get_transition(obs_frontend_source_list &transitions, const char *transition_name) {
    for (size_t i = 0; i < transitions.sources.num; i++) {
        if (strcmp(obs_source_get_name(transitions.sources.array[i]), transition_name) == 0) {
            return transitions.sources.array[i];
        }
    }
    return nullptr;
}

void obs_module_frontend_saveload(obs_data_t *save_data, bool saving, void *data)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(saving);
    UNUSED_PARAMETER(save_data);
    obs_frontend_source_list transitions = {0};
    obs_frontend_get_transitions(&transitions);

    if (saving) {
        auto *transi_backup_array = obs_data_array_create();
        for (size_t i = 0; i < transitions.sources.num; i++) {
            /* Do NOT call `obs_source_release` or `obs_scene_release`
             * on these sources
             */
            obs_source_t *transi = transitions.sources.array[i];
            if (strcmp(obs_source_get_id(transi), SHADERTASTIC_TRANSITION_NAME) == 0) {
                auto *settings = obs_source_get_settings(transi);
                if (settings != nullptr) {
                    auto *obj = obs_data_create();
                    obs_data_set_string(obj, "id", SHADERTASTIC_TRANSITION_NAME);
                    obs_data_set_string(obj, "name", obs_source_get_name(transi));
                    obs_data_set_obj(obj, "settings", settings);
                    obs_data_array_push_back(transi_backup_array, obj);
                    obs_data_release(settings);
                    obs_data_release(obj);
                }
            }
        }
        obs_data_set_array(save_data, "shadertastic_transitions", transi_backup_array);
        obs_data_array_release(transi_backup_array);
//        debug("%s", obs_data_get_json(save_data));
//        debug("saved settings");
    }
    else {
        auto *transi_backup_array = obs_data_get_array(save_data, "shadertastic_transitions");
        if (transi_backup_array != nullptr) {
            // Comparing the transitions with the saved ones, adding those missing if they are not present
            // This makes sure the transitions are not lost when the plugin is absent or in safe mode.
            size_t transitions_count = obs_data_array_count(transi_backup_array);
            for (size_t i = 0; i < transitions_count; i++) {
                auto *transi = obs_data_array_item(transi_backup_array, i);
                if (transi != nullptr) {
                    const char *transi_name = obs_data_get_string(transi, "name");
                    UNUSED_PARAMETER(transi_name);
                    auto *settings = obs_data_get_obj(transi, "settings");
                    if (transi_name != nullptr) {
                        auto *obs_transi = get_transition(transitions, transi_name);
                        if (obs_transi == nullptr) {
                            obs_transi = obs_frontend_add_transition(SHADERTASTIC_TRANSITION_NAME, transi_name, settings);
                            release_resource(obs_source_release, obs_transi);
                            debug("ok");
                        }
                    }
                    release_resource(obs_data_release, settings);
                    obs_data_release(transi);
                }
            }
            obs_data_array_release(transi_backup_array);
        }
    }
    obs_frontend_source_list_free(&transitions);
}

void load_effects(shadertastic_common *s, obs_data_t *settings, const std::string &effects_dir, const std::string &effects_type) {
    std::vector<std::string> dirs = list_directories((effects_dir + "/" + effects_type + "s").c_str());

    for (const auto &dir : dirs) {
        std::string effect_path = std::string(effects_dir)
            .append("/")
            .append(effects_type)
            .append("s/")
            .append(dir);
        if (s->effects->find(dir) != s->effects->end()) {
            warn("NOT LOADING EFFECT %s/%ss/%s : an effect with the name '%s' already exist", effects_dir.c_str(), effects_type.c_str(), dir.c_str(), dir.c_str());
        }
        else if (os_file_exists((effect_path + "/meta.json").c_str())) {
            shadertastic_effect_t effect(dir, effect_path);
            effect.load();
            if (effect.main_shader == nullptr) {
                debug ("NOT LOADING EFFECT %s", dir.c_str());
            }
            else {
                s->effects->insert(shadertastic_effects_map_t::value_type(dir, effect));
                shadertastic_effect_set_defaults(settings, &effect);
            }
        }
        else {
            debug ("NOT LOADING EFFECT %s : no meta.json found", dir.c_str());
        }
    }

    std::string extension = std::string(".") + effects_type + ".shadertastic";
    std::vector<std::string> zips = list_files(effects_dir.c_str(), extension);
    for (const auto &zip : zips) {
        fs::path fs_path(zip);
        std::string effect_name = fs_path.filename().string();
        effect_name = effect_name.substr(0, effect_name.length() - extension.length());
        std::string effect_path = zip;

        if (s->effects->find(effect_name) != s->effects->end()) {
            warn("NOT LOADING EFFECT %s : an effect with the name '%s' already exist", zip.c_str(), effect_name.c_str());
        }
        else if (true /*shadertastic_effect_t::is_effect(effect_path)*/) {
            // TODO the check that meta.json exists is missing in archived effects... I should add it back
            //debug("Effect %s: %s", effect_name.c_str(), effect_path.c_str());
            shadertastic_effect_t effect(effect_name, effect_path);
            effect.load();
            if (effect.main_shader == nullptr) {
                debug ("NOT LOADING EFFECT %s", zip.c_str());
            }
            else {
                s->effects->insert(shadertastic_effects_map_t::value_type(effect_name, effect));
                shadertastic_effect_set_defaults(settings, &effect);
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

#include "shader_filter.hpp"
#include "shader_transition.hpp"
#include "src/face_tracking/face_tracking_raster.h"
//----------------------------------------------------------------------------------------------------------------------

[[maybe_unused]] bool obs_module_load(void) {
    info("loaded version %s", PROJECT_VERSION);
    srand(time(nullptr));
    obs_data_t *settings = load_settings();
    apply_settings(settings);
    obs_data_release(settings);

    FaceTrackingCropShader::init();

    shaders_library.load();

    uint8_t transparent_tex_data[2 * 2 * 4] = {0};
    const uint8_t *transparent_tex = transparent_tex_data;
    obs_enter_graphics();
    shadertastic_transparent_texture = gs_texture_create(2, 2, GS_RGBA, 1, &transparent_tex, 0);
    obs_leave_graphics();

    struct obs_source_info shadertastic_transition_info = {};
    shadertastic_transition_info.id = SHADERTASTIC_TRANSITION_NAME;
    shadertastic_transition_info.type = OBS_SOURCE_TYPE_TRANSITION;
    shadertastic_transition_info.get_name = shadertastic_transition_get_name;
    shadertastic_transition_info.create = shadertastic_transition_create;
    shadertastic_transition_info.destroy = shadertastic_transition_destroy;
    shadertastic_transition_info.get_properties = shadertastic_transition_properties;
    shadertastic_transition_info.get_defaults = shadertastic_transition_get_defaults;
    shadertastic_transition_info.update = shadertastic_transition_update;
    shadertastic_transition_info.video_tick = shadertastic_transition_tick;
    shadertastic_transition_info.video_render = shadertastic_transition_video_render;
    shadertastic_transition_info.load = shadertastic_transition_update;
    shadertastic_transition_info.audio_render = shadertastic_transition_audio_render;
    shadertastic_transition_info.transition_start = shadertastic_transition_start;
    shadertastic_transition_info.transition_stop = shadertastic_transition_stop;
    //shadertastic_transition_info.video_tick = shadertastic_transition_tick;
    shadertastic_transition_info.video_get_color_space = shadertastic_transition_get_color_space;
    obs_register_source(&shadertastic_transition_info);

    struct obs_source_info shadertastic_filter_info = {};
    shadertastic_filter_info.id = SHADERTASTIC_FILTER_NAME;
    shadertastic_filter_info.type = OBS_SOURCE_TYPE_FILTER;
    shadertastic_filter_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW /*| OBS_SOURCE_SRGB *//*| OBS_SOURCE_COMPOSITE*/;
    shadertastic_filter_info.get_name = shadertastic_filter_get_name;
    shadertastic_filter_info.create = shadertastic_filter_create;
    shadertastic_filter_info.destroy = shadertastic_filter_destroy;
    shadertastic_filter_info.get_properties = shadertastic_filter_properties;
    shadertastic_filter_info.get_defaults = shadertastic_filter_get_defaults;
    shadertastic_filter_info.video_tick = shadertastic_filter_tick;
    shadertastic_filter_info.update = shadertastic_filter_update;
    shadertastic_filter_info.get_width = shadertastic_filter_getwidth,
    shadertastic_filter_info.get_height = shadertastic_filter_getheight,
    shadertastic_filter_info.video_render = shadertastic_filter_video_render;
    shadertastic_filter_info.load = shadertastic_filter_update;
    shadertastic_filter_info.show = shadertastic_filter_show;
    shadertastic_filter_info.hide = shadertastic_filter_hide;
    shadertastic_filter_info.video_get_color_space = shadertastic_filter_get_color_space;
    obs_register_source(&shadertastic_filter_info);

    QAction *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(obs_module_text("Shadertastic Settings")));
    QObject::connect(action, &QAction::triggered, show_settings_dialog);

    module_loaded = true;

    obs_video_info video_info;
    obs_get_video_info(&video_info);
    info("Video renderer: %s", video_info.graphics_module);
    std::string graphics_module = video_info.graphics_module;
    std::transform(graphics_module.begin(), graphics_module.end(), graphics_module.begin(),
        [](unsigned char c){ return std::tolower(c); });
    if (graphics_module.find("opengl") == std::string::npos) {
        is_direct3d = true;
    }

    obs_frontend_add_save_callback(obs_module_frontend_saveload, nullptr);

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

bool is_module_loaded() {
    return module_loaded;
}

[[maybe_unused]] void obs_module_unload(void) {
    module_loaded = false;
    shadertastic_filter_unload();

    obs_enter_graphics();
    {
        release_resource(gs_texture_destroy, shadertastic_transparent_texture);
    }
    obs_leave_graphics();

    FaceTrackingRaster::unload();
    FaceTrackingCropShader::release();
}
//----------------------------------------------------------------------------------------------------------------------

const char *shadertastic_transition_get_name(void *type_data) {
    UNUSED_PARAMETER(type_data);
    return obs_module_text("TransitionName");
}
const char *shadertastic_filter_get_name(void *type_data) {
    UNUSED_PARAMETER(type_data);
    return obs_module_text("FilterName");
}
//----------------------------------------------------------------------------------------------------------------------

void shadertastic_effect_set_defaults(obs_data_t *settings, shadertastic_effect_t *effect) {
    if (effect == nullptr || settings == nullptr) {
        return;
    }

    for (auto param: effect->effect_params) {
        param->set_default(settings, effect->name.c_str());
    }
}
//----------------------------------------------------------------------------------------------------------------------

void about_property(obs_properties_t *props) {
    obs_properties_add_text(
        props,
        "plugin_info",
        "<a href=\"https://shadertastic.com\">Shadertastic</a> (" PROJECT_VERSION ") by <a href=\"http://about.xurei.io/\">xurei</a>",
        OBS_TEXT_INFO
    );
}
//----------------------------------------------------------------------------------------------------------------------

MODULE_EXPORT [[maybe_unused]] const char *obs_module_name(void) {
    return obs_module_text("ModuleName");
}
//----------------------------------------------------------------------------------------------------------------------

MODULE_EXPORT [[maybe_unused]] const char *obs_module_description(void) {
    return obs_module_text("ModuleDescription");
}
//----------------------------------------------------------------------------------------------------------------------
