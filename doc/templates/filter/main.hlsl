// Common parameters for all shaders, as reference. Do not uncomment this (but you can remove it safely).
/*
uniform texture2d image;       // Texture of the source (filters only)
uniform float time;            // Time since the shader is running. Goes from 0 to infinity for filter effects
uniform float delta_time;      // Time elapsed since the previous frame
uniform int frame_index;       // Number of frames since the filter has been activated, or reset (see the "time" parameter type)
uniform float upixel;          // Width of a pixel in the UV space; equivalent of 1.0 / texture_width
uniform float vpixel;          // Height of a pixel in the UV space; equivalent of 1.0 / texture_height
uniform float rand_seed;       // Seed for pseudo-random functions; when used, allows effect to be different every time
uniform int nb_steps;          // number of steps (for multisteps effects)
uniform int current_step;      // index of current step (for multistep effects)
uniform texture2d tex_interm;  // Intermediate texture where the previous step will be rendered (for multistep effects)
*/

// Specific parameters of the shader. They must be defined in the meta.json file next to this one.
uniform float random_amount;
//----------------------------------------------------------------------------------------------------------------------

// Samplers: these define how the texture pixels are interpolated when you sample them.
// Two samplers are defined by default: textureSampler and pointSampler.
// You can adapt or remove them if you like
sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Mirror;
    AddressV  = Mirror;
};
sampler_state pointSampler {
    Filter    = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
//----------------------------------------------------------------------------------------------------------------------

float4 EffectLinear(float2 uv)
{
    // -----> THE CODE OF THE EFFECT GOES HERE <-----

    // Here is a basic example that will flip the image
    uv[0] = 1-uv[0];
    return image.Sample(textureSampler, uv);
}
//----------------------------------------------------------------------------------------------------------------------

// These are required objects and functions for the shader to work in OBS.
// You probably don't want to change anything from this point.

struct FragData {
    float2 uv  : TEXCOORD0;
};

struct VertData {
    float2 uv  : TEXCOORD0;
    float4 pos : POSITION;
};

float4 PSEffect(FragData f_in) : TARGET
{
    float4 rgba = EffectLinear(f_in.uv);
    return rgba;
}

VertData VSDefault(VertData v_in)
{
    VertData vert_out;
    vert_out.uv  = v_in.uv;
    vert_out.pos = mul(float4(v_in.pos.xyz, 1.0), ViewProj);
    return vert_out;
}

technique Draw
{
    pass
    {
        vertex_shader = VSDefault(v_in);
        pixel_shader = PSEffect(f_in);
    }
}
