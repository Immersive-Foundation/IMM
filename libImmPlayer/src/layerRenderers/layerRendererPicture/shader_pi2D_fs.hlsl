#define CUSTOM_ALPHA_TO_COVERAGE 1
#define MSAASampleCount 8

cbuffer FrameState : register(b0)
{
	struct
	{
		float       mTime;
		int         mFrame;
		int         dummy1;
		int         dummy2;
	}frame;
};

cbuffer LayersState : register(b3)
{
	struct
	{
		float4x4 mLayerToViewer;
		float  mLayerToViewerScale;
		float  mOpacity;
		float  mkUnused;
		float  mDrawInTime;
		float4   mDrawInParams; // draw-in and other effect parameters
		float4   mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
		uint   mID;
	}layer;
}


Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

#define k_pi 3.1415927



float4 main(
#if STEREOMODE==2	
	float clip : SV_ClipDistance0,
#endif
	float2 oUV : V2P_UV,
#if FORMAT_IS_STEREO==1
	float4 scale_offset : V2P_SCO,
#endif
#if CUSTOM_ALPHA_TO_COVERAGE==1
	float4 fragCoord : SV_POSITION,
	out uint coverage : SV_COVERAGE
#else
	float4 fragCoord : SV_POSITION
#endif
) : SV_TARGET
{

	float2 uv = oUV;

	#if FORMAT_IS_STEREO==1
	uv = clamp(uv * in_data.scale_offset.xy, float2(0.0,0.0), float2(1.0,0.5)) + in_data.scale_offset.zw;
	#endif

	float4 color = Texture.Sample(Sampler, uv);
    //float4 color = Texture.SampleLevel(Sampler, uv, 7.0);

#if COLOR_SPACE==0  // linear. Texture colors are in sRGB. Cheap convert to lienar here
	color.xyz = pow(abs(color.xyz), float3(2.2, 2.2, 2.2));
#endif
#if COLOR_SPACE==1 // gamma. Colors are already in gamma space, nothing to do here
#endif

	color.w *= layer.mOpacity;

    // alpha to coverage
	float2 q = fragCoord.xy + float2(0, float((frame.mFrame) & 7)*11.0);
	float ran = frac(52.9829189*frac(dot(q, float2(0.06711056, 0.00583715))));
	float al = clamp(color.a + 0.99*(ran - 0.5) / float(MSAASampleCount), 0.0, 1.0); // 0.99 is to make sure the dithering never makes the alpha leak to the previour or the next bucket
#if CUSTOM_ALPHA_TO_COVERAGE==1
	uint mask = (0xff00 >> uint(al*float(MSAASampleCount) + 0.5)) & 0xff;  // compute sample mask from input alpha, like hardware is doing it
	uint shift = (uint(ran*7.0) + layer.mID) & 7;                        // randomize mask per primitive ID and pixel
	coverage = (((mask << 8) | mask) >> shift) & 0xff;                     // barrel Shift 8 Bit
	al = 1.0;
#endif
    
	return float4(color.xyz, al);
}
