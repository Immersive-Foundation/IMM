static const char* shader_pi2D_fs = R"(

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

layout (binding=0) uniform sampler2D unTex0;

in V2FData
{
    vec3 WPos;
    vec3 OPos;
    vec2 UV;
}vf;

const int comasks[9] = int[9]( 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff );

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

out vec4 outColor;

void main( void )
{
	vec4 te = texture( unTex0, vf.UV );
	vec3 col = te.xyz;
	float al = te.w * layer.mOpacity;

	// HACK ---> This is wrong, in that the texture is already in SRGB space, but we
	//           still need to do this if we want until Quill fixes its color picker
	//           which incorrectly converts from SRGB to linear by squaring the color.
	//           So, the code below renders the images slightly wrong so that the brush
	//           strokes that were created in Quill with the wrong color picker match the
	//           picture from which they were picked.
    outColor = vec4( col*col, 1.0 );

	// Alpha to Coverage
	gl_SampleMask[0] = alpha2coverage(al,  ivec2(gl_FragCoord.xy), frame.mFrame, 0);

	//outColor = te;
}
)";