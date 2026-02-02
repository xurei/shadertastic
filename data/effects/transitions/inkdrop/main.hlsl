uniform bool do_color_displace;
//----------------------------------------------------------------------------------------------------------------------

// These are required objects for the shader to work.
// You don't need to change anything here, unless you know what you are doing
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

#define TAU 6.28318530718

#define SPEED 1.1

float rand(float2 n) {
    return fract(sin(dot(n, float2(12.9898, 4.1414))) * 43758.5453);
}

float noise(float2 p){
    float2 ip = floor(p);
    float2 u = float2(fract(p[0]), fract(p[1]));
    u = u*u*(3.0-2.0*u);

    float res = lerp(
        lerp(rand(ip+rand_seed),rand(ip+rand_seed+float2(1.0,0.0)),u.x),
        lerp(rand(ip+rand_seed+float2(0.0,1.0)),rand(ip+rand_seed+float2(1.0,1.0)),u.x),
        u.y
    );
    return res*res;
}

float fbm(float2 p, int octaves)
{
    float n = 0.0;
    float a = 1.0;
    float norm = 0.0;
    for(int i = 0; i < octaves; ++i)
    {
        n += noise(p) * a;
        norm += a;
        p *= 2.0;
        a *= 0.5;
    }
    return n / norm;
}

float4 EffectLinear( float2 uv_orig )
{
    float2 uv = uv_orig - float2(0.5, 0.5);

    float angle_orig = atan2(uv.y, uv.x);
    float angle = angle_orig + fbm(uv * 4.0, 3) * 0.5;
    float2 p = float2(cos(angle), sin(angle));

    float t = time * SPEED;
    t *= t;

    float l = dot(uv / t, uv / t);
    l -= (fbm(normalize(uv) * 3.0, 5) - 0.5);

    float ink1 = fbm(p * 8.0, 4) + 1.5 - l;
    ink1 = clamp(ink1, 0.0, 1.0);

    float ink2 = fbm(p * (8.0+rand_seed), 4) + 1.5 - l;
    ink2 = 1.0 - clamp(ink2, 0.0, 1.0);

    float2 displace = (1.0 - time) * ink2 * uv;

    float4 px_a = tex_a.Sample(textureSampler, uv_orig);
    float4 px_b = tex_b.Sample(textureSampler, uv_orig + (3.0 * displace));
    float4 px_out = lerp(px_a, px_b, ink1);

    if (do_color_displace) {
        float ink3 = fbm(float2(p.y, p.x) * (8.0+rand_seed), 4) + 1.5 - l;
        ink3 = 1.0 - clamp(ink3, 0.0, 1.0);
        float ink4 = fbm(float2(cos(angle+1.0), sin(angle+1.0)) * (8.0+rand_seed), 4) + 1.5 - l;
        ink4 = 1.0 - clamp(ink4, 0.0, 1.0);
        float2 displace2 = (1.0 - time) * ink3 * uv;
        float2 displace3 = (1.0 - time) * ink4 * uv;

        float4 px_b2 = tex_b.Sample(textureSampler, uv_orig + (3.0 * displace2));
        float4 px_b3 = tex_b.Sample(textureSampler, uv_orig + (3.0 * displace3));
        px_out.r = lerp(
            px_a.r*px_a.a,
            px_b.r*px_b.a,
            ink1
        ) / px_out.a;
        px_out.g = lerp(
            px_a.g*px_a.a,
            px_b2.g*px_b.a,
            ink1
        ) / px_out.a;
        px_out.b = lerp(
            px_a.b*px_a.a,
            px_b3.b*px_b.a,
            ink1
        ) / px_out.a;
    }
    else {
        px_out.rgb = lerp(
            px_a.rgb*px_a.a,
            px_b.rgb*px_b.a,
            ink1
        ) / px_out.a;
    }

    return px_out;
}
//----------------------------------------------------------------------------------------------------------------------

// You probably don't want to change anything from this point.

float4 PSEffect(FragData f_in) : TARGET
{
    float4 rgba = EffectLinear(f_in.uv);
    if (current_step == nb_steps - 1) {
        rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
    }
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
