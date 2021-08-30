static const char* shader_pip360Cubemap_vs = R"(

#if STEREOMODE==2
#extension GL_ARB_shader_viewport_layer_array : enable
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
layout (location=0) in vec3 in_position;
layout (location=1) in vec3 in_normal;

out Vertex_to_fragment_data
{
    vec3 direction;
} out_data;

void main()
{
    vec3 layer_position = in_position;
    vec3 viewer_position = (layer.mLayerToViewer * vec4(layer_position,1.0) ).xyz;

    out_data.direction = in_position;

    #if STEREOMODE==0
    #define eye_index 0
	#endif
	#if STEREOMODE==1
    #define eye_index pass.mID
	#endif
	#if STEREOMODE==2
	#define eye_index gl_InstanceID
    gl_ViewportIndex = gl_InstanceID;
	#endif

    gl_Position = display.mEye[eye_index].mMatrix_CamPrj * vec4(viewer_position,1.0);
}
)";