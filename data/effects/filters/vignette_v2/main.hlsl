uniform float strength;
uniform float curvature;
uniform float width;
uniform float height;
uniform float smoothing;
uniform float center_x;
uniform float center_y;
uniform bool alpha_vignette;
uniform bool keep_aspect_ratio;
uniform bool invert_effect;
//----------------------------------------------------------------------------------------------------------------------

sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
//----------------------------------------------------------------------------------------------------------------------

float4 EffectLinear(float2 uv)
{
    float4 px = image.Sample(textureSampler, uv);
    if (keep_aspect_ratio) {
        if (vpixel > upixel) {
            uv.x -= 0.5;
            uv.x *= vpixel/upixel;
            uv.x += 0.5;
        }
        else {
            uv.y -= 0.5;
            uv.y *= upixel/vpixel;
            uv.y += 0.5;
        }
    }
    float border = height / 500.0;
    float aspect_ratio_modifier = width / height;
    float2 center = float2(center_x*0.001, center_y*0.001);

    float outer = border * (1.0 + smoothing*0.001);
    float inner = border * (1.0 - smoothing*0.001);

    /*if (vpixel > upixel)*/ {
        uv.x -= center.x;
        uv.x /= aspect_ratio_modifier;
        uv.x += center.x;
    }
    /*else {
        uv.y -= center.y;
        uv.y /= aspect_ratio_modifier;
        uv.y += center.y;
    }*/

    float2 uv_centered = (uv-center)*2.0;

    //Calculate edge curvature
    float inv_curvature = 1.0 / curvature;
    float2 curve = pow(abs(uv_centered), float2(inv_curvature,inv_curvature));
    //Compute distance to edge
    float edge = pow(length(curve), curvature);

    //Compute vignette gradient and intensity
    float vignette = 1.0 - strength*smoothstep(inner, outer, edge);

    vignette = invert_effect ? 1.0 - vignette : vignette;

    px.rgb = srgb_linear_to_nonlinear(px.rgb);
    px.rgb *= alpha_vignette ? 1.0 : vignette;
    px.rgb = srgb_nonlinear_to_linear(px.rgb);

    px.a *= alpha_vignette ? vignette : 1.0;

    return px;
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
