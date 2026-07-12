uniform int blur_level_x;
uniform int blur_level_y;
uniform float sigma;
//----------------------------------------------------------------------------------------------------------------------

sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
//----------------------------------------------------------------------------------------------------------------------

float gaussian(float x) {
    return exp(-x*x);
    //Approximation of the gaussian using cos
    //return (1.0 + cos(x*3.0));
}

float4 getGaussianU(float2 uv, int nb_samples) {
    float4 px_out = image.Sample(textureSampler, uv);
    float gaussian_sum_alpha = gaussian(0);
    float gaussian_sum = gaussian_sum_alpha * px_out[3];
    px_out.rgb *= px_out[3];
    float nb_samples_f = float(nb_samples);
    #ifdef _D3D11
    [unroll(100)]
    #endif
    for (int i=1; i<nb_samples; ++i) {
        float du = i*upixel;
        float4 px_right = image.Sample(textureSampler, float2(uv[0]+du, uv[1]));
        float4 px_left = image.Sample(textureSampler, float2(uv[0]-du, uv[1]));

        float k = gaussian(float(i) / nb_samples_f * clamp(sigma, 1.0, 100.0));
        float alpha_impact = k * (px_right.a + px_left.a);
        px_out.rgb += k * (px_right.rgb*px_right.a + px_left.rgb*px_left.a);
        px_out.a += alpha_impact;
        gaussian_sum += alpha_impact;
        gaussian_sum_alpha += 2*k;
    }
    px_out.rgb /= gaussian_sum;
    px_out[3] /= gaussian_sum_alpha;
    return px_out;
}

float4 getGaussianV(float2 uv, int nb_samples) {
    float4 px_out = tex_interm.Sample(textureSampler, uv);
    float gaussian_sum_alpha = gaussian(0);
    float gaussian_sum = gaussian_sum_alpha * px_out[3];
    px_out.rgb *= px_out[3];
    float nb_samples_f = float(nb_samples);
    #ifdef _D3D11
    [unroll(100)]
    #endif
    for (int i=1; i<nb_samples; ++i) {
        float dv = i*vpixel;
        float4 px_right = tex_interm.Sample(textureSampler, float2(uv[0], uv[1]+dv));
        float4 px_left = tex_interm.Sample(textureSampler, float2(uv[0], uv[1]-dv));

        float k = gaussian(float(i) / nb_samples_f * clamp(sigma, 1.0, 100.0));
        float alpha_impact = k * (px_right.a + px_left.a);
        px_out.rgb += k * (px_right.rgb*px_right.a + px_left.rgb*px_left.a);
        px_out.a += alpha_impact;
        gaussian_sum += alpha_impact;
        gaussian_sum_alpha += 2*k;
    }
    px_out.rgb /= gaussian_sum;
    px_out[3] /= gaussian_sum_alpha;
    return px_out;
}

float4 EffectLinear(float2 uv)
{
    if (current_step == 0) {
        int nb_samples = max(1, blur_level_x);
        return getGaussianU(uv, nb_samples);
    }
    else {
        int nb_samples = max(1, blur_level_y);
        return getGaussianV(uv, nb_samples);
    }
}
//----------------------------------------------------------------------------------------------------------------------

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
