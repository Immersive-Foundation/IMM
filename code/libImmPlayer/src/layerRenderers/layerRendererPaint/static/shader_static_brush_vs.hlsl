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
        float4x4    mLayerToViewer; // layer to viewer
        float       mScale;
        float       mOpacity;
        float       mFlipSign;
        float       mDrawInTime;
        float4      mDrawInParams;
        float4      mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
        uint        mID;
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

struct vertex_format_t
{
    float   mPosX;
    float   mPosY;
    float   mPosZ;
    uint    mWid;
    uint    mColAlp;
    uint    mDirInf;
    uint    mAxUUn2;
    uint    mAxVUn3;
    float   mTim;
};

//struct chunk_data_t
//{
//	uint vertexOffset;
//};
StructuredBuffer<vertex_format_t> data : register(t8);
//StructuredBuffer<chunk_data_t> cdata : register(t9);

cbuffer ChunkData : register(b9)
{
    struct
    {
        uint vertexOffset;
		float mBiggestStroke;
    }chunk_data[128];
}

float4 unpack4(uint d)
{
    return float4(uint4(d, d >> 8u, d >> 16u, d >> 24u) & 255u) / 255.0;
}

float3 unpack3(uint d)
{
    return float3(uint3(d, d >> 8u, d >> 16u) & 255u) / 255.0;
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


float3 decodeUnitVector(uint data)
{
    uint2 d = uint2(data & 65535u, data >> 16u);
    float2 v = float2(d) / 32767.5 - 1.0;
    float3 nor = float3(v, 1.0 - abs(v.x) - abs(v.y)); // Rune Stubbe's version,
    float t = max(-nor.z, 0.0);                     // much faster than original
    nor.x += (nor.x>0.0) ? -t : t;                     // implementation of this
    nor.y += (nor.y>0.0) ? -t : t;                     // technique
    return normalize(nor);
}


void main(uint vertexID : SV_VertexID,
          #if STEREOMODE==2	
          uint instanceID : SV_InstanceID,
          out float  oClip : SV_ClipDistance0,
          #endif
          out float4 oColor    : V2P_COLOR,
          out float4 oPosition : SV_Position, 
          out uint   oInfo     : V2P_INFO )
{

    uint real_vertexID = vertexID + chunk_data[0].vertexOffset;

#if BRUSHTYPE==0
    uint bid = uint(vertexID);
#endif
#if BRUSHTYPE==1
    uint bid = uint(real_vertexID) >> 1u;
    uint vid = uint(real_vertexID) & 1u;
#endif
#if BRUSHTYPE==2 || BRUSHTYPE==3
    uint bid = uint(real_vertexID) / 7u;
    uint vid = uint(real_vertexID) % 7u;
#endif
#if BRUSHTYPE==4
    uint bid = uint(real_vertexID) >> 2u;
    uint vid = uint(real_vertexID) & 3u;
#endif


    vertex_format_t vertex = data[bid];


    float3 inVertex = float3(vertex.mPosX, vertex.mPosY, vertex.mPosZ);
    uint   inInfo = vertex.mDirInf >> 24u;
    float  inTime = vertex.mTim;
    float4 inColAlpha = unpack4(vertex.mColAlp);
    float3 inOri = normalize(-1.0 + 2.0*unpack3(vertex.mDirInf));
    float3 inAxU = decodeUnitVector(vertex.mAxUUn2);
    float3 inAxV = decodeUnitVector(vertex.mAxVUn3);

    float inWid = 1.7*chunk_data[0].mBiggestStroke * float(vertex.mWid&0x7fff)/32767.0;


    float3 pos = inVertex;
    float4 color = inColAlpha;




    // wiggle effect
    #if WIGGLE==1  
    {
        pos += layer.mKeepAlive[0].z*sin(layer.mKeepAlive[0].x*pos.yzx + layer.mKeepAlive[0].y*frame.mTime);
    }
    #endif


    float3 cpos = (mul(layer.mLayerToViewer, float4(pos, 1.0))).xyz;

    oInfo = (inColAlpha.w>0.996) ? layer.mID : inInfo;

    // directional stroke
    float f = 1.0;
    if (((inInfo >> 7) & 1u) == 0u)
    {
        float3 wori = normalize(mul(layer.mLayerToViewer,float4(inOri, 0.0)).xyz);
        //f = clamp( -wori.z, 0.0, 1.0 );
        f = clamp(dot(wori, normalize(cpos)), 0.0, 1.0);
        f = f*f;
    }

    #if COLOR_COMPRESSED==0 // linear. Colors are linear. But they are encoded in a sqrt() curve for more precission. Here we undo it
    oColor = float4(color.xyz*color.xyz, color.w*f*layer.mOpacity);
    #else //  COLOR_COMPRESSED==1 // gamma. Colors are already in gamma comnverted in the CPU before upload to the GPU. Nothing to do
    oColor = float4(color.xyz, color.w*f*layer.mOpacity);
    #endif

    #if DRAWIN==1 
    float drawingT = 2.0*layer.mDrawInTime-inTime;						
    oColor.w *= smoothstep(layer.mDrawInParams.z, layer.mDrawInParams.z + layer.mDrawInParams.w, drawingT);
    #endif

    //==================================================
    float3  bPos = cpos;
    float3  bV = normalize((mul(layer.mLayerToViewer,float4(inAxV, 0.0))).xyz);
    float3  bU = normalize((mul(layer.mLayerToViewer,float4(inAxU, 0.0))).xyz);

    //-------- line brush -------------------------

#if BRUSHTYPE==0
    float3 bWPos = bPos;
#endif

    //-------- ribbon brush -------------------------

#if BRUSHTYPE==1
    float u = float(vid) / 1.0;

    float wb = (-1.0 + 2.0*u) * inWid * layer.mScale;
    float3 bWPos = bPos - wb*bU;
#endif

    //-------- sphere and thick ribbon brush -------------------------

#if BRUSHTYPE==2 || BRUSHTYPE==3
    float u = float(vid) / 7.0;
    float a = u*6.283185;
    float2 sc = float2(cos(a), sin(a));
    #if BRUSHTYPE==3
    sc *= float2(1.0, 0.3);
    #endif
    float wb = inWid * layer.mScale;

    float3 bWPos = bPos + wb*(bU*sc.x + bV*sc.y);
#endif

    //-------- cube brush -------------------------

#if BRUSHTYPE==4
    float u = float(vid) / 4.0;

    float wb = inWid * layer.mScale;

    // this can be done based on vid with ints, no need to go float
    float2 sc = float2(sign(0.2 - abs(u - 0.65)), sign(abs(u - 0.35) - 0.3));   // generate the vertices of a square, for u = {0.0, 0.25, 0.5, 0.75, 1.0=0.0}

    float3 bWPos = bPos + wb*(bU*sc.x + bV*sc.y);
#endif

    float4x4 mat = display.mEye[iid].mMatrix_CamPrj;

    // GL to DX conversion. If we remove this, then enable dx2gl() in player.cpp::iDisplayPreRenderLayer::289 and also enable GL.GetGPUProjectionMatrix() in C#
    //mat[1][1] = -mat[1][1];
    //mat[2][2] = -0.5*(1.0+mat[2][2]);
    //mat[2][3] = -0.5*mat[2][3];

    oPosition = mul(mat, float4(bWPos, 1.0));


    #if STEREOMODE==2
    float ss = (instanceID == 0) ? -1.0 : 1.0;
    oPosition.x = 0.5*(oPosition.x + ss*oPosition.w);
    oClip = oPosition.x * ss;
    #endif
}
