#pragma pack_matrix(row_major)

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

cbuffer DisplayState : register(b4)
{
	struct
	{
		struct
		{
			//float4x4      mMatrix_Prj;
			//float4x4      mMatrix_Cam;
			float4x4      mMatrix_CamPrj;
			//float4x4      mInvMatrix_Prj;
			//float4x4      mInvMatrix_Cam;
			//float4x4      mInvMatrix_CamPrj;
		}mEye[2];
		float2        mResolution;
	}display;
}

cbuffer PassState : register(b5)
{
	struct
	{
		int mID;
		int kk1;
		int kk2;
		int kk3;
	}eyepass;
}


cbuffer PictureState : register(b9)
{
    struct
    {
        float2 unSize;
        float kk2;
        float kk3;
    }pictureState;
}

#if STEREOMODE==0
#define iid 0
#endif
#if STEREOMODE==1
// see Player::RenderStereoMultiPass::eyeID
//#define iid eyepass.mID
#define iid 0
#endif
#if STEREOMODE==2
#define iid instanceID
#endif


void main(float2 pos : POSITION,
#if STEREOMODE==2	
	uint instanceID : SV_InstanceID,
	out float  oClip : SV_ClipDistance0,
#endif
	out float2 oUV  : V2P_UV,
#if FORMAT_IS_STEREO==1
	out float4 scale_offset : V2P_SCO,
#endif
	out float4 oPosition : SV_Position)
{
    float3 opos = float3( pictureState.unSize*pos, 0.0);
	float3 cpos = (mul(layer.mLayerToViewer, float4(opos, 1.0))).xyz;

#if FORMAT_IS_STEREO==1
	scale_offset = vec4(1.0, 0.5, 0.0, 0.5*float(iid));
#endif

	oUV = 0.5 + 0.5*pos.xy*float2(1.0, -1.0);

	float4x4 mat = display.mEye[iid].mMatrix_CamPrj;

	oPosition = mul(mat, float4(cpos, 1.0));

#if STEREOMODE==2
	float ss = (instanceID == 0) ? -1.0 : 1.0;
	oPosition.x = 0.5*(oPosition.x + ss*oPosition.w);
	oClip = oPosition.x * ss;
#endif
}
