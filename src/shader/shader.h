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

class effect_shader {
    private:
    gs_effect_t *gs_effect = nullptr;

    public:
    gs_eparam_t *param_tex_a = nullptr;
    gs_eparam_t *param_tex_b = nullptr;
    gs_eparam_t *param_tex_interm = nullptr;
    gs_eparam_t *param_time = nullptr;
    gs_eparam_t *param_upixel = nullptr;
    gs_eparam_t *param_vpixel = nullptr;
    gs_eparam_t *param_rand_seed = nullptr;
    gs_eparam_t *param_current_step = nullptr;
    gs_eparam_t *param_nb_steps = nullptr;

    gs_eparam_t *param_fd_face_found = nullptr;
    gs_eparam_t *param_fd_leye_1 = nullptr;
    gs_eparam_t *param_fd_leye_2 = nullptr;
    gs_eparam_t *param_fd_reye_1 = nullptr;
    gs_eparam_t *param_fd_reye_2 = nullptr;
    gs_eparam_t *param_fd_face_1 = nullptr;
    gs_eparam_t *param_fd_face_2 = nullptr;
    gs_eparam_t *param_fd_points_tex = nullptr;

    std::string path;

    ~effect_shader();

    bool load(const char *shader_path);

    gs_eparam_t * get_param_by_name(const char *param_name);
    bool loop(const char *tech_name);

    void render(obs_source_t *filter, uint32_t cx, uint32_t cy);

    void release();

    //See cpp file for details
    //private:
    //static bool sourceHasValidBraces(const std::string& str);
    //static std::string removeComments(const std::string& str);
};

#endif /* SHADERTASTIC_SHADER_H */
