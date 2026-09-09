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

#ifndef SHADERTASTIC_BLUEPRINTS_LIBRARY_H
#define SHADERTASTIC_BLUEPRINTS_LIBRARY_H

#include <string>
#include <map>
#include "effect.h"

class BlueprintsLibrary {
    private:
        std::map<std::string, shadertastic_effect_blueprint_t *> blueprints;
    public:
        bool hasBlueprint(const std::string &name);
        shadertastic_effect_blueprint_t * getBlueprint(const std::string &name);
        shadertastic_effect_blueprint_t * loadBlueprint(const std::string &name, const std::string &path);
        void removeBlueprint(const std::string &name);
};

extern BlueprintsLibrary blueprints_library;

#endif //SHADERTASTIC_BLUEPRINTS_LIBRARY_H
