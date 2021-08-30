static const char* shader_static_brush_vs = R"(
#extension GL_ARB_shader_draw_parameters : enable
#if STEREOMODE==2
#extension GL_ARB_shader_viewport_layer_array : enable
#endif
//#extension GL_ARB_enhanced_layouts : enable

layout (std140, row_major, binding=0) uniform FrameState
{
    float       mTime;
    int         mFrame;
}frame;

layout (std140, row_major, binding=3) uniform LayersState
{
    mat4x4 mLayerToViewer;
    float  mLayerToViewerScale;
    float  mOpacity;
    float  mFlipSign;
    float  mDrawInTime;
    vec4   mAnimParams;			 // draw-in and other effect parameters
    vec4   mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint   mID;
}layer;


layout (std140, row_major, binding=4) uniform DisplayState
{
    struct
    {
        //mat4x4      mMatrix_Prj;
        //mat4x4      mMatrix_Cam;
        mat4x4      mMatrix_CamPrj;
        //mat4x4      mInvMatrix_Prj;
        //mat4x4      mInvMatrix_Cam;
        //mat4x4      mInvMatrix_CamPrj;
    }mEye[2];
    vec2        mResolution;
}display;

layout (std140, row_major, binding=5) uniform PassState
{
    int mID;
    int kk1;
    int kk2;
    int kk3;
}pass;

#if VERTEX_FORMAT==1
struct vertex_format_t
{
    float   mPosX;
    float   mPosY;
    float   mPosZ;
    uint    mWidInfo;
    uint    mColAlp;
    uint    mAxUUn2;
    uint    mAxVUn3;
};

#else

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

#endif

layout(std430, binding = 8) readonly buffer VertexData
{
    vertex_format_t data[];
};

layout (std140, row_major, binding=9) uniform ChunkData
{
    struct
    {
        uint mVertexOffset;
		float mBiggestStroke;
    }mData[128];
}chunk_data;

out V2CData
{
    vec4  col_tra;
    flat uint mask;
}vg;

vec4 unpack4( uint d )
{
    return vec4( ivec4(d,d>>8u,d>>16u,d>>24u)&255u )/255.0;
}

vec3 unpack3( uint d )
{
    return vec3( ivec3(d,d>>8u,d>>16u)&255u )/255.0;
}

vec3 decodeUnitVector( uint data )
{
    uvec2 d = uvec2( data&65535u, data>>16u );
    vec2 v = vec2(d)/32767.5 - 1.0;
    vec3 nor = vec3(v, 1.0 - abs(v.x) - abs(v.y)); // Rune Stubbe's version,
    float t = max(-nor.z,0.0);                     // much faster than original
    nor.x += (nor.x>0.0)?-t:t;                     // implementation of this
    nor.y += (nor.y>0.0)?-t:t;                     // technique
    return normalize( nor );
}


