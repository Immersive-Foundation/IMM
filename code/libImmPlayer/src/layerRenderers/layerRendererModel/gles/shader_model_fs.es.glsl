static const char* shader_model_fs = R"(
#extension GL_EXT_shader_io_blocks : enable

precision highp float;

layout (std140, row_major, binding=0) uniform FrameState
{
    float       mTime;
    int         mFrame;
}frame;

layout (std140, row_major, binding=3) uniform LayersState
{
    mat4x4      mMatrix;
    float       mScale;
    float       mOpacity;
    float       mAnimOffset;
    float       mAnimSpeedScale;
    vec4        mDrawInParams;
    vec4        mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint        mID;
}layer;

layout(binding = 7) uniform GlobalResources
{
    uvec2 mTexBlueNoise;
}globalResources;

layout (binding=0) uniform sampler2D unTex0;

in V2FData
{
    vec3 WPos;
    vec3 OPos;
    vec3 Col;
}vf;

out vec4 outColor;

void main( void )
{
    vec3 col = vf.Col;
    float al = layer.mOpacity;
    outColor = vec4(col,al); // DO ALPHA TO COVERAGE HERE!
}
)";