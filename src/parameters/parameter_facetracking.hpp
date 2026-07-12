/******************************************************************************
    Copyright (C) 2025 by xurei <xureilab@gmail.com>

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

#ifndef SHADERTASTIC_PARAMETER_FACETRACKING_HPP
#define SHADERTASTIC_PARAMETER_FACETRACKING_HPP

#include <string>
#include "parameter.hpp"
#include "onnxmediapipe/face_landmarks_triangles.h"
#include "src/face_tracking/face_tracking_state.h"
#include "src/face_tracking/face_tracking.h"
#include "src/face_tracking/face_tracking_raster.h"

class effect_parameter_facetracking : public effect_parameter {
    private:
        static constexpr char PARAM_STR_FACE_FOUND[] = "face_found";
        static constexpr char PARAM_STR_BOX_TL[] = "bbox_tl";
        static constexpr char PARAM_STR_BOX_BR[] = "bbox_br";
        static constexpr char PARAM_STR_POINTS_TEX[] = "points_tex";
        static constexpr char PARAM_STR_PRERASTER_TEX[] = "preraster_tex";

        // Weak access to the unique_ptr of the face_tracking
        face_tracking_state *face_tracking{};

        bool use_preraster{false};

        gs_eparam_t *param_fd_face_found{};
        gs_eparam_t *param_fd_face_tl{};
        gs_eparam_t *param_fd_face_br{};
        gs_eparam_t *param_fd_points_tex{};
        gs_eparam_t *param_fd_preraster_tex{};

        uint32_t cx{0};
        uint32_t cy{0};

        static constexpr face_tracking_bounding_box no_bounding_box{
            -1.0f, -1.0f
            -1.0f, -1.0f
        };

    public:
        explicit effect_parameter_facetracking(const effect_shader *shader) : effect_parameter(shader) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_FACETRACKING;
        }

        void initialize_params(const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
                        UNUSED_PARAMETER(effect_path);

            std::string face_found = get_full_subparam_name_static(name, PARAM_STR_FACE_FOUND);
            std::string bbox_tl = get_full_subparam_name_static(name, PARAM_STR_BOX_TL);
            std::string bbox_br = get_full_subparam_name_static(name, PARAM_STR_BOX_BR);
            std::string points_tex = get_full_subparam_name_static(name, PARAM_STR_POINTS_TEX);
            std::string preraster_tex = get_full_subparam_name_static(name, PARAM_STR_PRERASTER_TEX);

            param_fd_face_found = shader->get_param_by_name(face_found);
            param_fd_face_tl = shader->get_param_by_name(bbox_tl);
            param_fd_face_br = shader->get_param_by_name(bbox_br);
            param_fd_points_tex = shader->get_param_by_name(points_tex);

            // use_preraster field
            json_t *use_preraster_json = json_object_get(metadata, "use_preraster");
            this->use_preraster = json_is_boolean(use_preraster_json) ? json_boolean_value(use_preraster_json) : false;
            if (this->use_preraster) {
                param_fd_preraster_tex = shader->get_param_by_name(preraster_tex);
            }
        }

        void tick(shadertastic_common *s) override {
            face_tracking = s->face_tracking.get();
            cx = obs_source_get_width(s->source);
            cy = obs_source_get_height(s->source);
        }

        void set_default(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
        }

        void try_gs_set_val() override {
            if (!face_tracking) {
                return;
            }

            std::string face_found = get_full_subparam_name_static(name, PARAM_STR_FACE_FOUND);
            std::string bbox_tl = get_full_subparam_name_static(name, PARAM_STR_BOX_TL);
            std::string bbox_br = get_full_subparam_name_static(name, PARAM_STR_BOX_BR);
            std::string points_tex = get_full_subparam_name_static(name, PARAM_STR_POINTS_TEX);

            if (!face_tracking->facelandmark_results_display_results) {
                try_gs_effect_set_bool(face_found.c_str(), param_fd_face_found, false);
                try_gs_effect_set_vec2(bbox_tl.c_str(), param_fd_face_tl, &no_bounding_box.point1);
                try_gs_effect_set_vec2(bbox_br.c_str(), param_fd_face_br, &no_bounding_box.point2);
            }
            else {
                try_gs_effect_set_bool(face_found.c_str(), param_fd_face_found, true);
                {
                    auto bbox = face_tracking_get_bounding_box(&face_tracking->average_results, not_lips_eyes_indices, 310);
                    try_gs_effect_set_vec2(bbox_tl.c_str(), param_fd_face_tl, &bbox.point1);
                    try_gs_effect_set_vec2(bbox_br.c_str(), param_fd_face_br, &bbox.point2);
                    //debug("Face: %f %f %f %f", bbox.x1, bbox.y1, bbox.x2-bbox.x1, bbox.y2-bbox.y1);
                }
                try_gs_effect_set_texture(points_tex.c_str(), param_fd_points_tex, face_tracking->fd_points_texture);

                //Pre-raster texture
                if (use_preraster) {
                    auto *fd_preraster_texture = FaceTrackingRaster::mesh_uv_gpu(face_tracking->average_results.refined_landmarks, onnxmediapipe::face_triangles, (int)cx, (int)cy);
                    try_gs_effect_set_texture(points_tex.c_str(), param_fd_preraster_tex, fd_preraster_texture);
                }
            }
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            UNUSED_PARAMETER(effect_name);
            UNUSED_PARAMETER(props);
            // TODO faudrait ptetre mettre un message de warning, ou alors si on fait le truc de recyclage mais faut guider l'user
        }

        void set_visible(const bool visible) override {
            UNUSED_PARAMETER(visible);
        }
        [[nodiscard]] bool is_visible() const override {
            return true;
        }

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            UNUSED_PARAMETER(settings);
            UNUSED_PARAMETER(effect_name);
        }
};

#endif // SHADERTASTIC_PARAMETER_FACETRACKING_HPP
