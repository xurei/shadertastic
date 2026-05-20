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

#include <graphics/vec2.h>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>
#include <thread>
#include <obs-module.h>
#include <graphics/graphics.h>
#include <util/bmem.h>
#include "onnxmediapipe/face_landmarks_triangles.h"
#include "onnxmediapipe/models_provider.h"
#include "face_tracking.h"
#include "../logging_functions.hpp"
#include "../settings.h"
#include "../util/time_util.hpp"
#include "src/util/texture_util.h"
#include "src/shadertastic.hpp"
#include "face_tracking_points.h"

// Globals
static const cv::Mat failed(0, 0, CV_8UC1);
//----------------------------------------------------------------------------------------------------------------------

static inline float edge_function(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

struct RasterVertex {
    vec3 pos;       // NDC
    float bary[3];  // barycentrics
    float tri_id;   // normalized triangle id
};

gs_vertbuffer_t *create_uv_vbuffer(uint32_t num_verts)
{
    struct gs_vb_data *vrect = nullptr;
    vrect = gs_vbdata_create();
    vrect->num = num_verts;
    vrect->points = (struct vec3 *)bzalloc(sizeof(struct vec3) * num_verts);
    vrect->num_tex = 1;
    vrect->tvarray = (struct gs_tvertarray *)bzalloc(sizeof(struct gs_tvertarray) * 2);
    vrect->tvarray[0].width = 4;
    vrect->tvarray[0].array = bzalloc(sizeof(struct vec4) * num_verts);

    gs_vertbuffer_t *out;
    obs_enter_graphics();
    {
        out = gs_vertexbuffer_create(vrect, GS_DYNAMIC);
    }
    obs_leave_graphics();

	return out;
}

void fill_vertex_buffer(const std::vector<RasterVertex> &vertices, gs_vertbuffer_t *vb)
{
    const size_t vertex_count = vertices.size();
    auto *vb_data = gs_vertexbuffer_get_data(vb);

    for (size_t i = 0; i < vertex_count; ++i) {
        const RasterVertex& v = vertices[i];

        // POSITION
        vb_data->points[i] = { v.pos.x, v.pos.y, v.pos.z };

        // bary.xy
        struct vec4 *bary_xy = (struct vec4 *) vb_data->tvarray[0].array;
        bary_xy[i] = { v.bary[0], v.bary[1], v.bary[2], v.tri_id };
    }

    gs_vertexbuffer_flush(vb);
}

gs_effect_t *raster_effect = nullptr;
bool face_tracking_raster_mesh_uv_gpu_ready = false;
std::vector<RasterVertex> face_tracking_raster_vertices;
gs_vertbuffer_t *face_tracking_raster_vertexbuffer = nullptr;
gs_texrender_t *face_tracking_raster_texrender = nullptr;

inline vec3 face_tracking_to_ndc(const cv::Point3f& v) {
    float x = v.x * 2.0f - 1.0f;
    float y = shadertastic_is_direct3d() ? (1.0f - v.y * 2.0f) : (v.y * 2.0f - 1.0f);
    float z = 0.5f + v.z * 0.5f;
    vec3 out = {
        .x = x,
        .y = y,
        .z = z
    };
    return out;
};

gs_texrender_t* face_tracking_raster_mesh_uv_gpu(
    cv::Point3f uvs[],
    const cv::Vec3i triangles[],
    int width,
    int height
) {
    if (!face_tracking_raster_mesh_uv_gpu_ready) {
        face_tracking_raster_vertices.reserve(onnxmediapipe::nb_face_triangles * 3);
        face_tracking_raster_vertexbuffer = create_uv_vbuffer(onnxmediapipe::nb_face_triangles * 3);
        face_tracking_raster_texrender = gs_texrender_create(GS_RGBA32F, GS_Z32F);
        face_tracking_raster_mesh_uv_gpu_ready = true;
    }
    face_tracking_raster_vertices.clear();
    gs_texrender_reset(face_tracking_raster_texrender);

    face_tracking_raster_vertices.clear();
    //float minz = 1000.0f, maxz = -1000.0f;
    for (int tri_id = 0; tri_id < onnxmediapipe::nb_face_triangles; ++tri_id) {
        const cv::Vec3i& tri = triangles[tri_id];

        const cv::Point3f& v0 = uvs[tri[0]];
        const cv::Point3f& v1 = uvs[tri[1]];
        const cv::Point3f& v2 = uvs[tri[2]];

        auto vv0 = face_tracking_to_ndc(v0);
        auto vv1 = face_tracking_to_ndc(v1);
        auto vv2 = face_tracking_to_ndc(v2);

        //minz = std::min(minz, vv0.z);
        //minz = std::min(minz, vv1.z);
        //minz = std::min(minz, vv2.z);
        //maxz = std::max(maxz, vv0.z);
        //maxz = std::max(maxz, vv1.z);
        //maxz = std::max(maxz, vv2.z);

        float tri_norm = ((float)tri_id + 0.5f) / (float)onnxmediapipe::nb_face_triangles;

        face_tracking_raster_vertices.push_back(RasterVertex {vv0, {0.0f,0.0f,1.0f}, tri_norm});
        face_tracking_raster_vertices.push_back(RasterVertex {vv1, {1.0f,0.0f,0.0f}, tri_norm});
        face_tracking_raster_vertices.push_back(RasterVertex {vv2, {0.0f,1.0f,0.0f}, tri_norm});
    }

    //debug("Z VALS : [ %f, %f ]", minz, maxz);

    obs_enter_graphics();
    {
        // Render target
        auto *texrender = face_tracking_raster_texrender;
        if (!texrender) {
            goto end;
        }

        if (!gs_texrender_begin(texrender, width, height)) {
            goto end;
        }

        // Vertex buffer
        fill_vertex_buffer(face_tracking_raster_vertices, face_tracking_raster_vertexbuffer);
        gs_load_vertexbuffer(face_tracking_raster_vertexbuffer);
        gs_load_indexbuffer(nullptr);

        if (raster_effect == nullptr) {
            char *raster_effect_path = obs_module_file("effects/facetracking_raster.hlsl");
            raster_effect = gs_effect_create_from_file(raster_effect_path, nullptr);
        }

        gs_technique_t *tech = gs_effect_get_technique(raster_effect, "Draw");

        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);

        gs_enable_depth_test(true);
        vec4 clear_color = {0, 0, 0, 0};
        gs_clear(GS_CLEAR_COLOR | GS_CLEAR_DEPTH, &clear_color, 1.0f, 0);
        gs_ortho(0.0f, (float) width, 0.0f, (float) height, -100.0f, 100.0f); // This line took me A WHOLE WEEK to figure out
        //gs_depth_function(GS_GREATER);
        gs_depth_function(GS_LESS);

        auto prev_cull = gs_get_cull_mode();
        gs_set_cull_mode(GS_BACK);
        gs_draw(GS_TRIS, 0, onnxmediapipe::nb_face_triangles * 3);

        gs_technique_end_pass(tech);
        gs_technique_end(tech);

        gs_texrender_end(texrender);
        gs_set_cull_mode(prev_cull);
        gs_enable_depth_test(false);
    }

    end:
    obs_leave_graphics();
    return face_tracking_raster_texrender;
}

