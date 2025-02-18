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

#ifndef SHADERTASTIC_SHADERS_LIBRARY_HPP
#define SHADERTASTIC_SHADERS_LIBRARY_HPP

#include <map>
#include <memory>
#include <string>

#include "shader.h"

class shaders_library_t {
    private:
        std::map<std::string, std::shared_ptr<effect_shader>> shaders;
        std::shared_ptr<effect_shader> fallback_shader;

        std::shared_ptr<effect_shader> load_shader_file(const std::string &path);

    public:
        void load();

        std::shared_ptr<effect_shader> get(const std::string &path);

        std::string get_shader_path(const std::string &path);

        void reload(const std::string &path);
};

inline shaders_library_t shaders_library;

#endif /* SHADERTASTIC_SHADERS_LIBRARY_HPP */
