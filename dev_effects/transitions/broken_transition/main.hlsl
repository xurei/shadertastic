// Custom arguments, referenced in the meta.json file of the effet
uniform int glitchWidth;

//----------------------------------------------------------------------------------------------------------------------

#define sampleTex(is_a, uv) ((is_a) ? tex_a.Sample(textureSampler, uv) : tex_b.Sample(textureSampler, uv))
//----------------------------------------------------------------------------------------------------------------------

sampler_state textureSampler {
	Filter    = Linear;
	AddressU  = Wrap;
	AddressV  = Wrap;
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

float rand2(float2 co){
	float v = sin(dot(co, float2(12.9898, 78.233))) * 43758.5453;
	return fract(v);
}
float rand(float a, float b) {
	return rand2(float2(a, b));
}

float sigmoid(float strength, float n) {
	// Vanilla sigmoid ; this is not symetric between 0 and 1
	//return (1 / (1 + exp(-n)));

	n = strength * (n*2 - 1);
	float v0 = (1 / (1 + exp(strength)));
    float v1 = 1 - v0; // simplified from v1 = (1 / (1 + exp(-1 * strength)));

	return (1 / (1 + exp(-n)) - v0) / (v1-v0);
}

float sinplus1(float v) {
	return (1.0 + sin(v)) / 2.0;
}

float cosplus1(float v) {
	return (1.0 + cos(v)) / 2.0;
}

float sinwaves(float val) {
	return (
		0.2*sinplus1(17.0*val) +
		0.1*cosplus1(23.0*val) +
		0.2*sinplus1(47.0*val) +
		0.15*sinplus1(53.0*val) +
		0.15*cosplus1(97.0*val) +
		0.11*sinplus1(197.0*val)
	) / (0.2+0.1+0.2+0.15+0.15+0.11);
}

float getY(float4 val) {
	//Return Y value in the colorspace YUV (without the constant at the end of the calc)
	return (0.257*val.r + 0.504*val.g + 0.098*val.b) / 0.859;
}

float4 getMaxPixelBlend(FragData f_in, int nbToCheck) {
	float4 result = lerp(
		tex_a.Sample(textureSampler, f_in.uv),
		tex_b.Sample(textureSampler, f_in.uv),
		time
	);
	float maxY = getY(result);

	#ifdef _D3D11
	[loop]
	#endif
	for (int i=1; i<nbToCheck; ++i) {
		float2 pos = float2(f_in.uv[0] + i*upixel, f_in.uv[1]);
		float4 val = lerp(
			tex_a.Sample(textureSampler, pos),
			tex_b.Sample(textureSampler, pos),
			time
		);

		float Y = getY(val);
		if (Y > maxY) {
			maxY = Y;
			result = val;
		}

		pos = float2(f_in.uv[0] - i*upixel, f_in.uv[1]);
		val = lerp(
			tex_a.Sample(textureSampler, pos),
			tex_b.Sample(textureSampler, pos),
			time
		);

		Y = getY(val);
		if (Y > maxY) {
			maxY = Y;
			result = val;
		}
	}
	return result;
}

float4 getAvgPixelBlend(FragData f_in, int nbToCheck) {
	float4 result = lerp(
		tex_a.Sample(textureSampler, f_in.uv),
		tex_b.Sample(textureSampler, f_in.uv),
		time
	);

	#ifdef _D3D11
	[loop]
	#endif
	for (int i=1; i<nbToCheck; ++i) {
		float2 pos = float2(f_in.uv[0] + i*upixel, f_in.uv[1]);
		float4 val = lerp(
			tex_a.Sample(textureSampler, pos),
			tex_b.Sample(textureSampler, pos),
			time
		);

		result = result + val;

		pos = float2(f_in.uv[0] - i*upixel, f_in.uv[1]);
		val = lerp(
			tex_a.Sample(textureSampler, pos),
			tex_b.Sample(textureSampler, pos),
			time
		);

		result = result + val;
	}

	result /= (2*nbToCheck+1);
	return result;
}

float distFromCenter(float2 uv) {
    float uu = uv[0]-0.5;
    uu *= uu;
    float vv = uv[1]-0.5;
    vv *= vv;
    return sqrt(uu+vv);
}

//#include "devtools.hlsl"
//#include "color_conversion.hlsl"
//#include "blend_functions.hlsl"
//#include "getmaxpixel_functions.hlsl"
//#include "white_noise.hlsl"
//#include "getmax.hlsl"
//#include "burn.hlsl"
//#include "pixelate.hlsl"
//#include "sobel.hlsl"
//#include "waterfall.hlsl"
//#include "glitch.hlsl"
//#include "lab.hlsl"

float4 EffectLinear(FragData f_in)
{
	float t = sigmoid(4, time);
	float4 a_val = tex_a.Sample(textureSampler, f_in.uv);
	float4 b_val = tex_b.Sample(textureSampler, f_in.uv);
	float4 rgba = lerp(a_val, b_val, t);
	return rgba;
}

float4 PSEffect(FragData f_in) : TARGET
{
    float4 rgba = EffectLinear(f_in);
	rgba.rgb = srgb_nonlinear_to_linear(rgba.rgb);
	return rgba;
}

float4 PSEffectLinear(FragData f_in) : TARGET
{
	float4 rgba = EffectLinear(f_in);
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
