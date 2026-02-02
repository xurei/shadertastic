#include <opencv2/imgcodecs.hpp>

#include "texture_util.h"
#include "stb_image_write.h"

void render_texture(gs_texture_t *final_tex, const bool use_copy, const bool compensate_alpha) {
    const uint32_t cx = gs_texture_get_width(final_tex);
    const uint32_t cy = gs_texture_get_height(final_tex);
    gs_blend_state_push();
    //gs_blend_function(GS_BLEND_ONE, is_last_filter ? GS_BLEND_INVSRCALPHA : GS_BLEND_ZERO);
    // gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

    if (use_copy) {
        gs_blend_function_separate(
            GS_BLEND_ONE, GS_BLEND_ZERO,
            GS_BLEND_ONE, GS_BLEND_ZERO
        );
    }
    else {
		gs_blend_function_separate(
			GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA,
			GS_BLEND_ONE, GS_BLEND_INVSRCALPHA
		);
    }

    gs_effect_t *default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    gs_eparam_t *image = gs_effect_get_param_by_name(default_effect, "image");
    //gs_effect_set_float(gs_effect_get_param_by_name(default_effect, "multiplier"), 1.0);

    const bool linear_srgb = gs_get_linear_srgb();
    const bool previous = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(linear_srgb);

//    if (linear_srgb) {
        gs_effect_set_texture_srgb(image, final_tex);
//    }
//    else {
//        gs_effect_set_texture_srgb(image, final_tex);
//        //gs_effect_set_texture(image, final_tex);
//    }

    gs_technique_t *tech = gs_effect_get_technique(default_effect, compensate_alpha ? "DrawAlphaDivide" : "Draw");
    if (gs_technique_begin(tech)) {
        if (gs_technique_begin_pass(tech, 0)) {
            gs_draw_sprite(final_tex, 0, cx, cy);
            gs_technique_end_pass(tech);
        }
        gs_technique_end(tech);
    }

    gs_enable_framebuffer_srgb(previous);
    gs_blend_state_pop();
}

inline int get_opencv_format(const gs_color_format obs_format) {
    switch (obs_format) {
        case GS_UNKNOWN: return -1;
        case GS_A8:
        case GS_R8: return CV_8UC1;
        case GS_RGBA:
        case GS_BGRX:
        case GS_BGRA: return CV_8UC4;
            break;
        case GS_R10G10B10A2: return CV_32SC4;
        case GS_RGBA16: return CV_16UC4;
        case GS_R16: return CV_16UC1;
        case GS_RGBA16F: return CV_16FC4;
        case GS_RGBA32F: return CV_32FC4;
        case GS_RG16F: return CV_16FC2;
        case GS_RG32F: return CV_32FC2;
        case GS_R16F: return CV_16FC1;
        case GS_R32F: return CV_32FC1;
        default: return -1;
    }
}

#ifdef DEV_MODE
cv::Mat extractImage(gs_texture_t *tex) {
    obs_enter_graphics();
    const uint32_t cx = gs_texture_get_width(tex);
    const uint32_t cy = gs_texture_get_height(tex);
    auto color_format = gs_texture_get_color_format(tex);
    auto staging_texture = gs_stagesurface_create(cx, cy, color_format);
    gs_stage_texture(staging_texture, tex);

    uint8_t *gpu_data;
    uint32_t linesize;
    cv::Mat imageRGBA;

    if (gs_stagesurface_map(staging_texture, &gpu_data, &linesize)) {
        // Convert RGBA to BGR
        //debug_trace("  1 %lu", get_time_us()-tic);
        imageRGBA = cv::Mat(cy, cx, get_opencv_format(color_format));
        memcpy(imageRGBA.data, gpu_data, linesize*cy);
        //debug_trace("  2 %lu", get_time_us()-tic);

        gs_stagesurface_unmap(staging_texture);
        //debug_trace("  3 %lu", get_time_us()-tic);
    }
    else {
        //debug("cpt2");
        const cv::Mat failed(0, 0, CV_8UC1);
        gs_stagesurface_destroy(staging_texture);
        obs_leave_graphics();
        return failed;
    }

    gs_stagesurface_destroy(staging_texture);
    obs_leave_graphics();

    return imageRGBA;
}

bool saveMat(
    cv::Mat& src,
    std::string path
) {
        if (src.empty()) {
        return false;
    }

    if (src.channels() != 4) {
        return false;
    }

    cv::Mat tmp_float;

    switch (src.depth()) {
        case CV_8U: {
            src.convertTo(tmp_float, CV_32FC4, 1.0 / 255.0);
            break;
        }
        case CV_16U: {
            src.convertTo(tmp_float, CV_32FC4, 1.0 / 65535.0);
            break;
        }
        case CV_16F: {
            src.convertTo(tmp_float, CV_32FC4);
            break;
        }
        case CV_32F: {
            tmp_float = src;
            break;
        }
        case CV_64F: {
            src.convertTo(tmp_float, CV_32FC4);
            break;
        }
        default: {
            return false;
        }
    }

    cv::Mat clamped;
    cv::min(tmp_float, 1.0, clamped);
    cv::max(clamped, 0.0, clamped);

    cv::Mat rgba_8u;
    clamped.convertTo(rgba_8u, CV_8UC4, 255.0);

    //cv::cvtColor(rgba_8u, rgba_8u, cv::COLOR_BGRA2RGBA);

    if (!rgba_8u.isContinuous()) {
        rgba_8u = rgba_8u.clone();
    }

    int stride = rgba_8u.cols * 4;

    int result = stbi_write_png(
        path.c_str(),
        rgba_8u.cols,
        rgba_8u.rows,
        4,
        rgba_8u.data,
        stride
    );

    return result != 0;
}
#endif // DEV_MODE
