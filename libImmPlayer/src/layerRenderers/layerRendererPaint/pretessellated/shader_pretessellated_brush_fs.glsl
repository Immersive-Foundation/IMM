static const char* shader_pretessellated_brush_fs = R"(
#extension GL_ARB_bindless_texture : enable

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

layout(binding = 7) uniform GlobalResources
{
	uvec2 mTexBlueNoise;
}globalResources;


in vec4 mpos;

//layout (binding=0) uniform sampler2D unTex0;
//layout (binding=1) uniform sampler2D unTex1;

in V2CData
{
	vec4  col_tra;
	flat uint mask;
}vf;

layout(location = 0, index = 0) out vec4 outColor; 
//layout(location = 1, index = 0) out vec4 outColorB;

int alpha2coverage(float al, ivec2 p, uint frameID, uint primitiveID)
{
	const int MSAASampleCount = 8;

	// add dithering to the alpha value

    //vec2 q = vec2(p)+vec2(0,float((frameID)&7)*11.0); float ran = fract( 52.9829189*fract(dot(q,vec2(0.06711056,0.00583715)))); //   http://advances.realtimerendering.com/s2014/index.html
	float ran = texelFetch( sampler2DArray(globalResources.mTexBlueNoise), ivec3(p,frameID)&63, 0 ).x;
	
	al = clamp( al + 0.99*(ran-0.5)/float(MSAASampleCount), 0.0, 1.0); // 0.99 is to make sure the dithering never makes the alpha leak to the previour or the next bucket

	// compute sample mask from input alpha, like hardware is doing it
	uint mask = (0xff00 >> uint(al*float(MSAASampleCount) + 0.5)) & 0xff;

	// randomize mask per primitive ID and pixel
	uint shift = uint(ran*7.0);
	shift += primitiveID;
	shift &= 7;

	// barrel Shift 8 Bit
	uint b = (mask << 8) | mask;
	mask = (b >> shift) & 0xff;

 	return int(mask);
}

void main( void )
{
    vec3 col = vf.col_tra.xyz;
	float al = vf.col_tra.w;

    #if RENDERMODE==1
    if( bitCount(gl_SampleMaskIn[0]) < 8 ) col = vec3(0.0,0.0,0.0);
    #endif

	outColor = vec4( col, 1.0 );

	gl_SampleMask[0] = alpha2coverage( al, ivec2(gl_FragCoord.xy), frame.mFrame, vf.mask );
}
)";