cv::Mat face_tracking_raster_mesh_uv(
    cv::Point3f uvs[],
    const cv::Vec3i triangles[],
    int width,
    int height
) {
    cv::Mat out(height, width, CV_32FC4, cv::Scalar_<float>(0.0f, 0.0f, 0.0f, 0.0f));
    cv::Mat zbuffer(height, width, CV_32FC1, cv::Scalar_<float>(-std::numeric_limits<float>::infinity()));

    for (int tri_id = 0; tri_id < onnxmediapipe::nb_face_triangles; ++tri_id) {
        const cv::Vec3i& tri = triangles[tri_id];
        const cv::Point3f& v0 = uvs[tri[0]];
        const cv::Point3f& v1 = uvs[tri[1]];
        const cv::Point3f& v2 = uvs[tri[2]];

        cv::Point2f p0(v0.x * (float)width,  v0.y * (float)height);
        cv::Point2f p1(v1.x * (float)width,  v1.y * (float)height);
        cv::Point2f p2(v2.x * (float)width,  v2.y * (float)height);

        int min_x = std::max(0,          (int)std::floor(std::min({p0.x, p1.x, p2.x})));
        int max_x = std::min(width - 1,  (int)std::ceil (std::max({p0.x, p1.x, p2.x})));
        int min_y = std::max(0,          (int)std::floor(std::min({p0.y, p1.y, p2.y})));
        int max_y = std::min(height - 1, (int)std::ceil (std::max({p0.y, p1.y, p2.y})));

        float area = edge_function(p0, p1, p2);
        if (std::abs(area) < 1e-6f) {
            continue;
        }

        float inv_area = 1.0f / area;

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {

                cv::Point2f p((float)x + 0.5f, (float)y + 0.5f);

                float w0 = edge_function(p1, p2, p);
                float w1 = edge_function(p2, p0, p);
                float w2 = edge_function(p0, p1, p);

                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    // barycentriques normalisées
                    float b0 = w0 * inv_area;
                    float b1 = w1 * inv_area;
                    float b2 = w2 * inv_area;

                    // interpolation Z (uvs.z)
                    float z =
                        b0 * v0.z +
                        b1 * v1.z +
                        b2 * v2.z;

                    float& current_z = zbuffer.at<float>(y, x);

                    if (z > current_z) {
                        current_z = z;

                        float tri_norm = ((float)tri_id+0.5f) / (float)(onnxmediapipe::nb_face_triangles);

                        // On stocke 2 bary (la 3e = 1 - u - v)
                        out.at<cv::Vec4f>(y, x) = cv::Vec4f(
                            tri_norm,
                            b1,
                            b2,
                            1.0f
                        );
                    }
                }
            }
        }
    }

    return out;
}
//----------------------------------------------------------------------------------------------------------------------

