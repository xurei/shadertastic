uniform float4x4 ViewProj;
uniform texture2d image;
uniform float2 center;
uniform float2 crop_size;
uniform float rotation;
uniform float aspect_ratio;

sampler_state textureSampler {
    Filter    = Linear;
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

VertData VSDefault(VertData v_in) {
    VertData vert_out;
    vert_out.uv  = v_in.uv;
    vert_out.pos = mul(float4(v_in.pos.xyz, 1.0), ViewProj);
    return vert_out;
}
//----------------------------------------------------------------------------------------------------------------------

float2 uv_to_ortho(float2 uv) {
    float2 ortho_correction = float2(aspect_ratio, 1.0);
    float2 position = center * ortho_correction;

    float2 uv_orthonormal = uv * ortho_correction;
    float2 uv_local = uv_orthonormal - position;
    return uv_local;
}

float2 ortho_to_uv(float2 uv_local) {
    float2 ortho_correction = float2(aspect_ratio, 1.0);
    float2 position = center * ortho_correction;

    float2 uv_orthonormal = uv_local + position;
    float2 uv = uv_orthonormal / ortho_correction;
    return uv;
}
//----------------------------------------------------------------------------------------------------------------------

float4 EffectLinear(float2 uv) {
    // cropping
    float2 top_left = center - crop_size * 0.5;
    float2 uv_cropped = top_left + crop_size * uv;

    // rotating the whole image around the center
    float2 uv_ortho = uv_to_ortho(uv_cropped);
    float cos_angle = cos(rotation);
    float sin_angle = sin(rotation);
    float2 uv_ortho_rotated = float2(
        dot(float2(cos_angle, -sin_angle), uv_ortho),
        dot(float2(sin_angle, cos_angle), uv_ortho)
    );
    float2 uv_rotated = ortho_to_uv(uv_ortho_rotated);

    float2 uv_out = uv_rotated;

    float4 img_px = image.Sample(textureSampler, uv_out);
    return img_px;
}

float4 PSEffectLinear(FragData f_in) : TARGET {
    float4 rgba = EffectLinear(f_in.uv);
    return rgba;
}

technique DrawLinear {
    pass
    {
        vertex_shader = VSDefault(v_in);
        pixel_shader = PSEffectLinear(f_in);
    }
}
