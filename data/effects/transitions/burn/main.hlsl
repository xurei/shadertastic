uniform float random_noise;
uniform float smooth_level;
//----------------------------------------------------------------------------------------------------------------------

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

float getY(float3 val) {
    //Return Y value in the colorspace YUV (without the constant at the end of the calc)
    return (0.257*val.r + 0.504*val.g + 0.098*val.b) / 0.859;
}

float distFromCenter(float2 uv) {
    return length(uv - float2(0.5, 0.5));
}

float4 BurnPixels(float2 uv, float t, float4 px_a, float4 px_b) {
    float4 px_b_circle = px_b;
    px_b_circle.rgb = px_b.rgb / (1.0 + distFromCenter(uv) + random_noise*rand2(uv));

    float y_a = getY(px_a.rgb * px_a.a);
    float y_b = getY(px_b_circle.rgb * px_b.a);
    float avg = (y_a+y_b)/2.0;

    float smooth_avg = (1-t)*smooth_level*0.5;

    if (avg < (1-t) - smooth_avg) {
        return px_a;
    }
    else if (avg < (1-t)) {
        float4 px_out;
        float ratio = (avg - ((1-t) - smooth_avg)) / smooth_avg;
//        return float4(ratio,ratio,ratio,1.0);
        px_out.a = lerp(px_a.a, px_b.a, ratio);
        px_out.rgb = lerp(
            px_a.rgb * px_a.a,
            px_b.rgb * px_b.a,
            ratio
        ) / px_out.a;
        return px_out;
    }
    else {
        return px_b;
    }
}

float4 EffectLinear(float2 uv)
{
    float t = pow(time, 1.0/1.5);
    float4 px_a = tex_a.Sample(textureSampler, uv);
    float4 px_b = tex_b.Sample(textureSampler, uv);
    return BurnPixels(uv, t, px_a, px_b);
}
//----------------------------------------------------------------------------------------------------------------------

float4 PSEffect(FragData f_in) : TARGET
{
    float4 rgba = EffectLinear(f_in.uv);
    rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
    return rgba;
}

float4 PSEffectLinear(FragData f_in) : TARGET
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

technique DrawLinear
{
    pass
    {
        vertex_shader = VSDefault(v_in);
        pixel_shader = PSEffectLinear(f_in);
    }
}