void face_tracking_copy_points(onnxmediapipe::FaceLandmarksResults *facelandmark_results, float *points) {
    for (size_t i=0; i < refined_landmarks_num_points; ++i) {
        points[i*4+0] = facelandmark_results->refined_landmarks[i].x;
        points[i*4+1] = facelandmark_results->refined_landmarks[i].y;
        points[i*4+2] = facelandmark_results->refined_landmarks[i].z;
        points[i*4+3] = 1.0;
    }
}
//----------------------------------------------------------------------------------------------------------------------

face_tracking_bounding_box face_tracking_get_bounding_box(onnxmediapipe::FaceLandmarksResults *facelandmark_results, const unsigned short int *indices, int nb_indices) {
    cv::Point2f minPoint(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    cv::Point2f maxPoint(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

    for (int i=0; i<nb_indices; ++i) {
        const size_t landmark_i = indices[i];
        if (landmark_i < refined_landmarks_num_points) {
            auto &k = facelandmark_results->refined_landmarks[landmark_i];
            minPoint.x = std::min(minPoint.x, k.x);
            minPoint.y = std::min(minPoint.y, k.y);

            maxPoint.x = std::max(maxPoint.x, k.x);
            maxPoint.y = std::max(maxPoint.y, k.y);
        }
    }

    face_tracking_bounding_box out{
        minPoint.x, minPoint.y,
        maxPoint.x, maxPoint.y
    };
    return out;
}
//----------------------------------------------------------------------------------------------------------------------

void face_tracking_create(std::unique_ptr<face_tracking_state> &s) {
    debug('face_tracking_create');
    s.reset(new face_tracking_state);

    for (size_t i = 0; i < refined_landmarks_num_points * 3; ++i) {
        s->filters[i].setFrequency((float)std::max(1.0, obs_get_active_fps()));
        s->filters[i].setMinCutoff(10.0f);
        s->filters[i].setBeta(0.007f);
        s->filters[i].setDerivateCutoff(10.0f);
    }

    onnxmediapipe::ModelsProvider::initialize();

    s->crop_shader = std::make_unique<FaceTrackingCropShader>();

    obs_enter_graphics();
    {
        s->facedetection_texrender = gs_texrender_create(GS_RGBA32F, GS_ZS_NONE);

        if (!s->fd_points_texture) {
            s->fd_points_texture = gs_texture_create(refined_landmarks_num_points, 4, GS_RGBA32F, 1, nullptr, GS_DYNAMIC);
        }

        float *texpoints;
        uint32_t linesize2 = 0;
        gs_texture_map(s->fd_points_texture, (uint8_t **)(&texpoints), &linesize2);

        {
            // Fill in the 2nd and 3rd rows of the texture with the triangle map
            int start_idx = 1 * refined_landmarks_num_points * 4;
            for (int tri_id = 0; tri_id < onnxmediapipe::nb_face_triangles; ++tri_id) {
                auto tri = onnxmediapipe::face_triangles[tri_id];
                float tri_f[4] = {
                    ((float)tri[0] + 0.5f) / 478.0f,
                    ((float)tri[1] + 0.5f) / 478.0f,
                    ((float)tri[2] + 0.5f) / 478.0f,
                    1.0f
                };
                memcpy(&texpoints[start_idx + tri_id*4], tri_f, 4 * sizeof(float));
            }
            /*memcpy(&texpoints[k], facetracking_points_data, refined_landmarks_num_points * 4 * sizeof(float));
            k = 3 * refined_landmarks_num_points * 4;
            memcpy(&texpoints[k], facetracking_points_data, refined_landmarks_num_points * 4 * sizeof(float));*/
        }
        {
            // Fill in the last row of the texture with the point from the original model
            int k = 3 * refined_landmarks_num_points * 4;
            memcpy(&texpoints[k], facetracking_points_data, refined_landmarks_num_points * 4 * sizeof(float));
        }
        gs_texture_unmap(s->fd_points_texture);
    }
    obs_leave_graphics();
    //debug("STAGING TEXTURE = %p", s->staging_texture);

    /** Configure networks **/
    try {
        s->facemesh = std::make_shared <onnxmediapipe::FaceMesh>();
    }
    catch (const std::exception& error) {
        blog(LOG_INFO, "in detection inference creation, exception: %s", error.what());
    }
}
//----------------------------------------------------------------------------------------------------------------------

uint2 scaledown_aspectratio(uint32_t cx, uint32_t cy, uint32_t max_size) {
    if (cx == 0 && cy == 0) {
        return uint2 {
            .x = 192,
            .y = 192,
        };
    }
    if (cx > cy) {
        return uint2 {
            .x = max_size,
            .y = (cy * max_size) / cx,
        };
    }
    else {
        return uint2 {
            .x = (cx * max_size) / cy,
            .y = max_size,
        };
    }
}

void face_tracking_tick(face_tracking_state *s, gs_texture_t *source_tex, const float deltatime) {
    #ifdef DEV_MODE
    unsigned long tic = get_time_us();
    #endif
    auto facemesh = s->facemesh;
    if (!facemesh) {
        return;
    }

    size_t results_index = 0;

    bool face_found = true;

    // Scaling down cx and cy to make them fit in 192x192
    const uint32_t cx = gs_texture_get_width(source_tex);
    const uint32_t cy = gs_texture_get_height(source_tex);
    uint2 texrender_size_for_detection = scaledown_aspectratio(cx, cy, 192);

    bool prev_facelandmark_results_display_results = s->facelandmark_results_display_results;

    debug_trace("A %lu", get_time_us()-tic);

    if (facemesh->IsFaceDetectionNeeded()) {
        cv::Mat imageBGR = face_tracking_get_image_for_detection(s, source_tex, texrender_size_for_detection);

        if (imageBGR.empty()) {
            log_error("Something went wrong with the extraction of the source texture");
            face_found = false;
        }
        else {
            face_found = facemesh->RunFaceDetection(imageBGR);
        }
        return;
    }
    debug_trace("B %lu", get_time_us()-tic);
    s->facelandmark_results_display_results = face_found;

    if (face_found) {
        float2 roi_size = facemesh->getROISize();
        float2 roi_center = facemesh->getROICenter();
        float rotation = facemesh->getROIRotation();

        debug_trace("C %lu", get_time_us()-tic);

        // Scaling down cx and cy to make them fit in 192x192
        cv::Mat imageBGR = face_tracking_get_image_for_mesh(s, source_tex, roi_center, roi_size, rotation);
        debug_trace("D %lu", get_time_us()-tic);

        if (imageBGR.empty()) {
            log_error("Something went wrong with the extraction of the face to mesh");
            s->facelandmark_results_display_results = false;
        }
        else {
            s->facelandmark_results_display_results = facemesh->Run(imageBGR, (int)cx, (int)cy, s->facelandmark_results[results_index]);
        }
        debug_trace("E %lu", get_time_us()-tic);
    }

    if (!s->facelandmark_results_display_results && prev_facelandmark_results_display_results) {
        debug("lost track !");
    }

    if (!s->facelandmark_results_display_results) {
        /* nothing to do */
    }
    else {
        debug_trace("F %lu", get_time_us()-tic);
        if (shadertastic_settings().one_euro_enabled) {
            for (size_t i = 0; i < refined_landmarks_num_points; ++i) {
                s->filters[i * 3 + 0].setMinCutoff(std::max(0.00001f, shadertastic_settings().one_euro_min_cutoff));
                s->filters[i * 3 + 0].setBeta(std::max(0.0001f, shadertastic_settings().one_euro_beta));
                s->filters[i * 3 + 0].setDerivateCutoff(std::max(0.01f, shadertastic_settings().one_euro_deriv_cutoff));
                s->filters[i * 3 + 1].setMinCutoff(std::max(0.00001f, shadertastic_settings().one_euro_min_cutoff));
                s->filters[i * 3 + 1].setBeta(std::max(0.0001f, shadertastic_settings().one_euro_beta));
                s->filters[i * 3 + 1].setDerivateCutoff(std::max(0.01f, shadertastic_settings().one_euro_deriv_cutoff));
                s->filters[i * 3 + 2].setMinCutoff(std::max(0.00001f, shadertastic_settings().one_euro_min_cutoff));
                s->filters[i * 3 + 2].setBeta(std::max(0.0001f, shadertastic_settings().one_euro_beta));
                s->filters[i * 3 + 2].setDerivateCutoff(std::max(0.01f, shadertastic_settings().one_euro_deriv_cutoff));
                s->average_results.refined_landmarks[i].x = s->filters[i * 3 + 0].filter(s->facelandmark_results[results_index].refined_landmarks[i].x, deltatime);
                s->average_results.refined_landmarks[i].y = s->filters[i * 3 + 1].filter(s->facelandmark_results[results_index].refined_landmarks[i].y, deltatime);
                s->average_results.refined_landmarks[i].z = s->filters[i * 3 + 2].filter(s->facelandmark_results[results_index].refined_landmarks[i].z, deltatime);
            }
        }
        else {
            for (size_t i = 0; i < facial_surface_num_points; ++i) {
                s->average_results.facial_surface[i].x = 0.0;
                s->average_results.facial_surface[i].y = 0.0;
                s->average_results.facial_surface[i].z = 0.0;
                size_t count = 0;
                for (size_t j = 0; j < FACEDETECTION_NB_ITERATIONS; ++j) {
                    s->average_results.facial_surface[i] += s->facelandmark_results[j].facial_surface[i];
                    ++count;
                }
                s->average_results.facial_surface[i] /= (float) count;
            }
            for (size_t i = 0; i < refined_landmarks_num_points; ++i) {
                s->average_results.refined_landmarks[i].x = 0.0;
                s->average_results.refined_landmarks[i].y = 0.0;
                s->average_results.refined_landmarks[i].z = 0.0;
                size_t count = 0;
                for (size_t j = 0; j < FACEDETECTION_NB_ITERATIONS; ++j) {
                    s->average_results.refined_landmarks[i] += s->facelandmark_results[j].refined_landmarks[i];
                    ++count;
                }
                s->average_results.refined_landmarks[i] /= (float) count;
            }
        }
        debug_trace("G %lu", get_time_us()-tic);

        float points[refined_landmarks_num_points * 4];
        // TODO we could simplify this by using Point4f directly, but it takes ~15µs to copy the points, probably worthless
        face_tracking_copy_points(&s->average_results, points);
        debug_trace("G1 %lu", get_time_us()-tic);

        // CPU Preraster (kept for debugging and comparing with GPU raster)
        //cv::Mat preraster = face_tracking_raster_mesh_uv(
        //    s->average_results.refined_landmarks,
        //    onnxmediapipe::face_triangles,
        //    (int)cx,
        //    (int)cy
        //);

        //{
        //    auto matA = extractImage(gs_texrender_get_texture(raster_texrender));
        //    saveMat(matA, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_a.png");
        //    saveMat(preraster, "/home/olivier/obs-plugins/obs-shadertastic/plugin/lab/debug_images/tex_a_comp.png");
        //    debug("ok");
        //}

        float *texpoints;
        uint32_t linesize2 = 0;
        debug_trace("G2 %lu", get_time_us()-tic);
        obs_enter_graphics();
        {
            gs_texture_map(s->fd_points_texture, (uint8_t **) (&texpoints), &linesize2);
            memcpy(texpoints, points, refined_landmarks_num_points * 4 * sizeof(float));
            gs_texture_unmap(s->fd_points_texture);
        }
        obs_leave_graphics();
        debug_trace("H %lu", get_time_us()-tic);

        //debug("Tick done");
    }
}
//----------------------------------------------------------------------------------------------------------------------

cv::Mat face_tracking_get_image_for_detection(face_tracking_state *s, gs_texture_t *source_tex, const uint2 &texrender_size) {
    const gs_color_space source_space = GS_CS_SRGB;
    const bool previous = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(true);

    if (!s->staging_texture_detection || gs_stagesurface_get_width(s->staging_texture_detection) != texrender_size.x || gs_stagesurface_get_height(s->staging_texture_detection) != texrender_size.y) {
        if (s->staging_texture_detection) {
            gs_stagesurface_destroy(s->staging_texture_detection);
        }
        s->staging_texture_detection = gs_stagesurface_create(texrender_size.x, texrender_size.y, GS_RGBA32F);
    }

    gs_texrender_reset(s->facedetection_texrender);
    if (gs_texrender_begin_with_color_space(s->facedetection_texrender, texrender_size.x, texrender_size.y, source_space)) {
        const uint32_t cx = gs_texture_get_width(source_tex);
        const uint32_t cy = gs_texture_get_height(source_tex);
        gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);
        render_texture(source_tex, true, false);

        gs_texrender_end(s->facedetection_texrender);

        gs_texture_t *tex = gs_texrender_get_texture(s->facedetection_texrender);

        gs_stage_texture(s->staging_texture_detection, tex);

        //debug("%p ----> %p", s->staging_texture_detection, tex);

        uint8_t *data;
        uint32_t linesize;

        if (gs_stagesurface_map(s->staging_texture_detection, &data, &linesize)) {
            // Convert to BGR
            cv::Mat imageRGBA((int)texrender_size.y, (int)texrender_size.x, CV_32FC4, data);
            cv::Mat imageBGR = rgbaToBgrFloat(imageRGBA);

            // Adjuting gamma
            cv::pow(imageBGR, 1.0 / 2.2, imageBGR);

            gs_stagesurface_unmap(s->staging_texture_detection);
            gs_enable_framebuffer_srgb(previous);

            return imageBGR;
        }
        else {
            debug("cpt2");
            gs_enable_framebuffer_srgb(previous);
            return failed;
        }
    }
    else {
        debug("cpt");
        gs_enable_framebuffer_srgb(previous);
        return failed;
    }
}
//----------------------------------------------------------------------------------------------------------------------

cv::Mat face_tracking_get_image_for_mesh(face_tracking_state *s, gs_texture_t *source_tex, float2 &roi_center, float2 &roi_size, float rotation) {
    return s->crop_shader->getCroppedImage(source_tex, roi_center, roi_size, rotation);
}
//----------------------------------------------------------------------------------------------------------------------

void face_tracking_destroy(std::unique_ptr<face_tracking_state> &s) {
    if (s != nullptr) {
        obs_enter_graphics();
        {
            release_resource(gs_texrender_destroy, s->facedetection_texrender);
            release_resource(gs_texture_destroy, s->fd_points_texture);
            release_resource(gs_stagesurface_destroy, s->staging_texture_detection);
            release_resource(gs_texrender_destroy, face_tracking_raster_texrender);
            release_resource(gs_vertexbuffer_destroy, face_tracking_raster_vertexbuffer);
            release_resource(gs_effect_destroy, raster_effect);
        }
        obs_leave_graphics();
        s->crop_shader.reset(); // NOLINT(bugprone-unused-return-value)
        s->facemesh.reset();
        s.reset(); // NOLINT(bugprone-unused-return-value)
    }
}
//----------------------------------------------------------------------------------------------------------------------
