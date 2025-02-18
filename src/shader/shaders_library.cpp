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

#include <obs-module.h>
#include "shaders_library.h"
#include "../logging_functions.hpp"

std::shared_ptr<effect_shader> shaders_library_t::load_shader_file(const std::string &path) {
    std::string shader_path = this->get_shader_path(path);
    debug("shaders_library.load_shader_file %s", shader_path.c_str());
    effect_shader *new_shader = new effect_shader();
    bool is_loaded = new_shader->load(shader_path.c_str());
    if (!is_loaded) {
        // Effect loading failed. Using the fallback effect to show ERR on the source
        delete new_shader;
        auto emplaced = shaders.emplace(path, fallback_shader);
        return emplaced.first->second;
    }
    else {
        auto emplaced = shaders.emplace(path, new_shader);
        return emplaced.first->second;
    }
}

void shaders_library_t::load() {
    char *fallback_shader_path = obs_module_file("effects/fallback_effect.hlsl");
    debug("fallback_shader_path %s", fallback_shader_path);
    fallback_shader = std::make_shared<effect_shader>();
    fallback_shader->load(fallback_shader_path);
    bfree(fallback_shader_path);
}

std::shared_ptr<effect_shader> shaders_library_t::get(const std::string &path) {
    debug("shaders_library.get %s", path.c_str());
    auto it = shaders.find(path);
    if (it == shaders.end()) {
        return this->load_shader_file(path);
    }
    else {
        return it->second;
    }
}

std::string shaders_library_t::get_shader_path(const std::string &path) {
    return (path + "/main.hlsl");
}

void shaders_library_t::reload(const std::string &path) {
    debug("Reloading Shader '%s'...", path.c_str());
    std::shared_ptr<effect_shader> shader_to_delete = this->get(path);
    shaders.erase(path);
    this->load_shader_file(path);
    debug("Reloading Shader DONE '%s'", path.c_str());
}
