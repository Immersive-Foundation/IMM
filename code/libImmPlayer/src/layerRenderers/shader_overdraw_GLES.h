const char * shader_overdraw_vs = R"(

#extension GL_EXT_shader_io_blocks : enable
#define NUM_VIEWS 2

#if STEREOMODE==2
#extension GL_OVR_multiview : enable
#define VIEW_ID gl_ViewID_OVR
layout(num_views=NUM_VIEWS) in;
#else
#define VIEW_ID 0
#endif

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
    float  mkUnused;
    float  mDrawInTime;
    vec4   mAnimParams; // draw-in and other effect parameters
    vec4   mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint   mID;
}layer;

layout (std140, row_major, binding=4) uniform DisplayState
{
    mat4x4      mViewerToEye_Prj[NUM_VIEWS];
    vec2        mResolution;
}display;

layout (std140, row_major, binding=5) uniform PassState
{
    int mID;
    int kk1;
    int kk2;
    int kk3;
}pass;

#if USE_16_BYTE_VERTS==1

layout (location = 3) uniform float vertexShortScale;

struct vertex_format_t
{
    int x; // posX, posY
    int y; // posZ, wid
    int z; // rgba16, axUx, axUy
    int w; // axUz, axVx, axVy, axVz
};

#else

struct vertex_format_t
{
    float   mPosX;
    float   mPosY;
    float   mPosZ;
    float   mWid;
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
    uint mVertexOffset[128];
} chunk_data;

out V2CData
{
    vec4  col_tra;
    flat uint mask;
}vg;

#if USE_16_BYTE_VERTS==0
vec4 unpack4( uint d )
{
    return vec4( ivec4(d & 255u, (d >> 8u) & 255u, (d >> 16u) & 255u, (d >> 24u) & 255u) ) / 255.0;
}

vec3 unpack3( uint d )
{
    return vec3( ivec3(d & 255u, (d >> 8u) & 255u, (d >> 16u) & 255u) ) / 255.0;
}
#endif

vec3 decodeUnitVector( uint data )
{
#if 0
    uvec3 d = uvec3( data, data>>15, data>>31 ) & uvec3(32767u,65535u,1u);
    vec2  xy = vec2(d.xy)*2.0/vec2(32767.0,65535.0) - 1.0;
    float z  = sqrt(clamp(1.0-dot(xy,xy),0.0,1.0)) * ((d.z==1u)?-1.0:1.0);
    return vec3(xy,z);
#endif
#if 1
    uvec2 d = uvec2( data&65535u, data>>16u );
    vec2 v = vec2(d)/32767.5 - 1.0;
    vec3 nor = vec3(v, 1.0 - abs(v.x) - abs(v.y)); // Rune Stubbe's version,
    float t = max(-nor.z,0.0);                     // much faster than original
    nor.x += (nor.x>0.0)?-t:t;                     // implementation of this
    nor.y += (nor.y>0.0)?-t:t;                     // technique
    return normalize( nor );
#endif
#if 0
    return normalize( -1.0+2.0*vec3( ivec3(data & 1023u,(data>>10u) & 1023u,(data>>20u) & 1023u)) / 1023.0);
#endif
}

