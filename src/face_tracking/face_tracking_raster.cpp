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

#include "onnxmediapipe/face_landmarks_triangles.h"
#include "src/shadertastic.hpp"

#include "face_tracking_raster.h"
//----------------------------------------------------------------------------------------------------------------------

gs_effect_t *FaceTrackingRaster::raster_effect = nullptr;
bool FaceTrackingRaster::mesh_uv_gpu_ready = false;
std::vector<FaceTrackingRaster::RasterVertex> FaceTrackingRaster::vertices;
gs_vertbuffer_t *FaceTrackingRaster::vertexbuffer = nullptr;
gs_texrender_t *FaceTrackingRaster::texrender = nullptr;
//----------------------------------------------------------------------------------------------------------------------

static inline float edge_function(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

gs_vertbuffer_t *create_uv_vbuffer(uint32_t num_verts) {
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

void fill_vertex_buffer(const std::vector<FaceTrackingRaster::RasterVertex> &vertices, gs_vertbuffer_t *vb) {
    const size_t vertex_count = vertices.size();
    auto *vb_data = gs_vertexbuffer_get_data(vb);

    for (size_t i = 0; i < vertex_count; ++i) {
        const FaceTrackingRaster::RasterVertex& v = vertices[i];

        // POSITION
        vb_data->points[i] = { v.pos.x, v.pos.y, v.pos.z };

        // bary.xy
        struct vec4 *bary_xy = (struct vec4 *) vb_data->tvarray[0].array;
        bary_xy[i] = { v.bary[0], v.bary[1], v.bary[2], v.tri_id };
    }

    gs_vertexbuffer_flush(vb);
}

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

void FaceTrackingRaster::load() {
    FaceTrackingRaster::vertices.reserve(onnxmediapipe::nb_face_triangles * 3);
    FaceTrackingRaster::vertexbuffer = create_uv_vbuffer(onnxmediapipe::nb_face_triangles * 3);
    FaceTrackingRaster::texrender = gs_texrender_create(GS_RGBA32F, GS_Z32F);
    FaceTrackingRaster::mesh_uv_gpu_ready = true;
}

gs_texture_t* FaceTrackingRaster::mesh_uv_gpu(cv::Point3f uvs[], const cv::Vec3i triangles[], int width, int height ) {
    if (!mesh_uv_gpu_ready) {
        load();
    }
    vertices.clear();
    gs_texrender_reset(texrender);

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

        vertices.push_back(RasterVertex {vv0, {0.0f, 0.0f, 1.0f}, tri_norm});
        vertices.push_back(RasterVertex {vv1, {1.0f, 0.0f, 0.0f}, tri_norm});
        vertices.push_back(RasterVertex {vv2, {0.0f, 1.0f, 0.0f}, tri_norm});
    }

    //debug("Z VALS : [ %f, %f ]", minz, maxz);

    obs_enter_graphics();
    {
        // Render target
        if (!texrender) {
            goto end;
        }

        if (!gs_texrender_begin(texrender, width, height)) {
            goto end;
        }

        // Vertex buffer
        fill_vertex_buffer(vertices, vertexbuffer);
        gs_load_vertexbuffer(vertexbuffer);
        gs_load_indexbuffer(nullptr);

        if (raster_effect == nullptr) {
            char *raster_effect_path = obs_module_file("effects/facetracking_raster.hlsl");
            raster_effect = gs_effect_create_from_file(raster_effect_path, nullptr);
            bfree(raster_effect_path);
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
    auto *out = (texrender == nullptr) ? nullptr : gs_texrender_get_texture(texrender);
    obs_leave_graphics();
    return out;
}

cv::Mat FaceTrackingRaster::mesh_uv(cv::Point3f uvs[], const cv::Vec3i triangles[], int width, int height) {
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

void FaceTrackingRaster::unload() {
    if (mesh_uv_gpu_ready) {
        obs_enter_graphics();
        {
            release_resource(gs_effect_destroy, raster_effect);
            release_resource(gs_texrender_destroy, texrender);
            release_resource(gs_vertexbuffer_destroy, vertexbuffer);
        }
        mesh_uv_gpu_ready = false;
    }
}
