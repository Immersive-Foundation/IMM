static const char* shader_pip360Cubemap_fs = R"(

#extension GL_EXT_shader_io_blocks : enable

precision highp float;
precision highp sampler2DArray;

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
    mat4x4      mMatrix;
    float       mScale;
    float       mOpacity;
    float       mAnimOffset;
    float       mAnimSpeedScale;
    vec4        mDrawInParams;
    vec4        mKeepAlive[2];        // note, can't pack in an array of float[8] due to granularity of GLSL array types
    uint        mID;
}layer;


layout(binding=7) uniform sampler2DArray mTexBlueNoise;

layout(binding = 0) uniform samplerCube u_tex0;

in Vertex_to_fragment_data
{
    vec3 direction;
} in_data;

out vec4 out_color;

int ComputeAlpha2CoverageNoDither(float InAlpha)
{
    return (0xf0 >> uint(InAlpha * 4.0f + 0.5f)) & 0xf;
}

int alpha2coverage(float al, ivec2 p, uint frameID, uint primitiveID)
{
    // add dithering to the alpha value
    float ran = texelFetch( mTexBlueNoise, ivec3(p, frameID)&63, 0 ).x;
    al = clamp( al + 0.99 * (ran - 0.5f) / 4.0f, 0.0, 1.0);

    // compute sample mask from input alpha, like hardware is doing it
    uint mask = (0xf0u >> uint(al * 4.0f + 0.5f)) & 0xfu;

    // randomize mask per primitive ID and pixel
    uint shift = uint(ran * 3.0f);
    shift += primitiveID;
    shift &= 3u;

    // barrel Shift 4 Bit
    uint b = (mask << 4) | mask;
    mask = (b >> shift) & 0xfu;

    return int(mask);
}

const float k_pi = 3.1415927;

void main( void )
{
    vec3 nor =  in_data.direction ;

    #if DEBUG_RENDER_MODE < 2
    vec4 te = texture(u_tex0, in_data.direction);
    vec3 col = te.xyz;

    float al = te.w * layer.mOpacity;
    #else
    // Gives roughly 12 levels of overdraw.
    vec3 col = vec3(0.08, 0.0, 0.0);
    float al = 1.00;
    #endif

    #if DEBUG_RENDER_MODE == 1 || DEBUG_RENDER_MODE == 3
    if( bitCount(gl_SampleMaskIn[0]) < 4) { col = vec3(0.0); }
        #endif

    out_color = vec4( col, 1.0 );

    gl_SampleMask[0] = alpha2coverage( al, ivec2(gl_FragCoord.x / dFdx(gl_FragCoord.x), gl_FragCoord.y / dFdy(gl_FragCoord.y)), uint(frame.mFrame), 0u );
}
)";