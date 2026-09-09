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

#ifndef SHADERTASTIC_EFFECT_H
#define SHADERTASTIC_EFFECT_H

#include <memory>
#include <utility>
#include <vector>
#include <jansson.h>

#include "params_list.hpp"
#include "shader/shader.h"
#include "parameters/parameter_prev_frame.hpp"
#include "src/shader/shaders_library.h"

struct shadertastic_effect_blueprint_t {
    const std::string path;
    const std::string name;
    std::string label;
    int nb_steps{};
    bool legacy_input_time = false;
    bool legacy_input_facedetection = false;

    protected:
    friend struct shadertastic_effect_t;
    json_t *metadata{nullptr};

    public:
    void load_blueprint();

    shadertastic_effect_blueprint_t(std::string name_, std::string path_): name(std::move(name_)), path(std::move(path_)) {}

    virtual ~shadertastic_effect_blueprint_t() {
        //release_resource(json_decref, metadata);
    }
};

struct shadertastic_effect_t : public shadertastic_effect_blueprint_t {
    std::vector<effect_parameter_prev_frame *> prev_frames_to_keep{};
    effect_parameter *param_facetracking{};

    params_list effect_params;
    std::unordered_map<std::string, std::list<effect_parameter *>> referenced_params;
    std::shared_ptr<effect_shader> main_shader{};

    [[nodiscard]] inline bool has_error() const {
        return shaders_library.has_error(this->path);
    }
    [[nodiscard]] inline std::string error_str() const {
        return shaders_library.get_error(this->path);
    }

    explicit shadertastic_effect_t(const std::string &name_, const std::string &path_): shadertastic_effect_blueprint_t(name_, path_) {}

    static shadertastic_effect_t from_blueprint(const shadertastic_effect_blueprint_t &blueprint) {
        shadertastic_effect_t out(blueprint.name, blueprint.path);
        out.label = blueprint.label;
        out.nb_steps = blueprint.nb_steps;
        out.metadata = blueprint.metadata;
        out.legacy_input_time = blueprint.legacy_input_time;
        out.legacy_input_facedetection = blueprint.legacy_input_facedetection;
        json_incref(out.metadata);
        return out;
    }
    void load();

    void reload();

    void attach_listeners(obs_properties *props);
    void create_properties(obs_properties *props);

    void set_params(gs_texture_t *a, gs_texture_t *b, int frame_index, bool is_studio_mode, float t, float delta_t, uint32_t cx, uint32_t cy, float rand_seed);

    void set_step_params(int current_step, gs_texture_t *interm) const;

    void show();

    void hide();

    void release();

    virtual ~shadertastic_effect_t() {
        /*if (metadata != nullptr && json_is_object(metadata)) {
            release_resource(json_decref, metadata);
        }*/
    }
};

#endif // SHADERTASTIC_EFFECT_H
