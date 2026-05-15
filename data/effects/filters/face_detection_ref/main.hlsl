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
uniform bool facetracking_face_found;
uniform float2 facetracking_bbox_tl;
uniform float2 facetracking_bbox_br;
uniform texture2d facetracking_points_tex;
uniform texture2d facetracking_preraster_tex;
uniform bool show_tex;
uniform bool show_mesh_face;
uniform bool show_mesh_lips;
uniform bool show_bounding_box;
uniform bool show_eyes;
uniform bool show_eyes_balls;
uniform bool show_mesh_preraster;
uniform float base_mult;
//----------------------------------------------------------------------------------------------------------------------

// These are required objects for the shader to work.
// You don't need to change anything here, unless you know what you are doing
sampler_state textureSampler {
    Filter    = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
sampler_state pointsSampler {
    Filter    = Point;
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

float4 EffectLinear(float2 uv)
{
    if (show_tex && uv.y < 0.05) {
        float4 px = facetracking_points_tex.Sample(textureSampler, uv / float2(1.0, 0.05));
        return px;
    }

    float4 px = show_mesh_preraster ? facetracking_preraster_tex.Sample(textureSampler, uv) : image.Sample(textureSampler, uv);

    if (px[3] <= 0.000001 || px.x+px.y+px.z <= 0.000001) {
        px = image.Sample(textureSampler, uv);
    }
    float4 mult = float4(base_mult, base_mult, base_mult , 1.0);

    float aspectRatio = vpixel/upixel;
    float2 orthoCorrection = float2(aspectRatio, 1.0);
    float2 uv_ortho = uv * orthoCorrection;

    if (show_eyes_balls) {
        float2 fd_leye_center = facetracking_points_tex.Sample(pointsSampler, float2((468.5)/478.0, 0)).xy * orthoCorrection;
        float2 fd_leye_top = facetracking_points_tex.Sample(pointsSampler, float2((470.5)/478.0, 0)).xy * orthoCorrection;
        float fd_leye_radius = abs(fd_leye_center.y - fd_leye_top.y);
        if (distance(uv_ortho, fd_leye_center) < fd_leye_radius) {
            return lerp(px, float4(1.0, 0.0, 0.0, 1.0), 0.4);
        }

        float2 fd_reye_center = facetracking_points_tex.Sample(pointsSampler, float2((473.5)/478.0, 0)).xy * orthoCorrection;
        float2 fd_reye_top = facetracking_points_tex.Sample(pointsSampler, float2((475.5)/478.0, 0)).xy * orthoCorrection;
        float fd_reye_radius = abs(fd_reye_center.y - fd_reye_top.y);
        if (distance(uv_ortho, fd_reye_center) < fd_reye_radius) {
            return lerp(px, float4(1.0, 1.0, 0.0, 1.0), 0.4);
        }
    }

    if (show_bounding_box) {
        if (
            (facetracking_bbox_tl.x - upixel <= uv.x && uv.x <= facetracking_bbox_tl.x + upixel) ||
            (facetracking_bbox_br.x - upixel <= uv.x && uv.x <= facetracking_bbox_br.x + upixel)
        ) {
            if (facetracking_bbox_tl.y <= uv.y && uv.y <= facetracking_bbox_br.y) {
                return float4(0.6, 0.1, 0.7, 1.0);
            }
        }
        if (
            (facetracking_bbox_tl.y - vpixel <= uv.y && uv.y <= facetracking_bbox_tl.y + vpixel) ||
            (facetracking_bbox_br.y - vpixel <= uv.y && uv.y <= facetracking_bbox_br.y + vpixel)
        ) {
            if (facetracking_bbox_tl.x <= uv.x && uv.x <= facetracking_bbox_br.x) {
                return float4(0.6, 0.1, 0.7, 1.0);
            }
        }
    }

    if (facetracking_bbox_tl.x - upixel <= uv.x && uv.x <= facetracking_bbox_br.x + upixel && facetracking_bbox_tl.y - vpixel <= uv.y && uv.y <= facetracking_bbox_br.y + vpixel) {
        mult = float4(1.0, 1.0, 1.0, 1.0);

        #ifdef _D3D11
        [loop]
        #endif
        for (int i=0; i<478; ++i) {
            float4 px2 = facetracking_points_tex.Sample(pointsSampler, float2((i + 0.5)/478.0, 0));
//            px2[2] = 1.0;
//            px2[3] = 1.0;

            float2 ptuv = float2(px2.x, px2.y);

            bool isEyes = (
                i > 467
            );

            /*if (isEyes) {
                continue;
            }*/

            bool isLipsUp = (
                i == 0 ||
                i == 11 ||
                i == 12 ||
                i == 13 ||
                i == 37 ||
                i == 38 ||
                i == 39 ||
                i == 40 ||
                i == 41 ||
                i == 42 ||
                i == 61 ||
                i == 62 ||
                i == 72 ||
                i == 73 ||
                i == 74 ||
                i == 76 ||
                i == 78 ||
                i == 80 ||
                i == 81 ||
                i == 82 ||
                i == 183 ||
                i == 184 ||
                i == 185 ||
                i == 191 ||
                i == 267 ||
                i == 268 ||
                i == 269 ||
                i == 270 ||
                i == 271 ||
                i == 272 ||
                i == 291 ||
                i == 292 ||
                i == 302 ||
                i == 303 ||
                i == 304 ||
                i == 306 ||
                i == 308 ||
                i == 310 ||
                i == 311 ||
                i == 312 ||
                i == 407 ||
                i == 408 ||
                i == 409 ||
                i == 415
            );

            bool isLipsDown = (
                i == 14 ||
                i == 15 ||
                i == 16 ||
                i == 17 ||
                i == 77 ||
                i == 84 ||
                i == 85 ||
                i == 86 ||
                i == 87 ||
                i == 88 ||
                i == 89 ||
                i == 90 ||
                i == 91 ||
                i == 95 ||
                i == 96 ||
                i == 146 ||
                i == 178 ||
                i == 179 ||
                i == 180 ||
                i == 181 ||
                i == 307 ||
                i == 314 ||
                i == 315 ||
                i == 316 ||
                i == 317 ||
                i == 318 ||
                i == 319 ||
                i == 320 ||
                i == 321 ||
                i == 324 ||
                i == 325 ||
                i == 375 ||
                i == 402 ||
                i == 403 ||
                i == 404 ||
                i == 405
            );

            if (ptuv.x - upixel*2 <= uv.x && uv.x <= ptuv.x + upixel*2 && ptuv.y - vpixel*2 <= uv.y && uv.y <= ptuv.y + vpixel*2) {
                if (isLipsUp) {
                    if (show_mesh_lips) {
                        return float4(1.0, 0.0, 0.0, 1.0);
                    }
                }
                else if (isLipsDown) {
                    if (show_mesh_lips) {
                        return float4(0.3, 0.0, 0.0, 1.0);
                    }
                }
                else if (isEyes) {
                    if (show_eyes) {
                        return float4(1.0, 0.0, 0.0, 1.0);
                    }
                }
                else {
                    if (show_mesh_face) {
                        px2.z = 1.0 - (0.5 + px2.z*4.0);
                        return float4(0.0, px2.z, 0.0, 1.0);
                    }
                }
            }
        }
    }

    return mult*px;
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
