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

#ifndef SHADERTASTIC_FACE_TRACKING_RASTER_H
#define SHADERTASTIC_FACE_TRACKING_RASTER_H

#include <obs-module.h>
#include <opencv2/core.hpp>

class FaceTrackingRaster {
    public:
        /**
         * Prepare the static resources required to preraster
         */
        static void load();

        [[maybe_unused]] static gs_texture_t* mesh_uv_gpu(cv::Point3f uvs[], const cv::Vec3i triangles[], int width, int height);

        [[maybe_unused]] static cv::Mat mesh_uv(cv::Point3f uvs[], const cv::Vec3i triangles[], int width, int height);

        static void unload();

        struct RasterVertex {
            vec3 pos;       // NDC
            float bary[3];  // barycentrics
            float tri_id;   // normalized triangle id
        };

    private:
        static bool mesh_uv_gpu_ready;
        static gs_effect_t *raster_effect;
        static gs_vertbuffer_t *vertexbuffer;
        static gs_texrender_t *texrender;
        static std::vector<RasterVertex> vertices;
};

#endif //SHADERTASTIC_FACE_TRACKING_RASTER_H
