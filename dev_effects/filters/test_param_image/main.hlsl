// Common parameters for all shaders, as reference. Do not uncomment this (but you can remove it safely).
/*
uniform float time;            // Time since the shader is running. Goes from 0 to 1 for transition effects; goes from 0 to infinity for filter effects
uniform texture2d image;       // Texture of the source (filters only)
uniform texture2d tex_interm;  // Intermediate texture where the previous step will be rendered (for multistep effects)
uniform float upixel;          // Width of a pixel in the UV space
uniform float vpixel;          // Height of a pixel in the UV space
uniform float rand_seed;       // Seed for random functions
uniform int current_step;      // index of current step (for multistep effects)
uniform int nb_steps;          // number of steps (for multistep effects)
*/

// Specific parameters of the shader. They must be defined in the meta.json file next to this one.
uniform texture2d tex_orig;
uniform texture2d tex_editable;
uniform texture2d tex_fixed;
uniform texture2d tex_empty;
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

//Here goes your implementation !

float4 EffectLinear(float2 uv)
{
    // -----> YOUR CODE GOES HERE <-----
    if (uv.x < 0.25) {
        return tex_orig.Sample(textureSampler, uv * float2(4.0, 1.0));
    }
    if (uv.x < 0.50) {
        return tex_editable.Sample(textureSampler, (uv - float2(0.25, 0.0)) * float2(4.0, 1.0));
    }
    if (uv.x < 0.75) {
        return tex_empty.Sample(textureSampler, (uv - float2(0.50, 0.0)) * float2(4.0, 1.0));
    }
    else {
        return tex_fixed.Sample(textureSampler, (uv - float2(0.75, 0.0)) * float2(4.0, 1.0));
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
