
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// CBUFFERS
// cBuffers are available to all shader stages and are divided by update frequency and usage

cbuffer cbDrawPerFrame : register(b0) {
	matrix orthoMatrix : packoffset(c0);
	float v_gamma : packoffset(c4.x);
	float v_contrast : packoffset(c4.y);
};

cbuffer cbMainPerFrame : register(b1) {
	matrix mvpMatrix : packoffset(c0);
	float3 viewOrigin : packoffset(c4);
	float3 viewForward : packoffset(c5);
	float3 viewRight : packoffset(c6);
	float3 viewUp : packoffset(c7);
	float2 skyScroll : packoffset(c8.x);
	float skyScale : packoffset(c8.z);
	float turbTime : packoffset(c8.w);
	float4 vBlend : packoffset(c9);
	float2 WarpTime : packoffset(c10.x);
};

cbuffer cbPerObject : register(b2) {
	matrix LocalMatrix : packoffset(c0);
	float AlphaVal : packoffset(c4.x);
	float AbsLight : packoffset(c4.y);
	float3 EntOrigin : packoffset(c5);
};

cbuffer cbPerMesh : register(b3) {
	float3 MeshScale : packoffset(c0);
	float3 MeshTranslate : packoffset(c1.x);
	float LerpBlend : packoffset(c1.w);
	float3 ShadeLight : packoffset(c2);
	float3 ShadeVector : packoffset(c3);
};

cbuffer cbPerLight : register(b4) {
	float3 LightOrigin : packoffset(c0.x);
	float LightRadius : packoffset(c0.w);
	float3 LightColour : packoffset(c1.x);
	float MinLight : packoffset(c1.w);
};


// common to mesh and surf
struct PS_DYNAMICLIGHT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 LightVector : LIGHTVECTOR;
	float3 Normal : NORMAL;
};


static const float M_PI = 3.14159265f;


#ifdef VERTEXSHADER
struct VS_QUADBATCH {
	float4 Position : POSITION;
	float4 Color : COLOUR;
	float2 TexCoord : TEXCOORD;
};

Buffer<float> LightStyles : register(t0);
Buffer<float4> LightNormals : register(t1);
#endif


// this ensures that we use the same dynamic calcs on surfs and meshs
// outside of the #ifdef guard because it's used by the VS for meshs and the GS for surfs
PS_DYNAMICLIGHT GenericDynamicVS (float4 Position, float3 Normal, float2 TexCoord)
{
	PS_DYNAMICLIGHT vs_out;

	vs_out.Position = mul (LocalMatrix, Position);
	vs_out.TexCoord = TexCoord;
	vs_out.LightVector = LightOrigin - Position.xyz;
	vs_out.Normal = Normal;

	return vs_out;
}


#ifdef PIXELSHADER
// sampler slots
sampler mainSampler : register(s0);		// main sampler used for most objects, wrap mode, obeys gl_texturemode and gl_texture_anisotropy
sampler lmapSampler : register(s1);		// lightmap sampler, always linear, clamp mode, no mips, no anisotropy
sampler warpSampler : register(s2);		// underwater and other warps, always linear, wrap mode, no mips, no anisotropy
sampler drawSampler : register(s3);		// used for the 2d render; point sampled, wrap mode, no mips, no anisotropy

// texture slots
Texture2D<float4> mainTexture : register(t0);	// main diffuse texture on most objects
Texture2DArray<float4> lmapTexture : register(t1);	// lightmap styles 0/1/2/3
Texture2D<float4> solidSkyTexture : register(t2);	// solid sky layer
Texture2D<float4> alphaSkyTexture : register(t3);	// alpha sky layer
Texture2D<float4> warpTexture : register(t4);	// underwater warp noise texture


// faster than a full-screen gamma pass over the scene as a post-process, and more flexible than texture gamma
float4 GetGamma (float4 colorin)
{
	// gamma is not applied to alpha
	float3 contrasted = colorin.rgb * v_contrast;	// this isn't actually "contrast" but it's consistent with what other engines do
	return float4 (pow (max (contrasted, 0.0f), v_gamma), colorin.a);
}

// common to mesh and surf
float4 GenericDynamicPS (PS_DYNAMICLIGHT ps_in) : SV_TARGET0
{
	// https://github.com/id-Software/Quake-Tools/blob/c0d1b91c74eb654365ac7755bc837e497caaca73/qutils/LIGHT/LTFACE.C#L353
	float dist = length (ps_in.LightVector);
	clip (LightRadius - dist);

	// reading the diffuse texture early so that it should interleave with some ALU ops
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));

	/*
	https://github.com/id-Software/Quake-Tools/blob/c0d1b91c74eb654365ac7755bc837e497caaca73/qutils/LIGHT/LTFACE.C#L403
	https://github.com/id-Software/Quake-Tools/blob/c0d1b91c74eb654365ac7755bc837e497caaca73/qutils/LIGHT/LTFACE.C#L410
	https://github.com/id-Software/Quake-Tools/blob/c0d1b91c74eb654365ac7755bc837e497caaca73/qutils/LIGHT/LIGHT.C#L13
	nitpickers corner
	(hello tomaz! "What codebase is this based on? ... Thats NOT from the original quake code"): angle = (1.0-scalecos) + scalecos*angle;
	but scalecos is 0.5, so substituting: angle = (1.0-0.5) + 0.5*angle;
	and evaluate (1.0-0.5): angle = 0.5 + 0.5*angle;
	or (because multiplication and addition are commutative but multiplication takes precedence): angle = angle * 0.5 + 0.5;
	*/
	float Angle = dot (normalize (ps_in.Normal), normalize (ps_in.LightVector)) * 0.5f + 0.5f;

	/*
	https://github.com/id-Software/Quake-Tools/blob/c0d1b91c74eb654365ac7755bc837e497caaca73/qutils/LIGHT/LTFACE.C#L411
	light.exe used a 0..255 scale whereas shaders use a 0..1 scale so bring it down; but using 256 which the hardware might optimize better, then doubled for overbright range
	*/
	float Add = ((LightRadius - dist) * 0.0078125f) * Angle;

	// for translucency - because this is done as an additive rather than alpha blend, we modulate the output
	// down by the alpha value instead of actually writing out alpha, and the end result comes out (mostly) OK.
	return float4 (diff.rgb * LightColour * Add * AlphaVal, 0.0f);
}
#endif

