uniform float4x4 ViewProj;
uniform texture2d image;

struct VertInOut {
	float4 pos : POSITION;
    float2 bary01   : TEXCOORD0;
    //float2 bary2_id : TEXCOORD1;
};

VertInOut VSDefault(VertInOut v_in)
{
	VertInOut vert_out;
    vert_out.pos = float4(v_in.pos.xyz, 1.0);
    vert_out.bary01 = v_in.bary01;
    //vert_out.bary2_id = v_in.bary2_id;
	//vert_out.uv  = v_in.uv;
	//vert_out.col = use_color ? v_in.col : float4(0.0, 0.0, 0.0, 1.0);
	return vert_out;
}

float4 ps_main(VertInOut v_in) : TARGET
{
    float3 bary = float3(
        v_in.bary01.x,
        v_in.bary01.y,
        0.0 //v_in.bary2_id.x
    );

    float tri_id = 0.5; //v_in.bary2_id.y;

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