uniform int flip_type;

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

float4 EffectLinear(float2 uv)
{
    float4 px = image.Sample(textureSampler, uv);
    switch(flip_type) {
        case 0: /*R → G → B → R*/ px.rgb = float3(px[2], px[0], px[1]); break;
        case 1: /*R ← G ← B ← R*/ px.rgb = float3(px[1], px[2], px[0]); break;
        case 2: /*R ↔ G*/ px.rgb = float3(px[1], px[0], px[2]); break;
        case 3: /*R ↔ B*/ px.rgb = float3(px[2], px[1], px[0]); break;
        case 4: /*G ↔ B*/ px.rgb = float3(px[0], px[2], px[1]); break;
    }
    return px;
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
