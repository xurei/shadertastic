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

#define FACEDETECTION_WIDTH 1280
#define FACEDETECTION_HEIGHT 720

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

static void face_tracking_update(face_tracking_state *s);

void face_tracking_create(face_tracking_state *s);

void face_tracking_tick(face_tracking_state *s, obs_source_t *target_source);

void face_tracking_render(face_tracking_state *s, effect_shader *main_shader);

void face_tracking_destroy(face_tracking_state *s);

#endif /* SHADERTASTIC_FACE_TRACKING_H */
