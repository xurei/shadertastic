// Common parameters for all shaders, as reference. Do not uncomment this (but you can remove it safely).
/*
uniform float time;            // Time since the shader is running. Goes from 0 to 1 for transition effects; goes from 0 to infinity for filter effects
uniform int frame_index;       // Auto incremented value of the current frame. Does not increment during steps
uniform texture2d image;       // Texture of the source (filters only)
uniform texture2d tex_interm;  // Intermediate texture where the previous step will be rendered (for multistep effects)
uniform float upixel;          // Width of a pixel in the UV space
uniform float vpixel;          // Height of a pixel in the UV space
uniform float rand_seed;       // Seed for random functions
uniform int current_step;      // index of current step (for multistep effects)
uniform int nb_steps;          // number of steps (for multistep effects)
*/

// Specific parameters of the shader. They must be defined in the meta.json file next to this one.
uniform texture2d prev_image;
uniform float prev_alpha;
uniform float zoom;
uniform float center_x;
uniform float center_y;
uniform bool show_debug_point;
uniform int reset_on_toggle;
uniform int superposition_mode;
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

bool inside_box(float2 v, float2 left_top, float2 right_bottom) {
    float2 s = step(left_top, v) - step(right_bottom, v);
    return s.x * s.y != 0.0;
}

float4 EffectLinear(float2 uv)
{
    float2 uv2 = uv - float2(center_x, 1.0-center_y);
    float zoom_ratio;
    if (zoom > 0.0) {
        zoom_ratio = 1.0 - ( pow(100, zoom) - 1 ) / (100 - 1);  // Log scale magique : (a^x - 1) / (a - 1);
    }
    else {
        zoom_ratio = 1.0 + 16.0*abs( pow(100, -zoom) - 1 ) / (100 - 1);  // Log scale magique : (a^x - 1) / (a - 1)
    }

    if (current_step == 1) {
        float4 px_out = tex_interm.Sample(textureSampler, uv);
        if (show_debug_point) {
            if (center_x - upixel < uv.x && uv.x < center_x + upixel) {
                px_out.rgb = 1.0 - px_out.rgb;
                px_out.a = 1.0;
            }
            else if ((1.0-center_y) - vpixel < uv.y && uv.y < (1.0-center_y) + vpixel) {
                px_out.rgb = 1.0 - px_out.rgb;
                px_out.a = 1.0;
            }

            uv2 /= zoom_ratio;
            uv2 += float2(center_x, 1.0-center_y);
            if (0.0 - vpixel/zoom_ratio < uv2.y && uv2.y < 0.0 + vpixel/zoom_ratio) {
                if (0.0 <= uv2.x && uv2.x <= 1.0) {
                    return float4(1.0, 1.0, 0.0, 1.0);
                }
            }
            else if (1.0 - vpixel/zoom_ratio < uv2.y && uv2.y < 1.0 + vpixel/zoom_ratio) {
                if (0.0 <= uv2.x && uv2.x <= 1.0) {
                    return float4(1.0, 1.0, 0.0, 1.0);
                }
            }
            if (0.0 - upixel/zoom_ratio < uv2.x && uv2.x < 0.0 + upixel/zoom_ratio) {
                if (0.0 <= uv2.y && uv2.y <= 1.0) {
                    return float4(1.0, 1.0, 0.0, 1.0);
                }
            }
            else if (1.0 - upixel/zoom_ratio < uv2.x && uv2.x < 1.0 + upixel/zoom_ratio) {
                if (0.0 <= uv2.y && uv2.y <= 1.0) {
                    return float4(1.0, 1.0, 0.0, 1.0);
                }
            }
        }
        return px_out;
    }
    else {
        if (frame_index < 1 && reset_on_toggle == 0) {
            return image.Sample(textureSampler, uv);
        }

        uv2 = (uv2*zoom_ratio) + float2(center_x, 1.0-center_y);

        float r = length(uv2 * float2(vpixel/upixel, 1.0));

        float4 px = image.Sample(textureSampler, uv);
        float4 prev_px;

        if (uv2.x < 0.0 || uv2.x > 1.0 || uv2.y < 0.0 || uv2.y > 1.0) {
            prev_px = float4(0.0,0.0,0.0, 0.0);
        }
        else {
            #ifdef _D3D11
            [loop]
            #endif
            for (float du=-upixel; du <= upixel; du += upixel) {
                #ifdef _D3D11
                [loop]
                #endif
                for (float dv=-vpixel; dv <= vpixel; dv += vpixel) {
                    prev_px += prev_image.Sample(textureSampler, uv2 + float2(du,dv));
                }
            }
            prev_px *= 0.111111110;
        }

        if (superposition_mode == 1) {
            float4 tmp = prev_px;
            prev_px = px;
            px = tmp;
        }

        float alpha = px.a;
        alpha = max(alpha, lerp(prev_px.a*prev_alpha, px.a, px.a));
        alpha = clamp(alpha, 0.0, 1.0);

        float4 px_out = float4(0.0, 0.0, 0.0, alpha);
        px_out.rgb = lerp(
            prev_px.rgb,
            px.rgb,
            px.a
        );

        return px_out;
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
