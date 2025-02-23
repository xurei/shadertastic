uniform texture2d fd_points_tex;
uniform bool fd_face_found;
uniform float zoom_level;
uniform bool show_ref_points;
uniform bool preserve_aspect_ratio;
//----------------------------------------------------------------------------------------------------------------------

// These are required objects for the shader to work.
// You don't need to change anything here, unless you know what you are doing
sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
sampler_state pointsSampler {
    Filter    = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};

struct VertData {
    float2 uv  : TEXCOORD0;
    float4 pos : POSITION;
};

struct FragData {
    float2 uv  : TEXCOORD0;
};

VertData VSDefault(VertData v_in)
{
    VertData vert_out;
    vert_out.uv  = v_in.uv;
    vert_out.pos = mul(float4(v_in.pos.xyz, 1.0), ViewProj);
    return vert_out;
}
//----------------------------------------------------------------------------------------------------------------------

float rand2(float2 co){
	float v = sin(dot(co, float2(12.9898, 78.233))) * 43758.5453;
	return fract(v);
}
float rand(float a, float b) {
	return rand2(float2(a, b));
}

bool inside_box(float2 v, float2 left_top, float2 right_bottom) {
    float2 s = step(left_top, v) - step(right_bottom, v);
    return s.x * s.y != 0.0;
}

float2 uv_to_ortho(float2 uv) {
    if (!preserve_aspect_ratio) {
        return uv;
    }
    float aspect_ratio = vpixel/upixel;
    float2 ortho_correction = float2(aspect_ratio, 1.0);
    float2 uv_ortho = uv * ortho_correction;
    return uv_ortho;
}

float2 ortho_to_uv(float2 uv_ortho) {
    if (!preserve_aspect_ratio) {
        return uv_ortho;
    }
    float aspect_ratio = vpixel/upixel;
    float2 ortho_correction = float2(aspect_ratio, 1.0);
    float2 uv = uv_ortho / ortho_correction;
    return uv;
}
//----------------------------------------------------------------------------------------------------------------------

