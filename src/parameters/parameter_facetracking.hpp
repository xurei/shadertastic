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

namespace effect_parameter_facetracking_detail {
    using base_param = effect_parameter;

    enum reset_type_t {
        CHOICE_NO = 0,
        CHOICE_YES = 1,
        CHOICE_PROMPT = 2,
    };
}
class effect_parameter_facetracking : public effect_parameter_facetracking_detail::base_param {
    private:
        static constexpr char PARAM_STR_FACE_FOUND[] = "face_found";
        static constexpr char PARAM_STR_BOX_TL[] = "bbox_tl";
        static constexpr char PARAM_STR_BOX_BR[] = "bbox_br";
        static constexpr char PARAM_STR_POINTS_TEX[] = "points_tex";
        static constexpr char PARAM_STR_PRERASTER_TEX[] = "preraster_tex";
        static constexpr char PARAM_STR_RESET_ON_FACE_LOST[] = "reset_on_face_lost";

        // Weak access to the unique_ptr of the face_tracking
        face_tracking_state *face_tracking{};

        bool use_preraster{false};

        effect_parameter_facetracking_detail::reset_type_t reset_on_face_lost_type{effect_parameter_facetracking_detail::CHOICE_NO};
        bool reset_on_face_lost{false};
        bool default_reset_on_face_lost{true};

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
        explicit effect_parameter_facetracking(const effect_shader *shader) : effect_parameter_facetracking_detail::base_param(shader) {
        }

        [[nodiscard]] effect_param_datatype type() const override {
            return PARAM_DATATYPE_FACETRACKING;
        }

        void initialize_params(const std::string &effect_name, const effect_shader *shader, json_t *metadata, const std::string &effect_path) override {
            UNUSED_PARAMETER(effect_name);
            UNUSED_PARAMETER(effect_path);

            json_t *reset_on_face_lost_obj = json_object_get(metadata, PARAM_STR_RESET_ON_FACE_LOST);
            if (json_is_object(reset_on_face_lost_obj)) {
                // TODO label ?
                //json_t *label_json = json_object_get(reset_on_face_lost_obj, "label");
                json_t *default_json = json_object_get(reset_on_face_lost_obj, "default");

                reset_on_face_lost_type = effect_parameter_facetracking_detail::CHOICE_PROMPT;
                default_reset_on_face_lost = (bool)json_bool_value_or(default_json, true);
            }
            else if (json_is_string(reset_on_face_lost_obj) && strcmp(json_string_value(reset_on_face_lost_obj), "prompt") == 0) {
                reset_on_face_lost_type = effect_parameter_facetracking_detail::CHOICE_PROMPT;
            }
            else {
                reset_on_face_lost = json_is_boolean(reset_on_face_lost_obj) ? json_boolean_value(reset_on_face_lost_obj) : false;
                reset_on_face_lost_type = reset_on_face_lost ? effect_parameter_facetracking_detail::CHOICE_YES : effect_parameter_facetracking_detail::CHOICE_NO;
            }

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

        void set_default(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            obs_data_set_default_bool(settings, get_full_subparam_name_static(full_param_name, std::string(PARAM_STR_RESET_ON_FACE_LOST)).c_str(), default_reset_on_face_lost);
        }

        void render_property_ui(const char *effect_name, obs_properties_t *props) override {
            std::string full_param_name = get_full_param_name(effect_name);
            if (reset_on_face_lost_type == effect_parameter_facetracking_detail::CHOICE_PROMPT) {
                auto *ui_reset_prop = obs_properties_add_list(
                    props, get_full_subparam_name_static(full_param_name, PARAM_STR_RESET_ON_FACE_LOST).c_str(), "On face lost:",
                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_BOOL
                );
                obs_property_list_add_bool(ui_reset_prop, "Keep previous results", false);
                obs_property_list_add_bool(ui_reset_prop, "Reset face tracking", true);
            }
        }

        void apply_visible(const char *effect_name, obs_properties_t *props, const bool visible) override {
            std::string full_param_name = get_full_param_name(effect_name);
            std::string reset_param_name = get_full_subparam_name_static(full_param_name, PARAM_STR_RESET_ON_FACE_LOST);
            auto *reset_prop = obs_properties_get(props, reset_param_name.c_str());
            if (reset_prop != nullptr) {
                obs_property_set_visible(reset_prop, visible);
            }
        }

        void tick(shadertastic_common *s) override {
            face_tracking = s->face_tracking.get();
            cx = obs_source_get_width(s->source);
            cy = obs_source_get_height(s->source);
        }

        void try_gs_set_val() override {
            if (!face_tracking) {
                return;
            }

            std::string face_found = get_full_subparam_name_static(name, PARAM_STR_FACE_FOUND);
            std::string bbox_tl = get_full_subparam_name_static(name, PARAM_STR_BOX_TL);
            std::string bbox_br = get_full_subparam_name_static(name, PARAM_STR_BOX_BR);
            std::string points_tex = get_full_subparam_name_static(name, PARAM_STR_POINTS_TEX);

            if (!face_tracking->facelandmark_results_display_results && reset_on_face_lost) {
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

        void set_data_from_settings(obs_data_t *settings, const char *effect_name) override {
            std::string full_param_name = get_full_param_name(effect_name);
            if (reset_on_face_lost_type == effect_parameter_facetracking_detail::CHOICE_PROMPT) {
                reset_on_face_lost = obs_data_get_bool(settings, get_full_subparam_name_static(full_param_name, PARAM_STR_RESET_ON_FACE_LOST).c_str());
            }
        }
};

#endif // SHADERTASTIC_PARAMETER_FACETRACKING_HPP
