static const char* shader_pip360Equirect_fs = R"(
#extension GL_ARB_bindless_texture : enable

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


layout(binding = 7) uniform GlobalResources
{
	uvec2 mTexBlueNoise;
}globalResources;

layout(binding = 0) uniform sampler2D u_tex0;

in Vertex_to_fragment_data
{
    vec3 direction;
#if FORMAT_IS_STEREO==1
    vec4 scale_offset;
#endif
} in_data;

out vec4 out_color;

const float k_pi = 3.1415927;

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
    vec3 nor = normalize( in_data.direction );

    vec2 uv  = vec2(0.5 + 0.5*atan(nor.x,-nor.z)/k_pi, acos(nor.y)/k_pi );
    vec2 uv2 = vec2(0.5 + 0.5*atan(nor.x,abs(nor.z))/k_pi, uv.y); // compute UVs again without seams for correct gradient computation

    #if FORMAT_IS_STEREO==1
    uv = clamp( uv * in_data.scale_offset.xy, vec2(0.0,0.0), vec2(1.0,0.5) ) + in_data.scale_offset.zw;
    #endif

	vec4 te = textureGrad(u_tex0, uv, dFdx(uv2), dFdy(uv2));
    vec3 col = te.xyz;

	#if COLOR_SPACE==0  // linear. Texture colors are in sRGB. Cheap convert to lienar here
	out_color = vec4( pow(col,vec3(2.2)), 1.0 );
	#endif
	#if COLOR_SPACE==1 // gamma. Colors are already in gamma space, nothing to do here
	out_color = vec4( col, 1.0 );
	#endif

	float al = te.w * layer.mOpacity;

	gl_SampleMask[0] = alpha2coverage(al,  ivec2(gl_FragCoord.xy), frame.mFrame, 0);
}
)";