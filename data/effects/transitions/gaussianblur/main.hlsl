uniform float break_point;
uniform float max_blur_level;
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

float sigmoid(float strength, float n) {
    // Vanilla sigmoid ; this is not symetric between 0 and 1
    //return (1 / (1 + exp(-n)));

    n = strength * (n*2 - 1);
    float v0 = 1 / (1 + exp(strength));
    float v1 = 1 - v0; // simplified from v1 = (1 / (1 + exp(-1 * strength)));

    return (1 / (1 + exp(-n)) - v0) / (v1-v0);
}

float sigmoid_centered(float strength, float center, float n) {
    if (center < 0.5) {
        float width = 2 * center;
        return sigmoid(strength, n/width);
    }
    else {
        float width = 2 * (1-center);
        float m = (n - center + width/2) / width;
        return sigmoid(strength, m);
    }
}

float gaussian(float x) {
    return exp(-x*x);
}

float4 getGaussianU(float2 uv, int nb_samples) {
    float4 px_out = tex_interm.Sample(textureSampler, uv);
    float gaussian_sum_alpha = gaussian(0);
    float gaussian_sum = gaussian_sum_alpha * px_out[3];
    px_out.rgb *= px_out[3];
    float nb_samples_f = float(nb_samples);
    #ifdef _D3D11
    [unroll(50)]
    #endif
    for (int i=1; i<nb_samples; ++i) {
        float du = i*upixel;
        float4 px_right = tex_interm.Sample(textureSampler, float2(uv[0]+du, uv[1]));
        float4 px_left = tex_interm.Sample(textureSampler, float2(uv[0]-du, uv[1]));

        float k = gaussian(float(i) / nb_samples_f);
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
    [unroll(50)]
    #endif
    for (int i=1; i<nb_samples; ++i) {
        float dv = i*vpixel;
        float4 px_right = tex_interm.Sample(textureSampler, float2(uv[0], uv[1]+dv));
        float4 px_left = tex_interm.Sample(textureSampler, float2(uv[0], uv[1]-dv));

        float k = gaussian(float(i) / nb_samples_f);
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
        float lerp_ratio = sigmoid_centered(10, break_point, time);
        float4 px_a = tex_a.Sample(textureSampler, uv);
        float4 px_b = tex_b.Sample(textureSampler, uv);
        float4 px_out = lerp(
            px_a,
            px_b,
            lerp_ratio
        );
        px_out.rgb = lerp(
            px_a.rgb * px_a.a,
            px_b.rgb * px_b.a,
            lerp_ratio
        ) / px_out.a;
        return px_out;
    }
    else {
        float t = time;
        if (t < break_point) {
            t = sigmoid_centered(5, break_point/2.0, t);
        }
        else {
            t = -1 * sigmoid_centered(5, break_point + (1.0-break_point) / 2.0, t) + 1;
        }

        int nb_samples = int(max(1, 10*max_blur_level * t));

        if (current_step == 1) {
            return getGaussianU(uv, nb_samples);
        }
        else {
            return getGaussianV(uv, nb_samples);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

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
