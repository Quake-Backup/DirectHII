

struct VS_MESH {
	uint4 PrevTriVertx: PREVTRIVERTX;
	uint4 CurrTriVertx: CURRTRIVERTX;
	float2 TexCoord: TEXCOORD;
};

struct PS_MESH {
	float4 Position: SV_POSITION;
	float2 TexCoord: TEXCOORD;
	float3 Normal : NORMAL;
};


#ifdef VERTEXSHADER
float4 MeshLerpPosition (VS_MESH vs_in)
{
	return float4 (lerp (vs_in.PrevTriVertx, vs_in.CurrTriVertx, LerpBlend).xyz * MeshScale + MeshTranslate, 1.0f);
}

float3 MeshLerpNormal (VS_MESH vs_in)
{
	float3 n1 = LightNormals.Load (vs_in.PrevTriVertx.w).xyz;
	float3 n2 = LightNormals.Load (vs_in.CurrTriVertx.w).xyz;
	return normalize (lerp (n1, n2, LerpBlend));
}

PS_MESH MeshLightmapVS (VS_MESH vs_in)
{
	PS_MESH vs_out;

	vs_out.Position = mul (LocalMatrix, MeshLerpPosition (vs_in));
	vs_out.TexCoord = vs_in.TexCoord;
	vs_out.Normal = MeshLerpNormal (vs_in);

	return vs_out;
}

PS_DYNAMICLIGHT MeshDynamicVS (VS_MESH vs_in)
{
	return GenericDynamicVS (MeshLerpPosition (vs_in), MeshLerpNormal (vs_in), vs_in.TexCoord);
}
#endif


#ifdef PIXELSHADER
float4 MeshLightmapPS (PS_MESH ps_in) : SV_TARGET0
{
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));
	float shadedot = dot (normalize (ps_in.Normal), ShadeVector);
	float3 lmap = ShadeLight * max (shadedot + 1.0f, (shadedot * 0.2954545f) + 1.0f);

	// diff.a * AlphaVal because otherwise special trans types don't work
	return float4 (diff.rgb * lmap, diff.a * AlphaVal);
}
#endif