void main()
{
    uint real_vertexID = uint(gl_VertexID) + chunk_data.mVertexOffset[0];

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

#if USE_16_BYTE_VERTS==1
    vertex_format_t vertex = data[bid];

    // posx(2) posy(2) | posz(2) wid(2) | rgba(2) axUx(1) axUy(1) | axUz(1) axVx(1) axVy(1) axVz(1)

    float scaleAspect = 1.0 / vertexShortScale;

    vec3 inVertex = vec3(
        scaleAspect * float(vertex.x >> 16),
        scaleAspect * float((vertex.x << 16) >> 16), // Double shift to preserve the sign.
        scaleAspect * float(vertex.y >> 16)
    );
    float inWid = scaleAspect * float(vertex.y & 0xFFFF);

    // A(1)B(5)G(5)R(5)
    // We only have 5 bits for each of RGB in this format and 1 bit for transparency.
    vec4 inColAlpha = vec4(
        float((vertex.z >> 16) & 0x1F) / 32.0,
        float((vertex.z >> 21) & 0x1F) / 32.0,
        float((vertex.z >> 26) & 0x1F) / 32.0,
        float((vertex.z >> 31) & 0x1)
    );

    vec3 inAxU = vec3(
        float((vertex.z >> 8) & 0xFF),
        float(vertex.z & 0xFF),
        float((vertex.w >> 24) & 0xFF) ) / 255.0;

    inAxU = normalize( -1.0+2.0*inAxU );

    vec3 inAxV = vec3(
        float((vertex.w >> 16) & 0xFF),
        float((vertex.w >> 8) & 0xFF),
        float(vertex.w & 0xFF) ) / 255.0;

    inAxV = normalize( -1.0+2.0*inAxV );

#else
    vertex_format_t vertex = data[bid];

    vec3  inVertex   = vec3( vertex.mPosX, vertex.mPosY, vertex.mPosZ );
    uint  inInfo     = vertex.mDirInf>>24u;
    float inTime     = vertex.mTim;
    vec4  inColAlpha = unpack4( vertex.mColAlp );
    vec3  inOri      = normalize( -1.0+2.0*unpack3( vertex.mDirInf ) );
    vec3  inAxU      = decodeUnitVector( vertex.mAxUUn2 );
    vec3  inAxV      = decodeUnitVector( vertex.mAxVUn3 );
    float inWid      = vertex.mWid;
#endif

    vec3 pos = inVertex;

#if STEREOMODE==0
    #define iid 0
#endif
#if STEREOMODE==1
    #define iid pass.mID
#endif
#if STEREOMODE==2
    #define iid VIEW_ID
#endif

    // wiggle effect
    #if WIGGLE==1
    {
        pos += layer.mKeepAlive[0].z*sin( layer.mKeepAlive[0].x*inVertex.yzx + layer.mKeepAlive[0].y*frame.mTime);
    }
    #endif

    vec3 cpos = (layer.mLayerToViewer * vec4(pos, 1.0)).xyz;

    float f = 1.0;

#if USE_16_BYTE_VERTS==0
    vg.mask = (inColAlpha.w>0.999) ? layer.mID : inInfo;

    // directional stroke
    vec3 ori = inOri;
    if( ((inInfo>>7)&1u)==0u)
    {
        vec3 wori = normalize( (layer.mLayerToViewer * vec4(ori,0.0) ).xyz );
        //f = clamp( -wori.z, 0.0, 1.0 );
        f = clamp( dot(wori,normalize(cpos)), 0.0, 1.0 );
        f = f*f;
    }
#endif

    vg.col_tra.w = inColAlpha.w * f * layer.mOpacity;
    #if COLOR_COMPRESSED==0
    vg.col_tra.xyz = inColAlpha.xyz * inColAlpha.xyz;
    #endif
    #if COLOR_COMPRESSED==1
    vg.col_tra.xyz = inColAlpha.xyz;
    #endif
    #if COLOR_COMPRESSED==2
    vg.col_tra.xyz = fbInverseToneMap(inColAlpha.xyz * inColAlpha.xyz);
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

    gl_Position = display.mViewerToEye_Prj[iid] * vec4(bWPos,1.0);
}

)";

const char * shader_overdraw_fs = R"(
#extension GL_EXT_shader_io_blocks : enable

precision highp float;

in V2CData
{
    vec4  col_tra;
    flat uint mask;
}vf;

layout(location = 0) out vec4 outColor;

void main( void )
{
    vec3 col = vf.col_tra.xyz;
    float al = vf.col_tra.w;

    //if(0.0001 > al) discard;

#if RENDER_WIREFRAME==1
    if( bitCount(gl_SampleMaskIn[0]) < 4) { col = vec3(0.0); }
#endif

    outColor = vec4( col, al );
}
)";
 
