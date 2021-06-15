static const char* shader_pi2D_vs = R"(

#extension GL_EXT_shader_io_blocks : enable

#if STEREOMODE==2
#extension GL_OVR_multiview : enable
#define VIEW_ID gl_ViewID_OVR
layout(num_views=2) in;
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
    mat4x4      mMatrix; // layer to viewer
    float       mScale;
    float       mOpacity;
    float       mAnimOffset;
    float       mAnimSpeedScale;
    vec4        mDrawInParams;
    vec4        mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint        mID;
}layer;

layout (std140, row_major, binding=4) uniform DisplayState
{
    mat4x4   mViewerToEye_Prj[2];
    vec2     mResolution;
}display;

layout (std140, row_major, binding=5) uniform PassState
{
    int mID;
    int kk1;
    int kk2;
    int kk3;
}pass;

layout (location = 0) uniform vec4 unSize;

layout (location=0) in vec2 inVertex;

out V2FData
{
    vec3 WPos;
    vec3 OPos;
    vec2 UV;
}vo;


void main()
{
    vec3 opos = vec3(unSize.xy * inVertex,0.0);
    vec3 wpos = (layer.mMatrix * vec4(opos,1.0)).xyz;
    vo.WPos = wpos;
    vo.OPos = opos;
    vo.UV = 0.5 + 0.5*inVertex*vec2(1.0,-1.0);

    #if STEREOMODE==0
    #define iid 0
    #endif
    #if STEREOMODE==1
    #define iid pass.mID
    #endif
    #if STEREOMODE==2
    #define iid VIEW_ID
    #endif

    gl_Position = display.mViewerToEye_Prj[iid] * vec4(wpos,1.0);
}
)";
