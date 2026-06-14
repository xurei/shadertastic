// Common parameters for all shaders, as reference. Do not uncomment this (but you can remove it safely).
/*
uniform texture2d tex_a;       // Texture of the previous scene (transitions only)
uniform texture2d tex_b;       // Texture of the next scene (transitions only)
uniform float time;            // Time since the shader is running. Goes from 0 to 1 for transition effects
uniform float delta_time;      // Time elapsed (in seconds!) since the previous frame.
uniform int frame_index;       // Number of frames since the transition has started
uniform float upixel;          // Width of a pixel in the UV space; equivalent of 1.0 / texture_width
uniform float vpixel;          // Height of a pixel in the UV space; equivalent of 1.0 / texture_height
uniform float rand_seed;       // Seed for pseudo-random functions; when used, allows effect to be different every time
uniform bool is_studio_mode;   // True if OBS is using the transition slider of the studio mode. Mostly useful for developers
uniform int nb_steps;          // number of steps (for multisteps effects)
uniform int current_step;      // index of current step (for multistep effects)
uniform texture2d tex_interm;  // Intermediate texture where the previous step will be rendered (for multistep effects)
*/

// Specific parameters of the shader. They must be defined in the meta.json file next to this one.
uniform float template_float;
//----------------------------------------------------------------------------------------------------------------------

// These are required objects for the shader to work.
// You don't need to change anything here, unless you know what you are doing
sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
//----------------------------------------------------------------------------------------------------------------------

float4 EffectLinear(float2 uv)
{
    // -----> YOUR CODE GOES HERE <-----

    // Here is a basic example that will progressively show the next scene from right to left :
    if (uv[0] < 1-time) {
        return tex_a.Sample(textureSampler, uv);
    }
    else {
        return tex_b.Sample(textureSampler, uv);
    }
}
//----------------------------------------------------------------------------------------------------------------------

// You probably don't want to change anything from this point.

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
