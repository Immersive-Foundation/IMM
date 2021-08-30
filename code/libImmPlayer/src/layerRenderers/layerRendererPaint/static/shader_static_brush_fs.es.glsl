static const char* shader_static_brush_fs = R"(
#extension GL_EXT_shader_io_blocks : enable

precision highp float;
precision highp sampler2DArray;

layout (std140, row_major, binding=0) uniform FrameState
{
    float       mTime;
    int         mFrame;
}frame;

layout(binding=7) uniform sampler2DArray mTexBlueNoise;

in V2CData
{
    vec4  col_tra;
    flat uint mask;
}vf;

layout(location = 0) out vec4 outColor;

int ComputeAlpha2CoverageNoDither(float InAlpha)
{
    return (0xf0 >> uint(InAlpha * 4.0f + 0.5f)) & 0xf;
}

int BarrelShift4Bit(int mask, uint shift) {
    int b = (mask << 4) | mask;
    return (b >> shift) & 0xf;
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

void main( void )
{
    // debug render modes: normal, wireframe, overdraw, wireframe + overdraw
    vec3 col = vf.col_tra.xyz;
    float al = vf.col_tra.w;

    //if(0.0001 > al) discard;

    outColor = vec4(col, 1.0);

    gl_SampleMask[0] = alpha2coverage(al, ivec2(gl_FragCoord.x / dFdx(gl_FragCoord.x), gl_FragCoord.y / dFdy(gl_FragCoord.y)), uint(frame.mFrame), vf.mask);
}
)";