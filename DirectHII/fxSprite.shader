

struct PS_DRAWSPRITE {
	float4 Position : SV_POSITION;
	float4 Color : COLOUR;
	float2 TexCoord : TEXCOORD;
};


#ifdef VERTEXSHADER
PS_DRAWSPRITE SpriteVS (VS_QUADBATCH vs_in)
{
	PS_DRAWSPRITE vs_out;

	vs_out.Position = mul (mvpMatrix, vs_in.Position);
	vs_out.Color = vs_in.Color;
	vs_out.TexCoord = vs_in.TexCoord;

	return vs_out;
}
#endif


#ifdef PIXELSHADER
float4 SpritePS (PS_DRAWSPRITE ps_in) : SV_TARGET0
{
	// adjust for pre-multiplied alpha
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));
	return float4 (diff.rgb * diff.a, diff.a) * ps_in.Color;
}
#endif