float4 EffectLinear(float2 uv) {
    float2 uv_ortho = uv_to_ortho(uv);
    float2 tex_size = uv_to_ortho(float2(1.0, 1.0));

    float2 face_tip = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((1 + 0.5)/478.0, 0)).xy);
    float2 face_center = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((197 + 0.5)/478.0, 0)).xy);
    float2 face_top = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((10 + 0.5)/478.0, 0)).xy);
    float2 face_bottom = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((152 + 0.5)/478.0, 0)).xy);

    if (!fd_face_found) {
        return image.Sample(textureSampler, uv);
    }

    float2 face_left1 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((234 + 0.5)/478.0, 0)).xy);
    float2 face_left2 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((227 + 0.5)/478.0, 0)).xy);
    float2 face_left3 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((116 + 0.5)/478.0, 0)).xy);
    float2 face_left  = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((117 + 0.5)/478.0, 0)).xy);
    if (face_left.x > face_left1.x) {
        face_left = face_left1;
    }
    if (face_left.x > face_left2.x) {
        face_left = face_left2;
    }
    if (face_left.x > face_left3.x) {
        face_left = face_left3;
    }
    if (face_left.x > face_tip.x) {
        face_left = face_tip;
    }

    float2 face_right1 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((454 + 0.5)/478.0, 0)).xy);
    float2 face_right2 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((447 + 0.5)/478.0, 0)).xy);
    float2 face_right3 = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((345 + 0.5)/478.0, 0)).xy);
    float2 face_right  = uv_to_ortho(fd_points_tex.Sample(pointsSampler, float2((346 + 0.5)/478.0, 0)).xy);
    if (face_right.x < face_right1.x) {
        face_right = face_right1;
    }
    if (face_right.x < face_right2.x) {
        face_right = face_right2;
    }
    if (face_right.x < face_right3.x) {
        face_right = face_right3;
    }
    if (face_right.x < face_tip.x) {
        face_right = face_tip;
    }

    float2 top_left = float2(face_left.x, face_top.y);
    float2 bottom_right = float2(face_right.x, face_bottom.y);
    float2 zoomed_region_size = bottom_right - top_left;

    if (preserve_aspect_ratio) {
        float aspect_ratio = vpixel/upixel;
        float zoomed_region_aspect_ratio = zoomed_region_size.x / zoomed_region_size.y;
        if (zoomed_region_aspect_ratio < aspect_ratio) {
            float new_w = zoomed_region_size.y * aspect_ratio;
            top_left.x -= (new_w - zoomed_region_size.x) * 0.5;
            bottom_right.x += (new_w - zoomed_region_size.x) * 0.5;
            zoomed_region_size.x = new_w;
        }
        else if (zoomed_region_aspect_ratio > aspect_ratio) {
            float new_h = zoomed_region_size.x / aspect_ratio;
            top_left.y -= (new_h - zoomed_region_size.y) * 0.5;
            bottom_right.y += (new_h - zoomed_region_size.y) * 0.5;
            zoomed_region_size.y = new_h;
        }
    }
    float2 center = (top_left + bottom_right) * 0.5;

    float2 zoomed_region_size_scaled = zoomed_region_size / zoom_level;
    zoomed_region_size_scaled = clamp(zoomed_region_size_scaled, float2(0.0, 0.0), tex_size);

    // Compute eligible area for the center based on the zoom level
    float2 eligible_area_top_left = uv_to_ortho(float2(0.5, 0.5)) - (tex_size - zoomed_region_size_scaled)*0.5;
    float2 eligible_area_bottom_right = uv_to_ortho(float2(0.5, 0.5)) + (tex_size - zoomed_region_size_scaled)*0.5;
    center = max(center, eligible_area_top_left);
    center = min(center, eligible_area_bottom_right);

    float2 top_left_scaled = center - (zoomed_region_size_scaled) * 0.5;
    float2 bottom_right_scaled = center + (zoomed_region_size_scaled) * 0.5;

    float2 uv_scaled = top_left_scaled + zoomed_region_size_scaled * uv;

    if (show_ref_points) {
        float uref = upixel*2;
        float vref = vpixel*2;
        float2 uvref = float2(0.002,0.002);
        if (inside_box(uv_ortho, face_center - uvref, face_center + uvref)) {
            return float4(1.0, 1.0, 1.0, 1.0);
        }
        if (inside_box(uv_ortho, face_left - uvref, face_left + uvref)) {
            return float4(1.0, 1.0, 0.0, 1.0);
        }
        if (inside_box(uv_ortho, face_right - uvref, face_right + uvref)) {
            return float4(0.0, 1.0, 1.0, 1.0);
        }
        if (inside_box(uv_ortho, face_top - uvref, face_top + uvref)) {
            return float4(1.0, 0.0, 0.0, 1.0);
        }
        if (inside_box(uv_ortho, face_bottom - uvref, face_bottom + uvref)) {
            return float4(1.0, 0.0, 0.0, 1.0);
        }

        float4 px_orig = image.Sample(textureSampler, uv);

        if (inside_box(uv_ortho, top_left_scaled, top_left_scaled + zoomed_region_size_scaled)) {
            px_orig = lerp(px_orig, float4(0.0, 1.0, 0.0, 1.0), 0.5);
        }
        if (inside_box(uv_ortho, top_left, top_left + zoomed_region_size)) {
            px_orig = lerp(px_orig, float4(1.0, 0.0, 0.0, 1.0), 0.5);
        }
        if (inside_box(uv_ortho, eligible_area_top_left, eligible_area_bottom_right)) {
            px_orig = lerp(px_orig, float4(0.0, 0.0, 1.0, 1.0), 0.5);
        }

        return lerp(
            px_orig,
            image.Sample(textureSampler, ortho_to_uv(uv_scaled)),
            0.3
        );
    }
    else {
        return image.Sample(textureSampler, ortho_to_uv(uv_scaled));
    }
}
//----------------------------------------------------------------------------------------------------------------------

// You probably don't want to change anything from this point.

float4 PSEffect(FragData f_in) : TARGET
{
    float4 rgba = EffectLinear(f_in.uv);
    return rgba;
}

technique Draw
{
    pass
    {
        vertex_shader = VSDefault(v_in);
        pixel_shader = PSEffect(f_in);
    }
}
