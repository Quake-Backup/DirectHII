

struct VS_PARTICLE {
	float4 Position : POSITION;
	float4 Color : COLOUR;
	float Size : SIZE;
	float2 Offsets : OFFSETS;
};


struct PS_PARTICLE {
	float4 Position : SV_POSITION;
	float4 Color : COLOUR;
};


#ifdef VERTEXSHADER
PS_PARTICLE ParticleVS (VS_PARTICLE vs_in)
{
	PS_PARTICLE vs_out;

	// hack a scale up to keep particles from disapearing
	float2 ScaleUp = vs_in.Offsets * (1.0f + dot (vs_in.Position.xyz - viewOrigin, viewForward) * 0.002f);

	// compute new particle origin
	float3 Position = vs_in.Position.xyz + (viewRight * ScaleUp.x + viewUp * ScaleUp.y) * vs_in.Size;

	// and finally write it out
	vs_out.Position = mul (mvpMatrix, float4 (Position, 1.0f));
	vs_out.Color = vs_in.Color;

	return vs_out;
}
#endif


#ifdef PIXELSHADER
float4 ParticlePS (PS_PARTICLE ps_in) : SV_TARGET0
{
	// square particles to match with the crunchy pixel look
	return GetGamma (ps_in.Color);
}
#endif

