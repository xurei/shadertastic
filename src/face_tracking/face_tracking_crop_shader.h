#include <graphics/graphics.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "../util/tuple.h"

class FaceTrackingCropShader {
private:
    gs_texrender_t *source_texrender;
    gs_texrender_t *crop_texrender;
    gs_stagesurf_t *staging_texture = nullptr;
    gs_effect_t *gs_crop_effect = nullptr;
    gs_eparam_t *gs_crop_param_center = nullptr;
    gs_eparam_t *gs_crop_param_crop_size = nullptr;
    gs_eparam_t *gs_crop_param_rotation = nullptr;
    gs_eparam_t *gs_crop_param_aspect_ratio = nullptr;

public:
    FaceTrackingCropShader();
    ~FaceTrackingCropShader();

    cv::Mat getCroppedImage(obs_source_t *target_source, float2 &roi_center, float2 &roi_size, float rotation);
};
