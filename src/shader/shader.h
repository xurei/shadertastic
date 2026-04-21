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

#ifndef SHADERTASTIC_SHADER_H
#define SHADERTASTIC_SHADER_H

#include <obs-module.h>

class effect_shader {
    private:
    gs_effect_t *gs_effect = nullptr;

    public:
    // 0 -> 1 with transitions; time elapsed with filters when enabled
    gs_eparam_t *param_time = nullptr;

    // Transition only
    gs_eparam_t *param_tex_a = nullptr;
    gs_eparam_t *param_tex_b = nullptr;
    gs_eparam_t *param_is_studio_mode = nullptr;

    // Transition and filter
    gs_eparam_t *param_tex_interm = nullptr;
    gs_eparam_t *param_frame_index = nullptr;
    gs_eparam_t *param_delta_time = nullptr;
    gs_eparam_t *param_upixel = nullptr;
    gs_eparam_t *param_vpixel = nullptr;
    gs_eparam_t *param_rand_seed = nullptr;
    gs_eparam_t *param_current_step = nullptr;
    gs_eparam_t *param_nb_steps = nullptr;

    std::string path{};

    ~effect_shader();

    std::string load(const char *shader_path);

    [[nodiscard]] gs_eparam_t * get_param_by_name(const char *param_name) const;
    [[nodiscard]] gs_eparam_t * get_param_by_name(const std::string &param_name) const;
    bool loop(const char *tech_name);

    //void render(gs_texrender_t *texrender, uint32_t cx, uint32_t cy);
    void render(obs_source_t *filter, gs_texrender_t *texrender, uint32_t cx, uint32_t cy);

    void release();

    //See cpp file for details
    //private:
    //static bool sourceHasValidBraces(const std::string& str);
    //static std::string removeComments(const std::string& str);
};

#endif // SHADERTASTIC_SHADER_H
