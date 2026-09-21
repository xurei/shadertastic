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

#include "blueprints_library.h"
#include "effect.h"

BlueprintsLibrary blueprints_library;

bool BlueprintsLibrary::hasBlueprint(const std::string &effect_type, const std::string &name) {
    std::string blueprint_name = effect_type + ":" + name;
    return !(this->blueprints.find(blueprint_name) == this->blueprints.end());
}

shadertastic_effect_blueprint_t * BlueprintsLibrary::getBlueprint(const std::string &effect_type, const std::string &name) {
    std::string blueprint_name = effect_type + ":" + name;
    if (!this->hasBlueprint(effect_type, name)) {
        return nullptr;
    }
    return this->blueprints[blueprint_name];
}

shadertastic_effect_blueprint_t * BlueprintsLibrary::loadBlueprint(const std::string &effect_type, const std::string &name, const std::string &path) {
    std::string blueprint_name = effect_type + ":" + name;
    shadertastic_effect_blueprint_t *blueprint = new shadertastic_effect_blueprint_t(name, path);
    blueprint->load_blueprint();

    if (blueprint->is_loaded()) {
        this->blueprints[blueprint_name] = blueprint;
        return blueprint;
    }
    else {
        return nullptr;
    }
}

void BlueprintsLibrary::removeBlueprint(const std::string &effect_type, const std::string &name) {
    if (hasBlueprint(effect_type, name)) {
        delete this->blueprints[name];
        this->blueprints.erase(name);
    }
}
