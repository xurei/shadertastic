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

uniform int kernel_size;
uniform int filter_mode;
uniform float lightness;
uniform float orig_ratio;

#define FILTER_MODE_RGB 1
#define FILTER_MODE_YUV 4
#define FILTER_MODE_LUMINANCE 2
#define FILTER_MODE_LUMINANCE_ONLY 3

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

float3 rgb2yuvNormalized(float3 rgb) {
    return float3(
        ( 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b),
        (-0.148 * rgb.r - 0.291 * rgb.g + 0.439 * rgb.b),
        ( 0.439 * rgb.r - 0.368 * rgb.g - 0.071 * rgb.b)
    );
}

float3 yuvNormalized2rgb(float3 yuv) {
    return float3(
        yuv[0]                  + 1.596 * yuv[2],
        yuv[0] - 0.392 * yuv[1] - 0.813 * yuv[2],
        yuv[0] + 2.017 * yuv[1]
    );
}

float4 EffectLinear(float2 uv)
{
    float kernel[5 * 5];
    float divider = 0.0;

    if (kernel_size == 3) {
        int k = 0;
        divider = 2*47.0 + 162.0;
        kernel[k++] = 0.0; kernel[k++] =  0.0; kernel[k++] = 0.0; kernel[k++] =  0.0; kernel[k++] = 0.0;
        kernel[k++] = 0.0; kernel[k++] = -47.0; kernel[k++] = 0.0; kernel[k++] = 47.0; kernel[k++] = 0.0;
        kernel[k++] = 0.0; kernel[k++] = -162.0; kernel[k++] = 0.0; kernel[k++] = 162.0; kernel[k++] = 0.0;
        kernel[k++] = 0.0; kernel[k++] = -47.0; kernel[k++] = 0.0; kernel[k++] = 47.0; kernel[k++] = 0.0;
        kernel[k++] = 0.0; kernel[k++] =  0.0; kernel[k++] = 0.0; kernel[k++] =  0.0; kernel[k++] = 0.0;
    };
    if (kernel_size == 5) {
        int k = 0;
        divider = 2*(1.0+2.0+4.0+8.0) + (6.0+12.0);
        kernel[k++] =  -2; kernel[k++] = -1;kernel[k++] = 0; kernel[k++] = 1; kernel[k++] =  2;
        kernel[k++] =  -8; kernel[k++] = -4;kernel[k++] = 0; kernel[k++] = 4; kernel[k++] =  8;
        kernel[k++] = -12; kernel[k++] = -6;kernel[k++] = 0; kernel[k++] = 6; kernel[k++] = 12;
        kernel[k++] =  -8; kernel[k++] = -4;kernel[k++] = 0; kernel[k++] = 4; kernel[k++] =  8;
        kernel[k++] =  -2; kernel[k++] = -1;kernel[k++] = 0; kernel[k++] = 1; kernel[k++] =  2;
    };

    float4 px_center = image.Sample(textureSampler, uv);

    px_center.rgb *= px_center.a;
    float3 resultX = float3(0.0,0.0,0.0);
    float3 resultY = float3(0.0,0.0,0.0);

    bool is_yuv = (filter_mode == FILTER_MODE_LUMINANCE || filter_mode == FILTER_MODE_LUMINANCE_ONLY || filter_mode == FILTER_MODE_YUV);

    // Horizontal and Vertical Sobel combined
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            float weightX = kernel[(x + 2)*5 + (y + 2)];
            float weightY = kernel[(y + 2)*5 + (x + 2)];
            float4 pixelWeighted = image.Sample(textureSampler, uv + float2(x * upixel, y * vpixel));
            pixelWeighted.rgb *= pixelWeighted.a;
            if (is_yuv) {
                pixelWeighted.rgb = rgb2yuvNormalized(pixelWeighted.rgb);
            }
            resultX += weightX * pixelWeighted.rgb;
            resultY += weightY * pixelWeighted.rgb;
        }
    }

    resultX /= divider;
    resultY /= divider;
    resultX[0] = abs(resultX[0]);
    resultY[0] = abs(resultY[0]);
    if (!is_yuv) {
        resultX[1] = abs(resultX[1]);
        resultX[2] = abs(resultX[2]);
        resultY[1] = abs(resultY[1]);
        resultY[2] = abs(resultY[2]);
    }

    // End result
    float3 result;
    if (is_yuv) {
        result[0] = sqrt(resultX[0]*resultX[0] + resultY[0]*resultY[0]);
        result[1] = resultX[1] + resultY[1];
        result[2] = resultX[2] + resultY[2];
    }
    else {
        result = sqrt(resultX*resultX + resultY*resultY);
        result = clamp(result, 0.0, 1.0);
    }

    if (filter_mode == FILTER_MODE_RGB) {
        result = smoothstep(0.0, 1.0-lightness, result);
    }
    if (filter_mode == FILTER_MODE_LUMINANCE) {
        float3 center_yuv = rgb2yuvNormalized(px_center.rgb);
        result[0] = center_yuv[0] + smoothstep(0.0, 1.0-lightness, result[0]);
        result[0] = clamp(result[0], 0.0, 1.0);
        result[1] = center_yuv[1];
        result[2] = center_yuv[2];
        result = yuvNormalized2rgb(result);
    }
    if (filter_mode == FILTER_MODE_LUMINANCE_ONLY) {
        result[0] = smoothstep(0.0, 1.0-lightness, result[0]);
        result = float3(result[0], result[0], result[0]);
    }
    if (filter_mode == FILTER_MODE_YUV) {
        result = yuvNormalized2rgb(result);
        result = smoothstep(0.0, 1.0-lightness, result);
    }

    px_center.rgb = lerp(result.rgb, px_center.rgb, orig_ratio)/px_center.a;
    return px_center;
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