void main()
{
    uint real_vertexID = gl_VertexID + chunk_data.mData[0].mVertexOffset;


#if BRUSHTYPE==0
    uint bid = uint(real_vertexID);
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
    vec3  inVertex   = vec3( vertex.mPosX, vertex.mPosY, vertex.mPosZ );
    #if VERTEX_FORMAT==1
	 float inWid      = 1.7*chunk_data.mData[0].mBiggestStroke * float(vertex.mWidInfo >> 8u)/32767.0;
     uint  inInfo     = uint(vertex.mWidInfo & 7u);
	#else
     uint  inInfo     = vertex.mDirInf>>24u;
     float inTime     = vertex.mTim;
     vec3  inOri      = normalize( -1.0+2.0*unpack3( vertex.mDirInf ) );
	 float inWid      = 1.7*chunk_data.mData[0].mBiggestStroke * float(vertex.mWid&0x7fff)/32767.0;
	#endif
    vec4  inColAlpha = unpack4( vertex.mColAlp );
    vec3  inAxU      = decodeUnitVector( vertex.mAxUUn2 );
    vec3  inAxV      = decodeUnitVector( vertex.mAxVUn3 );

    vec3 pos = inVertex;

    
#if STEREOMODE==0
    #define iid 0
#endif
#if STEREOMODE==1
    #define iid pass.mID
#endif
#if STEREOMODE==2    
    #define iid gl_InstanceID
    gl_ViewportIndex = gl_InstanceID;
#endif


    // wiggle effect
    #if WIGGLE==1  
    {
        pos += layer.mKeepAlive[0].z*sin( layer.mKeepAlive[0].x*inVertex.yzx + layer.mKeepAlive[0].y*frame.mTime);
    }
    #endif
    
    vec3 cpos = (layer.mLayerToViewer * vec4(pos, 1.0)).xyz;

 
    vg.mask = (inColAlpha.w>0.999) ? layer.mID : inInfo;

    // directional stroke
	#if VERTEX_FORMAT==0
    float f = 1.0;
    vec3 ori = inOri;
    if( ((inInfo>>7)&1u)==0u)
    {
        vec3 wori = normalize( (layer.mLayerToViewer * vec4(ori,0.0) ).xyz );
        //f = clamp( -wori.z, 0.0, 1.0 );
        f = clamp( dot(wori,normalize(cpos)), 0.0, 1.0 );
        f = f*f;
    }
	#else
	const float f = 1.0;
	#endif

    vg.col_tra.w = inColAlpha.w * f * layer.mOpacity;
    #if COLOR_COMPRESSED==0
    vg.col_tra.xyz = inColAlpha.xyz * inColAlpha.xyz;
    #endif
    #if COLOR_COMPRESSED==1
    vg.col_tra.xyz = inColAlpha.xyz;
    #endif

    #if DRAWIN==1
        // this maps the first pt (tim=0) to 0 => 2 and last pt(tim=1) to -1,1
        // so overall -1,2 but not all vertices reach those
        //float drawingT = 2.0*layer.mDrawInTime-inTime;						
        //vg.col_tra *= smoothstep(layer.mAnimParams.z, layer.mAnimParams.z + layer.mAnimParams.w, drawingT);		
    #endif

    //==================================================
    vec3  bPos = cpos;
    vec3  bV = normalize( (layer.mLayerToViewer * vec4(inAxV,0.0) ).xyz );
    vec3  bU = normalize( (layer.mLayerToViewer * vec4(inAxU,0.0) ).xyz );

    //-------- line brush -------------------------

#if BRUSHTYPE==0
    vec3 bWPos = bPos;
#endif

    //-------- ribbon brush -------------------------

#if BRUSHTYPE==1
    float u = float(vid);
    float wb = (-1.0+2.0*u) * inWid * layer.mLayerToViewerScale;
    vec3 bWPos = bPos - wb*bU;
#endif

    //-------- sphere and thick ribbon brush -------------------------

#if BRUSHTYPE==2 || BRUSHTYPE==3
    float u = float(vid)/7.0;
    float a = u*6.283185;
    vec2 sc = vec2( cos(a), sin(a) );
    #if BRUSHTYPE==3
    sc *= vec2(1.0,0.3);
    #endif
    float wb = inWid * layer.mLayerToViewerScale;

    vec3 bWPos = bPos + wb*(bU*sc.x + bV*sc.y);
#endif

    //-------- cube brush -------------------------

#if BRUSHTYPE==4
    float u = float(vid)/4.0;

    float wb = inWid * layer.mLayerToViewerScale;

    // this can be done based on vid with ints, no need to go float
    vec2 sc = vec2( sign(0.2-abs(u-0.65)), sign(abs(u-0.35)-0.3) );   // generate the vertices of a square, for u = {0.0, 0.25, 0.5, 0.75, 1.0=0.0}

    vec3 bWPos = bPos + wb*(bU*sc.x + bV*sc.y);
#endif


    gl_Position = display.mEye[iid].mMatrix_CamPrj * vec4(bWPos,1.0);

}
)";