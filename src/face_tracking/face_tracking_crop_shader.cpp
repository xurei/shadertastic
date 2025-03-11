#include <obs-module.h>
#include "face_tracking_crop_shader.h"
#include "../util/rgba_to_rgb.h"
#include "../logging_functions.hpp"
//----------------------------------------------------------------------------------------------------------------------

static inline gs_texture_t * prepare_source_texture(gs_texrender_t *source_texrender, obs_source_t *target, uint32_t cx, uint32_t cy, enum gs_color_space space) {
    gs_texrender_reset(source_texrender);
    if (gs_texrender_begin_with_color_space(source_texrender, cx, cy, space)) {
		gs_blend_state_push();
		gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		vec4 clear_color{};

		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, (float)cx, 0.0f, (float)cy, -100.0f, 100.0f);

        obs_source_video_render(target);

		gs_blend_state_pop();

		gs_texrender_end(source_texrender);

        gs_texture_t *source_tex = gs_texrender_get_texture(source_texrender);
        return source_tex;
	}
    return nullptr;
}

static inline void render_filter_texture(gs_texture_t *source_tex, gs_effect_t *effect, uint32_t width, uint32_t height) {
	gs_technique_t *tech = gs_effect_get_technique(effect, "DrawLinear");
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	size_t passes, i;

	const bool linear_srgb = true;

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(linear_srgb);


//	if (linear_srgb) {
        gs_effect_set_texture_srgb(image, source_tex);
//    }
//	else {
//        gs_effect_set_texture(image, tex);
//    }

	passes = gs_technique_begin(tech);
	for (i = 0; i < passes; i++) {
		gs_technique_begin_pass(tech, i);
		gs_draw_sprite(source_tex, 0, width, height);
		gs_technique_end_pass(tech);
	}
	gs_technique_end(tech);

	gs_enable_framebuffer_srgb(previous);

    gs_set_linear_srgb(previous);
}
//----------------------------------------------------------------------------------------------------------------------

FaceTrackingCropShader::FaceTrackingCropShader() {
    obs_enter_graphics();
    {
        this->source_texrender = gs_texrender_create(GS_RGBA32F, GS_ZS_NONE);
        this->crop_texrender = gs_texrender_create(GS_RGBA32F, GS_ZS_NONE);

        // Crop effect for the mesh model
        char *crop192_path = obs_module_file("effects/crop192.hlsl");
        this->gs_crop_effect = gs_effect_create_from_file(crop192_path, nullptr);
        bfree(crop192_path);

        //gs_crop_param_top_left
        this->gs_crop_param_center = gs_effect_get_param_by_name(this->gs_crop_effect, "center");
        this->gs_crop_param_crop_size = gs_effect_get_param_by_name(this->gs_crop_effect, "crop_size");
        this->gs_crop_param_rotation = gs_effect_get_param_by_name(this->gs_crop_effect, "rotation");
        this->gs_crop_param_aspect_ratio = gs_effect_get_param_by_name(this->gs_crop_effect, "aspect_ratio");
    }
    obs_leave_graphics();
}

FaceTrackingCropShader::~FaceTrackingCropShader() {
    obs_enter_graphics();
    {
        if (this->staging_texture) {
            gs_stagesurface_destroy(this->staging_texture);
            this->staging_texture = nullptr;
        }
        if (this->gs_crop_effect) {
            gs_effect_destroy(this->gs_crop_effect);
            this->gs_crop_effect = nullptr;
        }
        if (this->crop_texrender) {
            gs_texrender_destroy(this->crop_texrender);
            this->crop_texrender = nullptr;
        }
        if (this->source_texrender) {
            gs_texrender_destroy(this->source_texrender);
            this->source_texrender = nullptr;
        }
    }
    obs_leave_graphics();
}

cv::Mat FaceTrackingCropShader::getCroppedImage(obs_source_t *target_source, float2 &roi_center, float2 &roi_size, float rotation) {
    const enum gs_color_space preferred_spaces[] = {
        GS_CS_SRGB,
        GS_CS_SRGB_16F,
        GS_CS_709_EXTENDED,
    };
    const enum gs_color_space source_space = obs_source_get_color_space(target_source, OBS_COUNTOF(preferred_spaces), preferred_spaces);

    uint32_t cx = obs_source_get_width(target_source);
    uint32_t cy = obs_source_get_height(target_source);
    float aspect_ratio = static_cast<float>(cx) / static_cast<float>(cy);

    gs_texture_t *source_tex = prepare_source_texture(this->source_texrender, target_source, cx, cy, source_space);

    if (source_tex == nullptr) {
        cv::Mat failed(0, 0, CV_32FC4);
        return failed;
    }

    if (!this->staging_texture || gs_stagesurface_get_width(this->staging_texture) != 192 || gs_stagesurface_get_height(this->staging_texture) != 192) {
        if (this->staging_texture) {
            gs_stagesurface_destroy(this->staging_texture);
        }
        this->staging_texture = gs_stagesurface_create(192, 192, GS_RGBA32F);
    }

    gs_texrender_reset(this->crop_texrender);
    if (gs_texrender_begin_with_color_space(this->crop_texrender, 192, 192, source_space)) {
        gs_blend_state_push();
        gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

        vec2 center{
            .x=roi_center.x,
            .y=roi_center.y,
        };
        vec2 crop_size{
            .x=roi_size.x,
            .y=roi_size.y,
        };
        gs_effect_set_vec2(this->gs_crop_param_center, &center);
        gs_effect_set_vec2(this->gs_crop_param_crop_size, &crop_size);
        gs_effect_set_float(this->gs_crop_param_rotation, rotation);
        gs_effect_set_float(this->gs_crop_param_aspect_ratio, aspect_ratio);

        gs_blend_state_push();
        gs_blend_function_separate(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

        struct vec4 clear_color{0, 0, 0, 0};
        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
        gs_ortho(0.0f, 192.0f, 0.0f, 192.0f, -100.0f, 100.0f);

        render_filter_texture(source_tex, this->gs_crop_effect, 192, 192);

        gs_blend_state_pop();
        gs_texrender_end(this->crop_texrender);
        gs_texture_t *tex = gs_texrender_get_texture(this->crop_texrender);

        gs_stage_texture(this->staging_texture, tex);

        uint8_t *data;
        uint32_t linesize;
        if (gs_stagesurface_map(this->staging_texture, &data, &linesize)) {
            // Convert RGBA to BGR
            cv::Mat imageRGBA(192, 192, CV_32FC4, data);
            cv::Mat imageBGR = rgbaToBgrFloat(imageRGBA);

            gs_stagesurface_unmap(this->staging_texture);

            return imageBGR;
        }
        else {
            debug("cpt2");
            cv::Mat failed(0, 0, CV_32FC4);
            return failed;
        }
    }
    cv::Mat failed(0, 0, CV_32FC4);
    return failed;
}
