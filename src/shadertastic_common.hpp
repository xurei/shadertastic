#ifndef SHADERTASTIC_COMMON_HPP
#define SHADERTASTIC_COMMON_HPP

#include <map>
#include <string>
#include <obs-module.h>
#include "face_tracking/face_tracking_state.h"
#include "effect.h"
#include "util/resource_util.hpp"

typedef std::map<std::string, shadertastic_effect_t> shadertastic_effects_map_t;
extern gs_texture_t *shadertastic_transparent_texture;

struct shadertastic_common {
    // !!! Do not add constructors or destructors. OBS handles this with bzalloc which won't execute them

    shadertastic_effects_map_t *effects{}; // Pointer is required here, otherwise the map gets corrupted (by OBS ?)
    shadertastic_effect_t *selected_effect{};
    obs_source_t *parent_source{};
    obs_source_t *source{};
    int frame_index = 0;
    float time = 0.0;
    float delta_time = 0.0;

    // Face tracking internal state
    std::unique_ptr<face_tracking_state> face_tracking{};

    // Filter previous state (enabled/disabled)
    bool was_enabled = false;

    bool must_render = true;

    void release();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // SHADERTASTIC_COMMON_HPP
