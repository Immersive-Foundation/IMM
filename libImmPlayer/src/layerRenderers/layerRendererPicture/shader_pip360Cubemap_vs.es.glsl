static const char* shader_pip360Cubemap_vs = R"(

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

layout (std140, row_major, binding=1) uniform LightState
{
    mat4x4      mMatrix_Shadow;
    mat4x4      mInvMatrix_Shadow;
    vec3        mLight;
}light;

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
layout (location=0) in vec3 in_position;
layout (location=1) in vec3 in_normal;

out Vertex_to_fragment_data
{
    vec3 direction;
} out_data;

void main()
{
    #if STEREOMODE==0
    #define iid 0
    #endif
    #if STEREOMODE==1
    #define iid pass.mID
    #endif
    #if STEREOMODE==2
    #define iid VIEW_ID
    #endif
    vec3 layer_position = in_position;
    vec3 viewer_position = (layer.mMatrix * vec4(layer_position,1.0) ).xyz;

    out_data.direction = in_position;
    gl_Position = display.mViewerToEye_Prj[iid]  * vec4(viewer_position,1.0);
}
)";