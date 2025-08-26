uniform int mode;
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

float4 HueShift (float4 pixel, float Shift)
{
    float3 P = float3(0.55735,0.55735,0.55735)*dot(float3(0.55735,0.55735,0.55735),pixel.xyz);
    float3 U = pixel.xyz-P;
    float3 V = cross(float3(0.55735,0.55735,0.55735),U);

    pixel.xyz = U*cos(Shift*6.2832) + V*sin(Shift*6.2832) + P;
    return float4(pixel.xyz, pixel.a);
}


float sinplus1(float v) {
    return (1.0 + sin(v)) / 2.0;
}
float cosplus1(float v) {
    return (1.0 + cos(v)) / 2.0;
}

float sinwaves(float val) {
    return (
        0.2*sinplus1(8.0*val) +
        0.1*cosplus1(13.0*val) +
        0.2*sinplus1(27.0*val + 0.2) +
        0.15*sinplus1(41.0*val) +
        0.05*cosplus1(87.0*val) +
        0.01*sinplus1(107.0*val)
    ) / (0.2+0.1+0.2+0.15+0.05+0.01);
}

float4 EffectLinear(float2 uv)
{
    float4 pixel = image.Sample(textureSampler, uv);
    if (mode == 0) {
        // Linear
        pixel = HueShift(pixel, time - int(time));
    }
    if (mode == 1) {
        // Random
        pixel = HueShift(pixel, sinwaves(time - int(time)));
    }

    return pixel;
}
//----------------------------------------------------------------------------------------------------------------------

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
