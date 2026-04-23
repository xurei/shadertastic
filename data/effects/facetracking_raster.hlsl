uniform float4x4 ViewProj;
//uniform texture2d image;

struct VertInOut {
	float4 pos : POSITION;
    float4 bary_id   : TEXCOORD0;
    //float2 bary2_id : TEXCOORD1;
};

VertInOut VSDefault(VertInOut v_in)
{
	VertInOut vert_out;
    vert_out.pos = float4(v_in.pos.xyz, 1.0);
    vert_out.bary_id = v_in.bary_id;
	return vert_out;
}

float4 ps_main(VertInOut v_in) : TARGET
{
    float3 bary = float3(
        v_in.bary_id.x,
        v_in.bary_id.y,
        v_in.bary_id.z
    );

    float tri_id = v_in.bary_id.w;

    return float4(
        tri_id,
        bary.x,
        bary.y,
        1.0
    );
}

technique Draw
{
    pass
    {
        vertex_shader = VSDefault(v_in);
        pixel_shader  = ps_main(v_in);
    }
}