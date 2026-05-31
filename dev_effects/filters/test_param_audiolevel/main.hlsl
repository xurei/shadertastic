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
uniform float audio_level;
uniform float audio_threshold;
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

#define font_width 3
#define font_height 5
// Inspired from https://www.shadertoy.com/view/3lGBDm ; Licensed under CC BY-NC-SA 3.0
float printValue__digitBin( int x ) {
    return (
        /*x==0?480599.0:
        x==1?139810.0:
        x==2?476951.0:
        x==3?476999.0:
        x==4?350020.0:
        x==5?464711.0:
        x==6?464727.0:
        x==7?476228.0:
        x==8?481111.0:
        x==9?481095.0:*/
        x==0?31599.0:
        x==1?9362.0:
        x==2?31183.0:
        x==3?31207.0:
        x==4?23524.0:
        x==5?29671.0:
        x==6?29679.0:
        x==7?31012.0:
        x==8?31727.0:
        x==9?31719.0:
        476674.0 // Question mark
    );

    // These magic numbers are in binary the ON/OFF state of a 5x4 grid. Example with 0:
    // 0: 480599 = 0111 0101 0101 0101 0111
    // -> 0111
    //    0101
    //    0101
    //    0101
    //    0111
}

/**
 * Print a numerical value in the rectangle with it top-right at the coordinates uv
 * @param uv coordinates of the top right pixel of the debug area, in UV coordinates
 * @param value numerical value to print
 * @param nbDecimal number of decimals to print
 * @param fontSize size of the font, 1.0 meaning the full height of the image texture
 * @return true if the pixel located at uv is a debug pixel, false otherwise
 * @example
 * // float2(1.0, 0.0) is top right uv coordinate of the image
 * if (printValue(uv, 42.23, float2(1.0, 0.0), 3, 0.2)) {
 *     // current pixel is a debug one, print it as full red
 *     return float4(1.0, 0.0, 0.0, 1.0);
 * }
 * else {
 *     // actual shader code
 * }
 */
bool printValue(float2 uv, float value_to_debug, float2 area_topRight, int nbDecimal, float fontSize) {
    nbDecimal = max(0, nbDecimal);
    if ((uv.y < 0.0) || (uv.y >= 1.0) || (uv.x < 0.0) || (uv.x >= 1.0)) {
        return false;
    }

    bool isNegative = (value_to_debug < 0.0);
    bool hasDecimals = (nbDecimal > 0);
    value_to_debug = abs(value_to_debug);
    float log10Value = log2(abs(value_to_debug)) / log2(10.0);
    float biggestIndex = max(0.0, floor(max(log10Value, 0.0)));

    float square_height = fontSize / (font_height+1);
    float square_width = square_height * upixel/vpixel;

    float area_height = (font_height+1) * square_height;
    float area_width = (font_width+1) * square_width * (nbDecimal + (hasDecimals ? 1 : 0) + (1 + biggestIndex) + (isNegative ? 1 : 0));
    float2 area_bottomLeft = area_topRight + float2(-area_width, area_height);

    if (inside_box(uv, area_topRight, area_bottomLeft)) {
        uv -= float2(area_bottomLeft.x, area_topRight.y);
        int square_u = int(uv.x / square_width);
        int square_v = int(uv.y / square_height);

        int digitIndex = square_u / (font_width+1);
        int digit_u = square_u - digitIndex * (font_width+1);

        float digitBin = 0.0;
        if (isNegative) {
            digitIndex--;
        }
        if (isNegative && digitIndex == -1) {
            digitBin = 448.0; // Minus character
        }
        else if (hasDecimals && digitIndex == biggestIndex + 1) {
            digitBin = 2.0; // Dot character
        }
        else {
            if (hasDecimals && digitIndex > biggestIndex) {
                digitIndex--;
                value_to_debug = fract( value_to_debug );
            }
            float currentDigitFloat = mod(value_to_debug * pow(10.0, -(biggestIndex-digitIndex)), 10.0);
            int currentDigit;
            if ( (digitIndex == biggestIndex + nbDecimal) && (currentDigitFloat < 9.0) ) {
                currentDigit = int(round(currentDigitFloat));
            }
            else {
                currentDigit = int(currentDigitFloat);
            }
            digitBin = printValue__digitBin(currentDigit);
        }

        if (digit_u >= font_width) {
            return false;
        }

        return fmod(digitBin / pow(2.0, ((font_width+1)-square_v) * (font_width) + digit_u), 2.0) >= 1.0;
    }

    return false;
}

//Here goes your implementation !

float4 EffectLinear(float2 uv)
{
    if (printValue(uv, audio_level, float2(0.99, 0.01), 3, 0.2)) {
        return float4(0.0, 1.0, 0.0, 1.0);
    }
    if (uv[0] > 0.99) {
        float4 px = float4(0.0, 0.0, 0.0, 1.0);
        float lvl = 100.0 + audio_level - (100.0 + audio_threshold);
        if (lvl < 0.0) {
            lvl = 0.0;
        }
        lvl = 1.0 - lvl / (-1.0 * audio_threshold);
        if (lvl < uv[1]) {
            px = float4(1.0, 1.0, 1.0, 1.0);
        }
        return px;
    }
    else {
        return image.Sample(textureSampler, uv);
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
