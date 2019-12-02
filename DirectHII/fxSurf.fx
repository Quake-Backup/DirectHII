
struct VS_SURFCOMMON {
	float4 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float2 Lightmap : LIGHTMAP;
	float MapNum : MAPNUM;
	int4 Styles: STYLES;
};

struct PS_LIGHTMAPPED {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Lightmap : LIGHTMAP;
	nointerpolation float4 Styles: STYLES;
};

struct PS_ABSLIGHT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

struct PS_DRAWTURB {
	float4 Position : SV_POSITION;
	float2 TexCoord0 : TEXCOORD0;
	float2 TexCoord1 : TEXCOORD1;
};

struct PS_DRAWSKY {
	float4 Position : SV_POSITION;
	float3 TexCoord : TEXCOORD;
};


#ifdef VERTEXSHADER
PS_LIGHTMAPPED SurfLightmapVS (VS_SURFCOMMON vs_in)
{
	PS_LIGHTMAPPED vs_out;

	vs_out.Position = mul (LocalMatrix, vs_in.Position);
	vs_out.TexCoord = vs_in.TexCoord;
	vs_out.Lightmap = float3 (vs_in.Lightmap, vs_in.MapNum);

	vs_out.Styles = float4 (
		LightStyles.Load (vs_in.Styles.x),
		LightStyles.Load (vs_in.Styles.y),
		LightStyles.Load (vs_in.Styles.z),
		LightStyles.Load (vs_in.Styles.w)
	);

	return vs_out;
}

PS_ABSLIGHT SurfAbsLightVS (VS_SURFCOMMON vs_in)
{
	PS_ABSLIGHT vs_out;

	vs_out.Position = mul (LocalMatrix, vs_in.Position);
	vs_out.TexCoord = vs_in.TexCoord;

	return vs_out;
}

VS_SURFCOMMON SurfDynamicVS (VS_SURFCOMMON vs_in)
{
	return vs_in;
}

PS_DRAWTURB SurfDrawTurbVS (VS_SURFCOMMON vs_in)
{
	PS_DRAWTURB vs_out;

	vs_out.Position = mul (LocalMatrix, vs_in.Position);
	vs_out.TexCoord0 = vs_in.TexCoord;
	vs_out.TexCoord1 = vs_in.Lightmap + turbTime;

	return vs_out;
}

PS_DRAWSKY SurfDrawSkyVS (VS_SURFCOMMON vs_in)
{
	PS_DRAWSKY vs_out;

	vs_out.Position = mul (LocalMatrix, vs_in.Position);
	vs_out.TexCoord = ((vs_in.Position.xyz + EntOrigin) - viewOrigin) * float3 (1.0f, 1.0f, 3.0f);

	return vs_out;
}
#endif


#ifdef GEOMETRYSHADER
[maxvertexcount (3)]
void SurfDynamicGS (triangle VS_SURFCOMMON gs_in[3], inout TriangleStream<PS_DYNAMICLIGHT> gs_out)
{
	// this is the same normal calculation as QBSP does
	float3 Normal = normalize (
		cross (gs_in[0].Position.xyz - gs_in[1].Position.xyz, gs_in[2].Position.xyz - gs_in[1].Position.xyz)
	);

	// output position needs to use the same transform as the prior pass to satisfy invariance rules
	// we inverse-transformed the light position by the entity local matrix so we don't need to transform the normal or the light vector stuff
	gs_out.Append (GenericDynamicVS (gs_in[0].Position, Normal, gs_in[0].TexCoord));
	gs_out.Append (GenericDynamicVS (gs_in[1].Position, Normal, gs_in[1].TexCoord));
	gs_out.Append (GenericDynamicVS (gs_in[2].Position, Normal, gs_in[2].TexCoord));
}
#endif


#ifdef PIXELSHADER
float4 SurfLightmapPS (PS_LIGHTMAPPED ps_in) : SV_TARGET0
{
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));
	float lmap = dot (lmapTexture.Sample (lmapSampler, ps_in.Lightmap), ps_in.Styles);

	return float4 (diff.rgb * lmap, diff.a * AlphaVal);
}

float4 SurfAbsLightPS (PS_ABSLIGHT ps_in) : SV_TARGET0
{
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));
	float lmap = AbsLight * 2.0f * AlphaVal; // double intensity to match the lightmapped surf

	return float4 (diff.rgb * lmap, diff.a * AlphaVal);
}

float4 SurfDrawTurbPS (PS_DRAWTURB ps_in) : SV_TARGET0
{
	float4 diff = GetGamma (mainTexture.SampleGrad (mainSampler, ps_in.TexCoord0 + sin (ps_in.TexCoord1) * 0.125f, ddx (ps_in.TexCoord0), ddy (ps_in.TexCoord0)));
	return float4 (diff.rgb, diff.a * AlphaVal);
}

float4 SurfDrawSkyPS (PS_DRAWSKY ps_in) : SV_TARGET0
{
	float3 skycoord = normalize (ps_in.TexCoord) * skyScale;

	float4 solidColor = GetGamma (solidSkyTexture.Sample (mainSampler, skycoord.xy + skyScroll.x));
	float4 alphaColor = GetGamma (alphaSkyTexture.Sample (mainSampler, skycoord.xy + skyScroll.y));

	return lerp (solidColor, alphaColor, alphaColor.a);
}
#endif

