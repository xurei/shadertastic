// Common parameters for all shaders, as reference. Do not uncomment this (but you can remove it safely).
/*
uniform float time;            // Time since the shader is running. Goes from 0 to 1 for transition effects; goes from 0 to infinity for filter effects
uniform texture2d image;       // Texture of the source (filters only)
uniform texture2d tex_interm;  // Intermediate texture where the previous step will be rendered (for multistep effects)
uniform float upixel;          // Width of a pixel in the UV space
uniform float vpixel;          // Height of a pixel in the UV space
uniform float rand_seed;       // Seed for random functions
uniform int current_step;      // index of current step (for multistep effects)
*/

// Specific parameters of the shader. They must be defined in the meta.json file next to this one.
uniform float ghost_strength;
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
    return lerp(
        image.Sample(textureSampler, uv),
        prev_tex.Sample(textureSampler, uv),
        ghost_strength
    );
    /*
    if (current_step == 0) {
        if (uv[0] > 0.5 && uv[1] > 0.5) {
            return prev_tex.Sample(textureSampler, (uv-0.5)*2);
        }
        float4 result = float4(1.0, 0.0, 0.5, 1.0);
        return result * image.Sample(textureSampler, uv);
    }
    else {
        if (uv[0] > 0.5 && uv[1] > 0.5) {
            return prev_tex.Sample(textureSampler, (uv-0.5)*2);
        }
        return image.Sample(textureSampler, uv);
    }
    */
    /*float t2 = fmod(time, 0.2);
    if (t2 < 0.01) {
        return image.Sample(textureSampler, uv);
    }
    else {
        return prev_tex.Sample(textureSampler, uv );
    }

    return lerp(
        image.Sample(textureSampler, uv),
        prev_tex.Sample(textureSampler, uv),
        ghost_strength
    );*/
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
