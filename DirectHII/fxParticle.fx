

struct PS_PARTICLE {
	float4 Position : SV_POSITION;
	float4 Color : COLOUR;
	float2 TexCoord : TEXCOORD;
};


#ifdef VERTEXSHADER
PS_PARTICLE GetParticleVert (VS_QUADBATCH vs_in, float HackUp, float TypeScale)
{
	PS_PARTICLE vs_out;

	// hack a scale up to keep particles from disapearing
	float2 ScaleUp = vs_in.TexCoord * (1.0f + dot (vs_in.Position.xyz - viewOrigin, viewForward) * HackUp);

	// compute new particle origin
	float3 Position = vs_in.Position.xyz + (viewRight * ScaleUp.x + viewUp * ScaleUp.y) * TypeScale;

	// and finally write it out
	vs_out.Position = mul (mvpMatrix, float4 (Position, 1.0f));
	vs_out.Color = vs_in.Color;
	vs_out.TexCoord = vs_in.TexCoord;

	return vs_out;
}

PS_PARTICLE ParticleCircleVS (VS_QUADBATCH vs_in)
{
	return GetParticleVert (vs_in, 0.002f, 0.666f);
}

PS_PARTICLE ParticleSquareVS (VS_QUADBATCH vs_in)
{
	return GetParticleVert (vs_in, 0.002f, 0.5f);
}
#endif


#ifdef PIXELSHADER
float4 ParticleCirclePS (PS_PARTICLE ps_in) : SV_TARGET0
{
	// procedurally generate the particle dot for good speed and per-pixel accuracy at any scale
	return GetGamma (float4 (ps_in.Color.rgb, saturate (ps_in.Color.a * (1.0f - dot (ps_in.TexCoord, ps_in.TexCoord)))));
}

float4 ParticleSquarePS (PS_PARTICLE ps_in) : SV_TARGET0
{
	// procedurally generate the particle dot for good speed and per-pixel accuracy at any scale
	return GetGamma (ps_in.Color);
}
#endif

