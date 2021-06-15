static const char* shader_pretessellated_brush_vs = R"(
#extension GL_ARB_shader_draw_parameters : enable
#if STEREOMODE==2
#extension GL_ARB_shader_viewport_layer_array : enable
#endif
//#extension GL_ARB_enhanced_layouts : enable

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

layout (std140, row_major, binding=9) uniform ChunkData
{
	struct
	{
		uint mVertexOffset;
	}mData[128];
}chunk_data;

out V2CData
{
	vec4  col_tra;
	flat uint mask;
}vg;



layout (location=0) in vec3 inVertex;
layout (location=1) in vec4  inColAlpha;
layout (location=2) in vec3  inOri;
layout (location=3) in uint  inInfo;
layout (location=4) in float inTime;


void main()
{
	vec3 pos = inVertex;

	
#if STEREOMODE==0
	#define iid 0
#endif
#if STEREOMODE==1
    #define iid pass.mID
#endif
#if STEREOMODE==2    
	#define iid gl_InstanceID
    gl_ViewportIndex = gl_InstanceID;
#endif


	// wiggle effect
    #if WIGGLE==1  
    {
        pos += layer.mKeepAlive[0].z*sin( layer.mKeepAlive[0].x*inVertex.yzx + layer.mKeepAlive[0].y*frame.mTime);
    }
    #endif
	
	vec3 cpos = (layer.mLayerToViewer * vec4(pos, 1.0)).xyz;

 
 	vg.mask = (inColAlpha.w>0.999) ? layer.mID : inInfo;

	// directional stroke
    float f = 1.0;
	vec3 ori = inOri;
	if( ((inInfo>>7)&1u)==0u)
    {
        vec3 wori = normalize( (layer.mLayerToViewer * vec4(ori,0.0) ).xyz );
		//f = clamp( -wori.z, 0.0, 1.0 );
		f = clamp( dot(wori,normalize(cpos)), 0.0, 1.0 );
        f = f*f;
    }

    vg.col_tra.w = inColAlpha.w * f * layer.mOpacity;
	#if COLOR_COMPRESSED==0
    vg.col_tra.xyz = inColAlpha.xyz * inColAlpha.xyz;
	#endif
	#if COLOR_COMPRESSED==1
    vg.col_tra.xyz = inColAlpha.xyz;
	#endif

    #if DRAWIN==1
		// this maps the first pt (tim=0) to 0 => 2 and last pt(tim=1) to -1,1
		// so overall -1,2 but not all vertices reach those
		//float drawingT = 2.0*layer.mDrawInTime-inTime;						
		//vg.col_tra *= smoothstep(layer.mAnimParams.z, layer.mAnimParams.z + layer.mAnimParams.w, drawingT);		
	#endif

	//==================================================
	gl_Position = display.mEye[iid].mMatrix_CamPrj * vec4(cpos,1.0);
}
)";
