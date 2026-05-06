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

#ifndef SHADERTASTIC_FACE_TRACKING_H
#define SHADERTASTIC_FACE_TRACKING_H

#include <obs-module.h>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <thread>
#include "face_tracking_state.h"
#include "onnxmediapipe/common.h"
#include "onnxmediapipe/landmark_refinement_indices.h"
#include "../shader/shader.h"
#include "../util/rgba_to_rgb.h"
#include "../util/tuple.h"

struct face_tracking_bounding_box {
    union {
        vec2 point1;
        struct {
            float x1;
            float y1;
        };
    };
    union {
        vec2 point2;
        struct {
            float x2;
            float y2;
        };
    };
};
//----------------------------------------------------------------------------------------------------------------------

void face_tracking_copy_points(onnxmediapipe::FaceLandmarksResults *facelandmark_results, float *points);

face_tracking_bounding_box face_tracking_get_bounding_box(onnxmediapipe::FaceLandmarksResults *facelandmark_results, const unsigned short int *indices, int nb_indices);

void face_tracking_create(std::unique_ptr<face_tracking_state> &s);

void face_tracking_tick(face_tracking_state *s, gs_texture_t *source_tex, float deltatime);

cv::Mat face_tracking_get_image_for_detection(face_tracking_state *s, gs_texture_t *source_tex, const uint2 &texrender_size);

cv::Mat face_tracking_get_image_for_mesh(face_tracking_state *s, gs_texture_t *source_tex, float2 &roi_topleft, float2 &roi_size, float rotation);

void face_tracking_destroy(std::unique_ptr<face_tracking_state> &s);

gs_texrender_t* face_tracking_raster_mesh_uv_gpu(
    cv::Point3f uvs[],
    const cv::Vec3i triangles[],
    int width,
    int height
);

#endif // SHADERTASTIC_FACE_TRACKING_H
