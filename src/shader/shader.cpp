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
#include <string>
#include "../logging_functions.hpp"
#include "../util/file_util.h"
#include "../is_module_loaded.h"
#include "shader.h"

effect_shader::~effect_shader() {
    debug("DELETE effect_shader %s", path.c_str());
    this->release();
}

bool effect_shader::load(const char *shader_path) {
    debug("SHADER PATH: %s", shader_path);
    this->path = std::string(shader_path);
    char *error_string = nullptr;
    char *shader_source_ = load_file_zipped_or_local(shader_path);
    if (shader_source_ == nullptr) {
        log_error("Could not open shader file %s : File does not exist", shader_path);
        return false;
    }
    else {
        std::string shader_source = std::string(shader_source_);
        bfree(shader_source_);

        // Global common arguments, must be in all the effects for this plugin
        shader_source = (std::string("") +
            "uniform float4x4 ViewProj;\n" +
            "uniform texture2d image;\n" +
            "uniform texture2d tex_a;\n" +
            "uniform texture2d tex_b;\n" +
            "uniform texture2d tex_interm;\n" +
            "uniform float time;\n" +
            "uniform float upixel;\n" +
            "uniform float vpixel;\n" +
            "uniform float rand_seed;\n" +
            "uniform int current_step;\n" +
            "uniform int nb_steps;\n" +
            "float srgb_nonlinear_to_linear_channel(float u) { return (u <= 0.04045) ? (u / 12.92) : pow((u + 0.055) / 1.055, 2.4); }\n" +
            "float3 srgb_nonlinear_to_linear(float3 v) { return float3(srgb_nonlinear_to_linear_channel(v.r), srgb_nonlinear_to_linear_channel(v.g), srgb_nonlinear_to_linear_channel(v.b)); }\n" +
            "#ifdef _D3D11 \n #define fract(a) frac(a) \n #else \n #define frac(a) fract(a) \n #endif \n" +
            shader_source
        );
        obs_enter_graphics();
        gs_effect = gs_effect_create(shader_source.c_str(), shader_path, &error_string);
        obs_leave_graphics();

        if (!gs_effect) {
            log_error("Could not open shader file %s : %s", shader_path, error_string);
            bfree(error_string);
            return false;
        }
        else {
            param_tex_a = gs_effect_get_param_by_name(gs_effect, "tex_a");
            param_tex_b = gs_effect_get_param_by_name(gs_effect, "tex_b");
            param_tex_interm = gs_effect_get_param_by_name(gs_effect, "tex_interm");
            param_time = gs_effect_get_param_by_name(gs_effect, "time");
            param_upixel = gs_effect_get_param_by_name(gs_effect, "upixel");
            param_vpixel = gs_effect_get_param_by_name(gs_effect, "vpixel");
            param_rand_seed = gs_effect_get_param_by_name(gs_effect, "rand_seed");
            param_current_step = gs_effect_get_param_by_name(gs_effect, "current_step");
            param_nb_steps = gs_effect_get_param_by_name(gs_effect, "nb_steps");

            param_fd_face_found = gs_effect_get_param_by_name(gs_effect, "fd_face_found");
            param_fd_leye_1 = gs_effect_get_param_by_name(gs_effect, "fd_leye_1");
            param_fd_leye_2 = gs_effect_get_param_by_name(gs_effect, "fd_leye_2");
            param_fd_reye_1 = gs_effect_get_param_by_name(gs_effect, "fd_reye_1");
            param_fd_reye_2 = gs_effect_get_param_by_name(gs_effect, "fd_reye_2");
            param_fd_face_1 = gs_effect_get_param_by_name(gs_effect, "fd_face_1");
            param_fd_face_2 = gs_effect_get_param_by_name(gs_effect, "fd_face_2");
            param_fd_points_tex = gs_effect_get_param_by_name(gs_effect, "fd_points_tex");

            return true;
        }
    }
}

bool effect_shader::loop(const char *tech_name) {
    return gs_effect_loop(gs_effect, tech_name);
}

gs_eparam_t *effect_shader::get_param_by_name(const char *param_name) {
    return gs_effect_get_param_by_name(gs_effect, param_name);
}

void effect_shader::render(obs_source_t *filter, uint32_t cx, uint32_t cy) {
    obs_source_process_filter_end(filter, gs_effect, cx, cy);
}

void effect_shader::release() {
    if (is_module_loaded() && gs_effect != nullptr) {
        debug("Release shader");
        obs_enter_graphics();
        gs_effect_destroy(gs_effect);
        gs_effect = nullptr;
        obs_leave_graphics();
    }
}

// These were an attempt to check the syntax of the shader, and avoid crashes during effect development
// But it is too simple and creates other weird problems (e.g. /* //bla */)
//bool effect_shader::sourceHasValidBraces(const std::string& str) {
//    int counter = 0;
//
//    for (char ch : str) {
//        if (ch == '{') {
//            counter++;
//        }
//        else if (ch == '}') {
//            counter--;
//            // If counter goes negative, there's a mismatched closing brace
//            if (counter < 0) {
//                log_error("COUNTER IS NEGATIVE");
//                return false;
//            }
//        }
//    }
//
//    if (counter != 0) {
//        log_error("COUNTER IS NOT ZERO");
//    }
//
//    // The string is valid if the counter is zero at the end
//    return counter == 0;
//}
//
//std::string effect_shader::removeComments(const std::string& code) {
//    // Regular expression to match single-line and multi-line comments
//    std::regex singleLineComment(R"(//.*?$)", std::regex_constants::multiline);
//    std::regex multiLineComment(R"(/\*.*?\*/)");
//
//    // Create a copy of the input code to modify
//    std::string result = code;
//
//    // Remove single-line comments
//    result = std::regex_replace(result, singleLineComment, "");
//
//    // Remove multi-line comments while preserving new lines
//    std::sregex_iterator it(result.begin(), result.end(), multiLineComment);
//    std::sregex_iterator end;
//    for (; it != end; ++it) {
//        std::string match = it->str();
//        size_t newlineCount = std::count(match.begin(), match.end(), '\n');
//        result.replace(it->position(), match.length(), std::string(newlineCount, '\n'));
//    }
//
//    return result;
//}
