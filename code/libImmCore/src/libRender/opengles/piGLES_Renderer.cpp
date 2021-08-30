#include <malloc.h>
#include <stdio.h>
#include <cstring>
#include <cwchar>

#if !defined(ANDROID)
// this seems to work just fine on Windows...
#include "gl3.h"
#include "gl31.h"
#include "gl2ext.h"
#include "gl32.h"
#else
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#endif

#define GL_TIME_ELAPSED 0x88BF
#define GL_GPU_DISJOINT 0x8FBB


#include "piGLES_Renderer.h"
#include "../../libBasics/piStr.h"

#if defined(ANDROID)

#include <android/log.h>
#include <cmath>
#include <inttypes.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "piGLES_Renderer", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "piGLES_Renderer", __VA_ARGS__)

#else

#define LOGE(...) do {} while(0)
#define LOGD(...) do {} while(0)

#endif

namespace ImmCore {

typedef struct
{
	unsigned int mProgID;
}piIShader;

typedef struct
{
    unsigned int mObjectID;
    piRenderer::QueryType mType;
}piIQuery;

typedef struct
{
	unsigned int	mObjectID;
	GLuint64		mHandle;
	bool			mIsResident;
	bool			mIOwnIt;
	piRenderer::TextureInfo		mInfo;
	piRenderer::TextureFilter	mFilter;
	piRenderer::TextureWrap		mWrap;
}piITexture;

typedef struct
{
	unsigned int mObjectID;
	piBuffer mVB0;
	piBuffer mVB1;
	piBuffer mEB;
	piRenderer::IndexArrayFormat mIndexArrayType;
}piIVertexArray;

typedef struct
{
	unsigned int mObjectID;
	unsigned int mSize;
	piRenderer::BufferUse mUse;
	GLenum mTarget;
}piIBuffer;

typedef struct
{
	unsigned int mObjectID;
}piISampler;

typedef struct
{
	unsigned int mObjectID;
	unsigned int mSamples;
	unsigned int mXres;
	unsigned int mYres;
}piIRTarget;

static int unidades[32] = { GL_TEXTURE0,  GL_TEXTURE1,	GL_TEXTURE2,  GL_TEXTURE3,
							GL_TEXTURE4,  GL_TEXTURE5,	GL_TEXTURE6,  GL_TEXTURE7,
							GL_TEXTURE8,  GL_TEXTURE9,	GL_TEXTURE10, GL_TEXTURE11,
							GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14, GL_TEXTURE15,
							GL_TEXTURE16, GL_TEXTURE17, GL_TEXTURE18, GL_TEXTURE19,
							GL_TEXTURE20, GL_TEXTURE21, GL_TEXTURE22, GL_TEXTURE23,
							GL_TEXTURE24, GL_TEXTURE25, GL_TEXTURE26, GL_TEXTURE27,
							GL_TEXTURE28, GL_TEXTURE29, GL_TEXTURE30, GL_TEXTURE31 };


static int format2gl(piRenderer::Format format, int *bpp, int *mode, int *moInternal, int *mode3, bool compressed )
{
	switch( format )
	{
	case piRenderer::Format::C1_8_UNORM:		   *bpp =  1; *mode = GL_RED;			   *moInternal = GL_R8;					  *mode3 = GL_UNSIGNED_BYTE; break;
	case piRenderer::Format::C2_8_UNORM:		   *bpp =  2; *mode = GL_RG;			   *moInternal = GL_RG8;				  *mode3 = GL_UNSIGNED_BYTE; break;
  //case piFORMAT_C3I8_RGB:    *bpp =  3; *mode = GL_RGB;			   *moInternal = GL_RGB8;				  *mode3 = GL_UNSIGNED_BYTE; break;
	case piRenderer::Format::C4_8_UNORM:   *bpp =  4; *mode = GL_RGBA;			   *moInternal = GL_RGBA8;				  *mode3 = GL_UNSIGNED_BYTE; break;
	case piRenderer::Format::DS_24_8_UINT:		   *bpp =  4; *mode = GL_DEPTH_COMPONENT;  *moInternal = GL_DEPTH_COMPONENT24;	  *mode3 = GL_UNSIGNED_BYTE;  break;
	case piRenderer::Format::D1_32_FLOAT:		   *bpp =  4; *mode = GL_DEPTH_COMPONENT;  *moInternal = GL_DEPTH_COMPONENT32F;   *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C1_16_FLOAT:	   *bpp =  2; *mode = GL_RED;			   *moInternal = GL_R16F;				  *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C2_16_FLOAT:	   *bpp =  4; *mode = GL_RG;			   *moInternal = GL_RG16F;				  *mode3 = GL_FLOAT;		  break;
  //case piFORMAT_C3F16_RGB:   *bpp =  6; *mode = GL_RGB;			   *moInternal = GL_RGB16F;				  *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C4_16_FLOAT:  *bpp =  8; *mode = GL_RGBA;			   *moInternal = GL_RGBA16F;			  *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C1_32_FLOAT:	   *bpp =  4; *mode = GL_RED;			   *moInternal = GL_R32F;				  *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C4_32_FLOAT:  *bpp = 16; *mode = GL_RGBA;			   *moInternal = GL_RGBA32F;			  *mode3 = GL_FLOAT;		  break;
	case piRenderer::Format::C1_8_UINT:	   *bpp =  1; *mode = GL_RED_INTEGER;	   *moInternal = GL_R8UI;				  *mode3 = GL_UNSIGNED_BYTE;  break;
	case piRenderer::Format::C1_16_UINT:	   *bpp =  2; *mode = GL_RED_INTEGER;	   *moInternal = GL_R16UI;				  *mode3 = GL_UNSIGNED_SHORT; break;
	case piRenderer::Format::C1_32_UINT:	   *bpp =  4; *mode = GL_RED_INTEGER;	   *moInternal = GL_R32UI;				  *mode3 = GL_UNSIGNED_INT;   break;

	case piRenderer::Format::C4_10_10_10_2_UNORM: *bpp = 4; *mode = GL_RGBA;		   *moInternal = GL_RGB10_A2;			  *mode3 = GL_UNSIGNED_BYTE; break;
  //case piFORMAT_C3I111110_RGBA:  *bpp = 4; *mode = GL_RGBA;		   *moInternal = GL_R11F_G11F_B10F;		  *mode3 = GL_UNSIGNED_BYTE; break;

	case piRenderer::Format::C4_8_UINT:	 *bpp =  4; *mode = GL_RGBA_INTEGER;	   *moInternal = GL_RGBA8UI;			  *mode3 = GL_UNSIGNED_BYTE;  break;
	case piRenderer::Format::C4_32_UINT: *bpp = 16; *mode = GL_RGBA_INTEGER;	   *moInternal = GL_RGBA32UI;			  *mode3 = GL_UNSIGNED_INT;	  break;
	case piRenderer::Format::C4_16_SINT: *bpp =  8; *mode = GL_RGBA_INTEGER;	   *moInternal = GL_RGBA16I;			  *mode3 = GL_SHORT;	  break;
	case piRenderer::Format::C4_32_SINT: *bpp = 16; *mode = GL_RGBA_INTEGER;	   *moInternal = GL_RGBA32I;			  *mode3 = GL_INT;	  break;

	default: return( 0 );
	}
	return( 1 );
}

static uint64_t piTexture_GetMem( const piITexture *me )
{
	int mode, fInternal, mode3, bpp;
	if( !format2gl( me->mInfo.mFormat, &bpp, &mode, &fInternal, &mode3, false ) ) return 0;
	return me->mInfo.mXres * me->mInfo.mYres * me->mInfo.mZres * bpp;
}



#define GL_DOUBLE 0x140A
#define GL_MIRROR_CLAMP_TO_EDGE  0x00008743
//#define GL_CLAMP_TO_BORDER 0x0000812d

//static const unsigned int filter2gl[] = { GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
static const unsigned int wrap2gl[]   = { GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRROR_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };
static const unsigned int glType[]	  = { GL_UNSIGNED_BYTE, GL_FLOAT, GL_INT, GL_DOUBLE, GL_HALF_FLOAT, GL_SHORT };
static const unsigned int glSizeof[]  = { 1, 4, 4, 8, 2, 2 };

static GLenum kGlQueryTypes[] = { GL_TIME_ELAPSED , /*GL_TIMESTAMP*/ 0, /*GL_SAMPLES_PASSED*/ 0, GL_ANY_SAMPLES_PASSED };


//---------------------------------------------
// GL error checking
//

#ifdef DEBUG
#define CHECK_GL_ERRORS 1
#endif

#ifdef CHECK_GL_ERRORS

const char * GL_ErrorForEnum( const GLenum e )
{
	switch( e )
	{
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		default: return "Unknown gl error code";
	}
}

bool GL_CheckErrors( const char * file, const char * function, int line )
{
	bool hadError = false;

	// There can be multiple errors that need reporting.
	do
	{
		GLenum err = glGetError();
		if ( err == GL_NO_ERROR )
		{
			break;
		}
		hadError = true;
		LOGE("GL Error at %s:%i %s : %s",  file, line, function, GL_ErrorForEnum( err ) );
	} while ( 1 );
	return hadError;
}

#define GL( func )		func; GL_CheckErrors(__FILE__, __FUNCTION__, __LINE__);

#else

#define GL( func )		func;

#endif

//---------------------------------------------

piRendererGLES::piRendererGLES()
	: piRenderer()
{
}

static const float verts2f[] = { -1.0f,  1.0f,
								 -1.0f, -1.0f,
								  1.0f,  1.0f,
								  1.0f, -1.0f };

static const float verts3f[] = {

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,

	1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,

	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, 1.0f,

	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f };


static const float verts3f3f[] = {

-1.0f, -1.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
-1.0f, -1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,
-1.0f,	1.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
-1.0f,	1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,

 1.0f,	1.0f, -1.0f,   1.0f, 0.0f, 0.0f,
 1.0f,	1.0f,  1.0f,   1.0f, 0.0f, 0.0f,
 1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,
 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 0.0f,

 1.0f, 1.0f,  1.0f,    0.0f, 1.0f, 0.0f,
 1.0f, 1.0f, -1.0f,    0.0f, 1.0f, 0.0f,
-1.0f, 1.0f,  1.0f,    0.0f, 1.0f, 0.0f,
-1.0f, 1.0f, -1.0f,    0.0f, 1.0f, 0.0f,

 1.0f, -1.0f, -1.0f,   0.0f, -1.0f, 0.0f,
 1.0f, -1.0f,  1.0f,   0.0f, -1.0f, 0.0f,
-1.0f, -1.0f, -1.0f,   0.0f, -1.0f, 0.0f,
-1.0f, -1.0f,  1.0f,   0.0f, -1.0f, 0.0f,

-1.0f,	1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
-1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
 1.0f,	1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
 1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f,

-1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
-1.0f,	1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
 1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
 1.0f,	1.0f, -1.0f,   0.0f, 0.0f, -1.0f };


void piRendererGLES::PrintInfo( int showExtensions ) //0=none, 1=inline, 2=mulitline
{
	if( !mReporter ) return;

	char *str = (char*)malloc( 65536 );
	if( !str ) return;

	int nume = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nume);

	sprintf( str, "OpenGL %s\nGLSL %s\n%s by %s\n%d extensions\n", (const char*)glGetString( GL_VERSION ), (const char*)glGetString( GL_SHADING_LANGUAGE_VERSION ),
																   (const char*)glGetString( GL_RENDERER ), (const char*)glGetString( GL_VENDOR ), nume );
	if( showExtensions==1 )
	{
		for( int i=0; i<nume; i++ )
		{
			strcat( str, (char const*)glGetStringi(GL_EXTENSIONS, i) );
			strcat( str, " " );
		}
	}
	else if( showExtensions==2 )
	{
		for( int i=0; i<nume; i++ )
		{
			strcat( str, (char const*)glGetStringi(GL_EXTENSIONS, i) );
			strcat( str, "\n" );
		}
	}
	mReporter->Info( str );
	free( str );
}

bool piRendererGLES::Initialize(int id, const void **hwnd, int num, bool disableVSync, bool disableErrors, piReporter *reporter, bool createDevice, void *device)
{
	mID = id;
	mBindedTarget = nullptr;
	mBoundVertexArray = nullptr;
	mReporter = reporter;

	int nume = 0; glGetIntegerv(GL_NUM_EXTENSIONS, &nume);
	mFeatureVertexViewport = false;
	mFeatureViewportArray  = false;
	for (int i = 0; i<nume; i++)
	{
		if( strcmp( (const char*)glGetStringi(GL_EXTENSIONS, i), "GL_ARB_viewport_array"			 ) == 0) mFeatureViewportArray	= true;
		if( strcmp( (const char*)glGetStringi(GL_EXTENSIONS, i), "GL_ARB_shader_viewport_layer_array") == 0) mFeatureVertexViewport = true;
	}


	int maxTextureUnits, maxViewportDimensions;
	glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits );
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &maxViewportDimensions);

	if( reporter )
	{
		char str[256];
		sprintf( str, "Num Texture Units: %d", maxTextureUnits ); reporter->Info( str );
		//sprintf( str, "Max Vertex per Patch: %d", maxVerticesPerPatch); reporter->Info(str);
		//sprintf( str, "Max GS invocations: %d", maxGSInvocations); reporter->Info(str);
		sprintf(str, "Max viewport dimensions: %d", maxViewportDimensions); reporter->Info(str);
	}

	//--- texture management ---

	mMemCurrent = 0;
	mMemPeak = 0;
	mNumCurrent = 0;
	mNumPeak = 0;
	mNumShaders= 0;
	mPeakShaders  = 0;

	//////////////

	mVBO[0] = this->CreateBuffer(verts2f,	sizeof(verts2f),   BufferType::Static, BufferUse::Vertex);
	mVBO[1] = this->CreateBuffer(verts3f3f, sizeof(verts3f3f), BufferType::Static, BufferUse::Vertex);
	mVBO[2] = this->CreateBuffer(verts3f,	sizeof(verts3f),   BufferType::Static, BufferUse::Vertex);
    if (!mVBO[0] || !mVBO[1] || !mVBO[2])
        return false;

	const piRArrayLayout lay0 = { 2 * sizeof(float), 1, 0, { { 2, piRArrayType_Float, false } } };
	const piRArrayLayout lay1 = { 6 * sizeof(float), 2, 0, { { 3, piRArrayType_Float, false }, { 3, piRArrayType_Float, false } } };
	const piRArrayLayout lay2 = { 3 * sizeof(float), 1, 0, { { 3, piRArrayType_Float, false } } };
	const piRArrayLayout lay3 = { 2 * sizeof(float), 1, 0, { { 2, piRArrayType_Float, false } } };
	const piRArrayLayout lay4 = { 4 * sizeof(float), 2, 0, { { 2, piRArrayType_Float, false }, { 2, piRArrayType_Float, false } } };

	mVA[0] = this->CreateVertexArray(1, mVBO[0], &lay0, nullptr, nullptr, nullptr, IndexArrayFormat::UINT_32);
	mVA[1] = this->CreateVertexArray(1, mVBO[1], &lay1, nullptr, nullptr, nullptr, IndexArrayFormat::UINT_32);
	mVA[2] = this->CreateVertexArray(1, mVBO[2], &lay2, nullptr, nullptr, nullptr, IndexArrayFormat::UINT_32);
    if (!mVA[0] || !mVA[1] || !mVA[2])
        return false;

    mNextPerformanceQuery = 0;
    for (int i = 0; i < QUERY_BUFFER_SIZE; ++i)
	{
		mPerfQueries[i] = this->CreateQuery(QueryType::TimeElapsed);
		if (!mPerfQueries[i])
			return false;
	}

    // GL_EXT_disjoint_timer_query, providing accurate GPU render timing in real-time to the app.
	GLint disjointOccurred;
	glGetIntegerv(GL_GPU_DISJOINT, &disjointOccurred); //clear

    mCurrentRenderState.mDepthClamp = true;
    mCurrentRenderState.mCullMode = piRenderer::CullMode::NONE;
    mCurrentRenderState.mFrontSideIsCounterClockWise = true;

    PrintInfo(1);


	return true;
}

void *piRendererGLES::GetContext(void)
{
	return nullptr;
}

bool piRendererGLES::SupportsFeature(RendererFeature feature)
{
	if (feature == RendererFeature::VIEWPORT_ARRAY)  return mFeatureViewportArray;
	if (feature == RendererFeature::VERTEX_VIEWPORT) return mFeatureVertexViewport;
	return false;
}

piRenderer::API piRendererGLES::GetAPI(void)
{
	return API::GLES;
}

void piRendererGLES::SetActiveWindow( int id )
{
}

void piRendererGLES::Enable(void)
{
}
void piRendererGLES::Disable(void)
{
}

void piRendererGLES::Report( void )
{
    LOGD("piGLES Report shaders: %d buffers %d MEM: %llu MB PEAK: shaders: %d buffers: %d, MEM: %llu MB",mNumShaders, mNumCurrent, mMemCurrent/(1024ull*1024ull), mPeakShaders, mNumPeak, mMemPeak / (1024ull * 1024ull));
    if( !mReporter ) return;

	mReporter->Begin( mMemCurrent, mMemPeak, mNumCurrent, mNumPeak );
	/*
	if( mMngTexNumCurrent!=0 )
	{
		TextureSlot *slots = (TextureSlot*)mMngTexSlots;
		for( int i=0; i<mMngTexNumCurrent; i++ )
		{
			mReporter->Texture( slots[i].mKey,
							   piTexture_GetMem( slots[i].mTexture ) >> 10L,
							   slots[i].mTexture->mInfo.mFormat,
							   slots[i].mTexture->mInfo.mCompressed,
							   slots[i].mTexture->mInfo.mXres,
							   slots[i].mTexture->mInfo.mYres,
							   slots[i].mTexture->mInfo.mZres );
		}
	}*/
	mReporter->End();
}


void piRendererGLES::Deinitialize( void )
{
    for (int i = 0; i < QUERY_BUFFER_SIZE; ++i)
	{
		this->DestroyQuery( mPerfQueries[i] );
	}

	//--- texture management ---

	this->DestroyVertexArray(mVA[0]);
	this->DestroyVertexArray(mVA[1]);
	this->DestroyVertexArray(mVA[2]);
	this->DestroyBuffer(mVBO[0]);
	this->DestroyBuffer(mVBO[1]);
	this->DestroyBuffer(mVBO[2]);
}

void piRendererGLES::SwapBuffers( void )
{
}

void piRendererGLES::StartPerformanceMeasure(void)
{
	// on disjoint exception, clear query buffer
	GLint disjointOccurred = 0;
	glGetIntegerv(GL_GPU_DISJOINT, &disjointOccurred);
	if (disjointOccurred)
	{
		mNumPerformanceQueries = 0;
		LOGD("disjoint timer exception");
	}

	// start new query
	if (mNumPerformanceQueries < QUERY_BUFFER_SIZE)
	{
		this->BeginQuery(mPerfQueries[mNextPerformanceQuery]);
		if (glGetError() == GL_NO_ERROR)
		{
			mNextPerformanceQuery = (mNextPerformanceQuery + 1) % QUERY_BUFFER_SIZE;
			++mNumPerformanceQueries;
		}
		else
		{
			LOGD("glBeginQuery error");
		}
	}
	else
	{
		//LOGD("timer query buffer overflow");
	}
}

void piRendererGLES::EndPerformanceMeasure(void)
{
	if (mNumPerformanceQueries <= 0 )
	{
		return;
	}

	int index = (mNextPerformanceQuery +
				 (QUERY_BUFFER_SIZE - mNumPerformanceQueries)) % QUERY_BUFFER_SIZE;

	this->EndQuery(mPerfQueries[index]);

}

uint64_t piRendererGLES::GetPerformanceMeasure(void)
{
    // on disjoint exception, clear query buffer
	GLint disjointOccurred = false;
	glGetIntegerv(GL_GPU_DISJOINT, &disjointOccurred);
	if (disjointOccurred)
	{
		mNumPerformanceQueries = 0;
		LOGD("disjoint timer exception");
	}

	if (mNumPerformanceQueries == 0)
	{
		return 0;
	}

	// read result of oldest pending query
	// We use buffering to avoid blocking when reading results.
	if (mNumPerformanceQueries > 0)
	{
		int index = (mNextPerformanceQuery +
				(QUERY_BUFFER_SIZE - mNumPerformanceQueries)) % QUERY_BUFFER_SIZE;
		GLuint available = 0;

		piIQuery * me = reinterpret_cast<piIQuery*>(mPerfQueries[index]);

		glGetQueryObjectuiv(me->mObjectID, GL_QUERY_RESULT_AVAILABLE, &available);

		if (available)
		{
			uint64_t elapsed_time_ns = this->GetQueryResult(mPerfQueries[index]);
			--mNumPerformanceQueries;
			if (glGetError() == GL_NO_ERROR)
			{
			    return elapsed_time_ns;
			}
		}
	}
	return 0;
}


void piRendererGLES::SetShadingSamples( int shadingSamples )
{
}

piRTarget piRendererGLES::CreateRenderTarget( piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf )
{
	return nullptr;
}

void piRendererGLES::DestroyRenderTarget( piRTarget obj )
{
	piIRTarget *me = (piIRTarget*)obj;

	glDeleteFramebuffers( 1, (GLuint*)&me->mObjectID );
}

void piRendererGLES::RenderTargetSampleLocations(piRTarget vdst, const float *locations )
{
}

void piRendererGLES::RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location)
{
}

void piRendererGLES::BlitRenderTarget( piRTarget vdst, piRTarget vsrc, bool color, bool depth )
{
}


bool piRendererGLES::SetRenderTarget( piRTarget obj )
{
	return true;
}

void piRendererGLES::SetViewport( int id, const int *vp )
{
	glViewport( vp[0], vp[1], vp[2], vp[3] );
}

void piRendererGLES::SetViewports(int num, const float *viewports) // x,y,w,h,zmin,zmax // 0.0, 0.0, 800.0, 600.0, 0.0, 1.0
{
}

void piRendererGLES::GetViewports(int *num, float *viewports) // x,y,w,h,zmin,zmax
{
}


//===========================================================================================================================================
static int ilog2i(int x)
{
	if (x >= 32768) return 16;
	if (x >= 16384) return 15;
	if (x >= 8192) return 14;
	if (x >= 4096) return 13;
	if (x >= 2048) return 12;
	if (x >= 1024) return 11;
	if (x >= 512) return 10;
	if (x >= 256) return 9;
	if (x >= 128) return 8;
	if (x >= 64) return 7;
	if (x >= 32) return 6;
	if (x >= 16) return 5;
	if (x >= 8) return 4;
	if (x >= 4) return 3;
	if (x >= 2) return 2;
	if (x >= 1) return 1;
	return 0;
}

piTexture piRendererGLES::CreateTextureFromID(unsigned int id, TextureFilter filter )
{
	piITexture *me = (piITexture*)malloc(sizeof(piITexture));
	if (!me)
		return nullptr;

	me->mObjectID = id;
	//me->mInfo = *info;
	me->mFilter = filter;
	//me->mWrap = rwrap;
	me->mIOwnIt = false;
	me->mHandle = 0;
	me->mIsResident = false;

	mNumCurrent++;

	return (piTexture)me;
}


piTexture piRendererGLES::CreateTexture( const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer )
{
	int moFormat, moInternal, moType, bpp;

	if (!format2gl(info->mFormat, &bpp, &moFormat, &moInternal, &moType, compress))
		return nullptr;

	piITexture *me = (piITexture*)malloc(sizeof(piITexture));
	if (!me)
		return nullptr;

	me->mHandle = 0;
	me->mIsResident = false;
	me->mIOwnIt = true;
	me->mInfo = *info;
	me->mFilter = filter;
	me->mWrap = wrap1;

	const int wrap = wrap2gl[static_cast<int>(wrap1)];

	if (info->mType == piRenderer::TextureType::T2D)
	{
		GL( glGenTextures(1, &me->mObjectID) );
		GL( glBindTexture(GL_TEXTURE_2D, me->mObjectID) );

		switch (filter)
		{
			case piRenderer::TextureFilter::NONE:
				GL( glTexImage2D(GL_TEXTURE_2D, 0, moInternal, info->mXres, info->mYres, 0, moFormat, moType, 0) );
				if (buffer)
				{
					const int rowsize = info->mXres*bpp;
					if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					GL( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer) );
				}

				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap) );
				break;

			case piRenderer::TextureFilter::LINEAR:
				GL( glTexImage2D(GL_TEXTURE_2D, 0, moInternal, info->mXres, info->mYres, 0, moFormat, moType, 0) );
				if (buffer)
				{
					const int rowsize = info->mXres*bpp;
					if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					GL( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer) );
				}

				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap) );
				GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap) );
				break;

			case piRenderer::TextureFilter::MIPMAP:
				{
					GL( glTexImage2D(GL_TEXTURE_2D, 0, moInternal, info->mXres, info->mYres, 0, moFormat, moType, 0) );
					if (buffer)
					{
						const int rowsize = info->mXres*bpp;
						if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						GL( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer) );
					}

					const int numMipmaps = ilog2i(info->mXres);
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipmaps) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap) );

					GL( glGenerateMipmap(GL_TEXTURE_2D) );

					if (aniso>1.0001f)
						GL( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso) );

					break;
				}
			case piRenderer::TextureFilter::NONE_MIPMAP:
				{
					const int numMipmaps = ilog2i(info->mXres);
					GL( glTexImage2D(GL_TEXTURE_2D, numMipmaps, moInternal, info->mXres, info->mYres, 0, moFormat, moType, 0) );
					if (buffer)
					{
						const int rowsize = info->mXres*bpp;
						if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						GL( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer) );
					}
					GL( glGenerateMipmap(GL_TEXTURE_2D) );

					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMipmaps) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap) );
					GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap) );

					if (aniso>1.0001f)
						GL( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso) );
					break;
				}
			case piRenderer::TextureFilter::PCF:
				break;
		}
	}
	else if (info->mType == piRenderer::TextureType::T3D)
	{
		GL( glGenTextures(1, &me->mObjectID) );
		GL( glBindTexture(GL_TEXTURE_3D, me->mObjectID) );

		if (filter != piRenderer::TextureFilter::MIPMAP)
		{
			GL( glTexStorage3D(GL_TEXTURE_3D, 1, moInternal, info->mXres, info->mYres, info->mZres) );
			if (buffer) GL( glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
		}
		else
		{
			const int numMipmaps = ilog2i(info->mXres);
			GL( glTexStorage3D(GL_TEXTURE_3D, numMipmaps, moInternal, info->mXres, info->mYres, info->mZres) );
			if (buffer) GL( glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer) );
			GL( glGenerateMipmap(GL_TEXTURE_3D) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, numMipmaps) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
			GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
		}
		GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap) );
		GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap) );
		GL( glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap) );
		if (aniso>1.0001f)
			GL( glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso) );
	}
	else if (info->mType == piRenderer::TextureType::TCUBE)
	{
		// TODO
	}
	else if (info->mType == piRenderer::TextureType::TCUBE_ARRAY)
	{
		// TODO
	}
	else if (info->mType == piRenderer::TextureType::T2D_ARRAY)
	{
		if (info->mMultisample>1)
		{
			// TODO
		}
		else
		{
			GL( glGenTextures(1, &me->mObjectID) );
			GL( glBindTexture(GL_TEXTURE_2D_ARRAY, me->mObjectID) );
			GL( glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, moInternal, info->mXres, info->mYres, info->mZres) );

			if (buffer) GL( glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer) );

			if (filter == piRenderer::TextureFilter::PCF)
			{
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
			}
			else if (filter == piRenderer::TextureFilter::NONE)
			{
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST) );
			}
			else
			{
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
				GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
			}
			GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, wrap) );
			GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap) );
			GL( glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap) );
		}

	}

	me->mHandle = me->mObjectID;

	mMemCurrent += piTexture_GetMem( me );
	mNumCurrent += 1;
	if( mNumCurrent>mNumPeak ) mNumPeak = mNumCurrent;
	if( mMemCurrent>mMemPeak ) mMemPeak = mMemCurrent;

	return piTexture(me);
}

piTexture piRendererGLES::CreateTexture2(const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer, int bindUsage)
{
    return this->CreateTexture(key, info, compress, filter, wrap1, aniso, buffer);
}

void piRendererGLES::ComputeMipmaps( piTexture vme )
{
	piITexture *me = (piITexture*)vme;
	if( me->mFilter!= piRenderer::TextureFilter::MIPMAP ) return;
	GL( glGenerateMipmap(GL_TEXTURE_2D) );
}


void piRendererGLES::DestroyTexture( piTexture vme )
{
	piITexture *me = (piITexture*)vme;

	if( me->mIOwnIt)
	{
		GL( glDeleteTextures(1, &me->mObjectID) );
		mMemCurrent -= piTexture_GetMem(me);
	}
	mNumCurrent--;

	free(me);
}


void piRendererGLES::AttachTextures( int num,
									 piTexture vt0, piTexture vt1, piTexture vt2, piTexture vt3, piTexture vt4, piTexture vt5, piTexture vt6, piTexture vt7,
									 piTexture vt8, piTexture vt9, piTexture vt10, piTexture vt11, piTexture vt12, piTexture vt13, piTexture vt14, piTexture vt15 )
{
	piITexture *t[16] = { (piITexture*)vt0, (piITexture*)vt1, (piITexture*)vt2, (piITexture*)vt3, (piITexture*)vt4, (piITexture*)vt5, (piITexture*)vt6, (piITexture*)vt7,
						  (piITexture*)vt8, (piITexture*)vt9, (piITexture*)vt10, (piITexture*)vt11, (piITexture*)vt12, (piITexture*)vt13, (piITexture*)vt14, (piITexture*)vt15 };

	GLuint texIDs[16];
	for (int i = 0; i<num; i++) {
		texIDs[i] = (t[i]) ? t[i]->mObjectID : 0;
		GL( glActiveTexture(GL_TEXTURE0 + i) );
		GL( glBindTexture(GL_TEXTURE_2D, texIDs[i]) );
	}
}

void piRendererGLES::AttachTextures( int num, piTexture *vt, int offset )
{
    GLuint texIDs[16];
    for (int i = 0; i<num; i++)
    {
        piITexture *t = (piITexture*)vt[i];
        texIDs[i] = t->mObjectID;

		GL( glActiveTexture(GL_TEXTURE0 + offset + i) );

        switch (t->mInfo.mType) {
			case piRenderer::TextureType::T1D:
				GL( glBindTexture(GL_TEXTURE_2D, texIDs[i]) );
				break;
			case piRenderer::TextureType::T2D:
				GL( glBindTexture(GL_TEXTURE_2D, texIDs[i]) );
        		break;
			case piRenderer::TextureType::T3D:
				GL( glBindTexture(GL_TEXTURE_3D, texIDs[i]) );
				break;
			case piRenderer::TextureType::T2D_ARRAY:
				GL( glBindTexture(GL_TEXTURE_2D_ARRAY, texIDs[i]) );
				break;
        	default:
        		// TODO
        		break;
		}
    }
}



void piRendererGLES::DettachTextures( void )
{
}

void piRendererGLES::AttachImage(int unit, piTexture texture, int level, bool layered, int layer, Format format)
{
}

void piRendererGLES::ClearTexture( piTexture vme, int level, const void *data )
{
}

void piRendererGLES::UpdateTexture( piTexture vme, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer )
{
	piITexture *me = (piITexture*)vme;

	int fFormat, fInternal, fType, bpp;
	if( !format2gl( me->mInfo.mFormat, &bpp, &fFormat, &fInternal, &fType, false ) )
		return;

	if( me->mInfo.mType== piRenderer::TextureType::T2D )
	{
		GL( glBindTexture( GL_TEXTURE_2D, me->mObjectID) );
		GL( glTexSubImage2D( GL_TEXTURE_2D, 0, x0, y0, xres, yres, fFormat, fType, buffer) );
		if (me->mFilter == piRenderer::TextureFilter::MIPMAP)
			GL( glGenerateMipmap(GL_TEXTURE_2D) );
	}
	else if( me->mInfo.mType== piRenderer::TextureType::T2D_ARRAY )
	{
		GL( glBindTexture( GL_TEXTURE_3D, me->mObjectID) );
		GL( glTexSubImage3D( GL_TEXTURE_3D, 0, x0, y0, z0, xres, yres, zres, fFormat, fType, buffer) );
	}
}

void piRendererGLES::MakeResident( piTexture vme )
{
}

void piRendererGLES::MakeNonResident( piTexture vme )
{
}

uint64_t piRendererGLES::GetTextureHandle( piTexture vme )
{
	piITexture *me = (piITexture*)vme;
	return me->mHandle;
}


//==================================================


piSampler piRendererGLES::CreateSampler(TextureFilter filter, TextureWrap wrap, float maxAnisotrop)
{
	piISampler *me = (piISampler*)malloc( sizeof(piISampler) );
	if( !me )
		return nullptr;

	GL( glGenSamplers( 1, &me->mObjectID ) );

	int glwrap = wrap2gl[ static_cast<int>(wrap) ];

	if (filter == piRenderer::TextureFilter::NONE)
	{
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f) );
	}
	else if (filter == piRenderer::TextureFilter::LINEAR)
	{
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE) );
		GL( glTexParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f) );
	}
	else if (filter == piRenderer::TextureFilter::MIPMAP)
	{
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f) );
	}
	else // if (filter == piRenderer::TextureFilter::PCF)
	{
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE) );
		GL( glSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f) );
		GL( glSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f) );
	}

	GL( glSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_R, glwrap ) );
	GL( glSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_S, glwrap ) );
	GL( glSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_T, glwrap ) );

	return (piSampler)me;
}

void piRendererGLES::DestroySampler( piSampler obj )
{
	piISampler *me = (piISampler*)obj;
	GL( glDeleteSamplers( 1, &me->mObjectID ) );
}


void piRendererGLES::AttachSamplers(int num, piSampler vt0, piSampler vt1, piSampler vt2, piSampler vt3, piSampler vt4, piSampler vt5, piSampler vt6, piSampler vt7)
{
	piISampler *t[8] = { (piISampler*)vt0, (piISampler*)vt1, (piISampler*)vt2, (piISampler*)vt3, (piISampler*)vt4, (piISampler*)vt5, (piISampler*)vt6, (piISampler*)vt7 };
	for(int i = 0; i < num; i++)
	{
		if (t[i])
		{
			glBindSampler(i, t[i]->mObjectID);
		}
	}
}

void piRendererGLES::DettachSamplers( void )
{
	for(GLuint i = 0; i < 8; i++)
	{
		glBindSampler(i, 0);
	}
}

//===========================================================================================================================================
static const char *versionStr = "#version 320 es\n";

static bool createOptionsString(char *buffer, const int bufferLength, const piShaderOptions *options )
{
	const int num = options->mNum;
	if (num>64) return false;

	int ptr = 0;
	for (int i = 0; i<num; i++)
	{
		int offset = pisprintf(buffer + ptr, bufferLength - ptr, "#define %s %d\n", options->mOption[i].mName, options->mOption[i].mValue);
		ptr += offset;
	}
	buffer[ptr] = 0;

	return true;
}

//piShader piRendererGLES::CreateShaderRaw(const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
//{
//   return nullptr;
//}

piShader piRendererGLES::CreateShader( const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
{
	piIShader *me = (piIShader*)malloc( sizeof(piIShader) );
	if( !me )
		return nullptr;

	const char *vtext = vs;
	const char *ctext = cs;
	const char *etext = es;
	const char *ftext = fs;

	//LOGD("CreateShader %s", vtext);
	me->mProgID = GL( glCreateProgram() );

	const int mVertShaderID = vs?glCreateShader( GL_VERTEX_SHADER ):-1;
	const int mCtrlShaderID = cs?glCreateShader( GL_TESS_CONTROL_SHADER ):-1;
	const int mEvalShaderID = es?glCreateShader( GL_TESS_EVALUATION_SHADER ):-1;
	const int mFragShaderID = fs?glCreateShader( GL_FRAGMENT_SHADER ):-1;

	char optionsStr[80*64] = { 0 };
	if (options != nullptr)
	{
		if( !createOptionsString(optionsStr, 80*64, options) )
			return nullptr;
	}

	const GLchar *vstrings[3] = { versionStr, optionsStr, vtext };
	const GLchar *cstrings[4] = { versionStr, optionsStr, ctext };
	const GLchar *estrings[4] = { versionStr, optionsStr, etext };
	const GLchar *fstrings[3] = { versionStr, optionsStr, ftext };

	if( vs ) GL( glShaderSource(mVertShaderID, 3, vstrings, 0) );
	if( cs ) GL( glShaderSource(mCtrlShaderID, 3, cstrings, 0) );
	if( es ) GL( glShaderSource(mEvalShaderID, 3, estrings, 0) );
	if( fs ) GL( glShaderSource(mFragShaderID, 3, fstrings, 0) );


	int result = 0;

	//--------
	if( vs )
	{
		GL( glCompileShader( mVertShaderID ) );
		GL( glGetShaderiv( mVertShaderID, GL_COMPILE_STATUS, &result ) );
		if( !result )
		{
			error[0]='V'; error[1]='S'; error[2]=':'; glGetShaderInfoLog( mVertShaderID, 1024, NULL, (char *)(error+3) );
			LOGE("GL Error at %s:%i %s : %s",  __FILE__, __LINE__, __FUNCTION__, error);
			return nullptr;
		}

	}
	//--------
	if( cs )
	{
		GL(glCompileShader( mCtrlShaderID ));
		GL(glGetShaderiv( mCtrlShaderID, GL_COMPILE_STATUS, &result ));
		if( !result )
		{
			if( error ) { error[0]='C'; error[1]='S'; error[2]=':'; glGetShaderInfoLog( mCtrlShaderID, 1024, NULL, (char *)(error+3) ); }
			LOGE("GL Error at %s:%i %s : %s",  __FILE__, __LINE__, __FUNCTION__, error);
			return nullptr;
		}
	}
	//--------
	if( es )
	{
		GL(glCompileShader( mEvalShaderID ));
		GL(glGetShaderiv( mEvalShaderID, GL_COMPILE_STATUS, &result ));
		if( !result )
		{
			if( error ) { error[0]='E'; error[1]='S'; error[2]=':'; glGetShaderInfoLog( mEvalShaderID, 1024, NULL, (char *)(error+3) ); }
			LOGE("GL Error at %s:%i %s : %s",  __FILE__, __LINE__, __FUNCTION__, error);
			return nullptr;
		}
	}
	if( fs )
	{
		GL( glCompileShader( mFragShaderID ) );
		GL( glGetShaderiv( mFragShaderID, GL_COMPILE_STATUS, &result ) );
		if( !result )
		{
			error[0]='F'; error[1]='S'; error[2]=':'; glGetShaderInfoLog( mFragShaderID, 1024, NULL, (char *)(error+3) );
			LOGE("GL Error at %s:%i %s : %s",  __FILE__, __LINE__, __FUNCTION__, error);
			return nullptr;
		}
	}

	if( vs ) GL( glAttachShader( me->mProgID, mVertShaderID ) );
	if( cs ) GL( glAttachShader( me->mProgID, mCtrlShaderID ) );
	if( es ) GL( glAttachShader( me->mProgID, mEvalShaderID ) );
	if( fs ) GL( glAttachShader( me->mProgID, mFragShaderID ) );


	GL( glLinkProgram( me->mProgID ) );
	GL( glGetProgramiv( me->mProgID, GL_LINK_STATUS, &result ) );
	if( !result )
	{
		error[0]='L'; error[1]='I'; error[2]=':'; glGetProgramInfoLog( me->mProgID, 1024, NULL, (char *)(error+3) );
		LOGE("GL Error at %s:%i %s : %s",  __FILE__, __LINE__, __FUNCTION__, error);
		return nullptr;
	}

	if( vs ) glDeleteShader( mVertShaderID );
	if( cs ) glDeleteShader( mCtrlShaderID );
	if( es ) glDeleteShader( mEvalShaderID );
	if( fs ) glDeleteShader( mFragShaderID );

	mNumShaders++;
	if (mNumShaders>mPeakShaders) mPeakShaders=mNumShaders;

	return (piShader)me;
}

piShader piRendererGLES::CreateShaderBinary(const piShaderOptions *options, const uint8_t *vs, const int vs_len, const uint8_t *cs, const int cs_len, const uint8_t *es, const int es_len, const uint8_t *gs, const int gs_len, const uint8_t *fs, const int fs_len, char *error)
{
	return nullptr;
}

piShader piRendererGLES::CreateCompute(const piShaderOptions *options, const char *cs, char *error)
{
	return nullptr;
}


void piRendererGLES::DestroyShader( piShader vme )
{
	piIShader *me = (piIShader *)vme;
	GL( glDeleteProgram( me->mProgID ) );
	mNumShaders --;
	free(me);
}

void piRendererGLES::AttachShader( piShader vme )
{
	piIShader *me = (piIShader *)vme;
	GL( glUseProgram( me->mProgID ) );
}

void piRendererGLES::DettachShader( void )
{
	GL( glUseProgram( 0 ) );
}

void piRendererGLES::AttachShaderConstants(piBuffer obj, int unit)
{
	piIBuffer *me = (piIBuffer *)obj;
	GL( glBindBufferRange(GL_UNIFORM_BUFFER, unit, me->mObjectID, 0, me->mSize) );
}

void piRendererGLES::AttachShaderBuffer(piBuffer obj, int unit)
{
	piIBuffer *me = (piIBuffer *)obj;
	GL( glBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, me->mObjectID ) );
}

void piRendererGLES::DettachShaderBuffer(int unit)
{
	GL( glBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, 0) );
}

void piRendererGLES::AttachAtomicsBuffer(piBuffer obj, int unit)
{
	piIBuffer *me = (piIBuffer *)obj;
	GL( glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, me->mObjectID) );
}

void piRendererGLES::DettachAtomicsBuffer(int unit)
{
	GL( glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, 0) );
}

void piRendererGLES::SetShaderConstant4F(const unsigned int pos, const float *value, int num)
{
	GL( glUniform4fv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant3F(const unsigned int pos, const float *value, int num)
{
	GL( glUniform3fv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant2F(const unsigned int pos, const float *value, int num)
{
	GL( glUniform2fv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant1F(const unsigned int pos, const float *value, int num)
{
	GL( glUniform1fv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant1I(const unsigned int pos, const int *value, int num)
{
	GL( glUniform1iv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num)
{
	GL( glUniform1uiv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant2UI(const unsigned int pos, const unsigned int *value, int num)
{
	GL( glUniform2uiv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant3UI(const unsigned int pos, const unsigned int *value, int num)
{
	GL( glUniform3uiv(pos,num,value) );
}
void piRendererGLES::SetShaderConstant4UI(const unsigned int pos, const unsigned int *value, int num)
{
	GL( glUniform4uiv(pos,num,value) );
}


void piRendererGLES::SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose)
{
	GL( glUniformMatrix4fv(pos,num,transpose,value) );
	//glProgramUniformMatrix4fv( ((piIShader *)mBindedShader)->mProgID, pos, num, transpose, value ); // can do without binding!
}
void piRendererGLES::SetShaderConstantSampler(const unsigned int pos, int unit)
{
	GL( glUniform1i(pos,unit) );
}

static const int r2gl_blendMode[] = {
	GL_ONE,
	GL_SRC_ALPHA,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA_SATURATE,
	GL_ZERO
};

static const int r2gl_blendEqua[] = {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX
};

void piRendererGLES::SetBlending(int buf,  BlendEquation equRGB, BlendOperations srcRGB, BlendOperations dstRGB,
										   BlendEquation equALP, BlendOperations srcALP, BlendOperations dstALP )
{

    // 3.2 only
    glBlendEquationSeparatei(buf, r2gl_blendEqua[static_cast<int>(equRGB)], r2gl_blendEqua[static_cast<int>(equALP)]);
    glBlendFuncSeparatei(buf, r2gl_blendMode[static_cast<int>(srcRGB)], r2gl_blendMode[static_cast<int>(dstRGB)],
                         r2gl_blendMode[static_cast<int>(srcALP)], r2gl_blendMode[static_cast<int>(dstALP)]);

}

void piRendererGLES::SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z )
{
	GL( glDepthMask( z? GL_TRUE : GL_FALSE ) );
	// On QCOM chipsets using a RGB rendertarget or a color mask of glColorMask( true, true, true, false) can cause a
	// 30% drop in performance. Setting colormask != 0xF for a draw disables order independent depth writes for those
	// draws, which affects depth rejection for subsequent draws.
	GL( glColorMask( c0, c0, c0, c0 ) );
}

void piRendererGLES::SetState( piState state, bool value )
{
	if( state==piSTATE_FRONT_FACE )
	{
		if( !value ) glFrontFace( GL_CW );
		else		 glFrontFace( GL_CCW );

		mCurrentRenderState.mFrontSideIsCounterClockWise = value;
	}
	else if (state == piSTATE_DEPTH_TEST)
	{
		if (value) glEnable(GL_DEPTH_TEST);
		else	   glDisable(GL_DEPTH_TEST);

		mCurrentRenderState.mDepthTest = value;
	}
	else if( state== piSTATE_CULL_FACE )
	{
		if( value ) glEnable(GL_CULL_FACE);
		else		glDisable(GL_CULL_FACE);

		// Otherwise needs piSTATE_CULL_FRONT_FACE to be set to determine which face is to be culled
		if (!value) mCurrentRenderState.mCullMode = piRenderer::CullMode::NONE;
	}
	else if( state == piSTATE_ALPHA_TO_COVERAGE )
	{
		if (value)	{ glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE); glDisable(GL_SAMPLE_COVERAGE ); }
		else		{ glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE); glDisable(GL_SAMPLE_COVERAGE); }
	}
	else if( state == piSTATE_BLEND )
	{
		if (value) glEnable(GL_BLEND);
		else	   glDisable(GL_BLEND);
		mCurrentRenderState.mBlend = value;
	}
}

typedef struct
{
    struct
    {
        bool wireFrame;
        bool depthClamp;
        piRenderer::CullMode cullMode;
        bool mFrontSideIsCounterClockWise;
        bool mMultisample;
    }mDesc;
}piIRasterState;

piRasterState piRendererGLES::CreateRasterState(bool wireframe, bool frontIsCounterClockWise, CullMode cullMode, bool depthClamp, bool multiSample)
{
    piIRasterState *me = (piIRasterState*)malloc(sizeof(piIRasterState));
    if (!me)
        return nullptr;

    me->mDesc.wireFrame = wireframe;
    me->mDesc.depthClamp = depthClamp;
    me->mDesc.cullMode = cullMode;
    me->mDesc.mFrontSideIsCounterClockWise = frontIsCounterClockWise;
    me->mDesc.mMultisample = multiSample;

    return (piRasterState)me;
}

void piRendererGLES::SetRasterState(const piRasterState vme)
{
    piIRasterState *me = (piIRasterState*)vme;

    if (me->mDesc.depthClamp != mCurrentRenderState.mDepthClamp)
    {
        //if (me->mDesc.depthClamp) glEnable(GL_DEPTH_CLAMP);  else glDisable(GL_DEPTH_CLAMP);
        mCurrentRenderState.mDepthClamp = me->mDesc.depthClamp;
    }

    if (me->mDesc.cullMode != mCurrentRenderState.mCullMode)
    {
        if (me->mDesc.cullMode == piRenderer::CullMode::NONE) { glDisable(GL_CULL_FACE); }
        else   if (me->mDesc.cullMode == piRenderer::CullMode::FRONT) { glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); }
        else /*if (me->mDesc.cullMode == piRenderer::CullMode::BACK)*/ { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); }

        mCurrentRenderState.mCullMode = me->mDesc.cullMode;
    }

    if (me->mDesc.mFrontSideIsCounterClockWise != mCurrentRenderState.mFrontSideIsCounterClockWise)
    {
        if (me->mDesc.mFrontSideIsCounterClockWise) glFrontFace(GL_CCW); else glFrontFace(GL_CW);
        mCurrentRenderState.mFrontSideIsCounterClockWise = me->mDesc.mFrontSideIsCounterClockWise;
    }
}

void piRendererGLES::DestroyRasterState(piRasterState vme)
{
	piIRasterState *me = (piIRasterState*)vme;
	free(me);
}

//----------------------------------------------------
typedef struct
{
	struct
	{
		bool mAlphaToCoverage;
		bool mIndependentBlendEnable;
		struct
		{
			bool mBlendEnabled;
		}mRenderTarget[8];
	}mDesc;
}piIBlendState;

piBlendState piRendererGLES::CreateBlendState(bool alphaToCoverage, bool enabled0)
{
	piIBlendState *me = (piIBlendState*)malloc(sizeof(piIBlendState));
	if (!me)
		return nullptr;

	me->mDesc.mAlphaToCoverage = alphaToCoverage;
	me->mDesc.mIndependentBlendEnable = false;
	me->mDesc.mRenderTarget[0].mBlendEnabled = enabled0;

	return (piBlendState)me;
}

void piRendererGLES::SetBlendState(const piBlendState vme)
{
	piIBlendState *me = (piIBlendState*)vme;

	if (me->mDesc.mAlphaToCoverage)
	{
		GL( glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) );
		GL( glDisable(GL_SAMPLE_COVERAGE) );
	}
	else
	{
		GL( glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE) );
		GL( glDisable(GL_SAMPLE_COVERAGE) );
	}
}

void piRendererGLES::DestroyBlendState(piBlendState vme)
{
	piIBlendState *me = (piIBlendState*)vme;
	free(me);
}

//-------------------------
struct piIDepthState
{
	int dummy;
};
piDepthState piRendererGLES::CreateDepthState(bool depthEnable, bool enabled0)
{
	piIDepthState *me = (piIDepthState*)malloc(sizeof(piIDepthState));
	if (!me)
		return nullptr;

	return (piDepthState)me;
}

void piRendererGLES::SetDepthState(const piDepthState vme)
{
	piIDepthState *me = (piIDepthState*)vme;
}

void piRendererGLES::DestroyDepthState(piDepthState vme)
{
	piIDepthState *me = (piIDepthState*)vme;
	free(me);
}

//-------------------------
void piRendererGLES::Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 )
{
	if( mBindedTarget == NULL )
	{
		int mode = 0;
		if( color0 ) { mode |= GL_COLOR_BUFFER_BIT;   glClearColor( color0[0], color0[1], color0[2], color0[3] ); }
		if( depth0 ) { mode |= GL_DEPTH_BUFFER_BIT;   glClearDepthf( 1.0f ); }
		glClear( mode );
	}
	else
	{
		float z = 1.0f;
		if( color0 ) GL( glClearBufferfv( GL_COLOR, 0, color0 ) );
		if( color1 ) GL( glClearBufferfv( GL_COLOR, 1, color1 ) );
		if( color2 ) GL( glClearBufferfv( GL_COLOR, 2, color2 ) );
		if( color3 ) GL( glClearBufferfv( GL_COLOR, 3, color3 ) );
		if( depth0 ) GL( glClearBufferfv( GL_DEPTH, 0, &z ) );
	}
//	  glClearBufferfi( GL_DEPTH_STENCIL, 0, z, s );
}

//-----------------------

piBuffer piRendererGLES::CreateBuffer(const void *data, unsigned int amount, BufferType mode, BufferUse use)
{
	piIBuffer *me = (piIBuffer*)malloc(sizeof(piIBuffer));
	if (!me)
		return nullptr;

	GL( glGenBuffers(1, &me->mObjectID) );

	GLenum usage = mode == BufferType::Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
	if (use == BufferUse::Vertex)
	{
		me->mTarget = GL_ARRAY_BUFFER;
	}
	else if (use == BufferUse::Index)
	{
		me->mTarget = GL_ELEMENT_ARRAY_BUFFER;
	}
	else if (use == BufferUse::ShaderResource)
	{
		me->mTarget = GL_SHADER_STORAGE_BUFFER;
	}
	else
	{
		me->mTarget = GL_UNIFORM_BUFFER;
	}

	GL( glBindBuffer(me->mTarget, me->mObjectID) );
	GL( glBufferData(me->mTarget, amount, data, usage) );

	me->mSize = amount;
	me->mUse = use;

	mMemCurrent += amount;
	mNumCurrent += 1;
	if (mNumCurrent>mNumPeak) mNumPeak = mNumCurrent;
	if (mMemCurrent>mMemPeak) mMemPeak = mMemCurrent;

	// TODO: remove?
	GL( glBindBuffer(me->mTarget, 0) );

	return (piBuffer)me;
}

void piRendererGLES::DestroyBuffer(piBuffer vme)
{
	piIBuffer *me = (piIBuffer*)vme;
	GL( glDeleteBuffers(1, &me->mObjectID ) );
	mMemCurrent -= me->mSize;
	mNumCurrent -= 1;
	free(me);
}

piBuffer piRendererGLES::CreateStructuredBuffer(const void *data, unsigned int numElements, unsigned int elementSize, BufferType mode, BufferUse use)
{
	return CreateBuffer(data, numElements*elementSize, mode, use);
}

piBuffer piRendererGLES::CreateBufferMapped_Start(void **ptr, unsigned int amount, BufferUse use)
{
	return nullptr;
}

void piRendererGLES::CreateBufferMapped_End(piBuffer vme)
{
}

void piRendererGLES::UpdateBuffer(piBuffer obj, const void *data, int offset, int len, bool invalidate)
{
	piIBuffer *me = (piIBuffer *)obj;

	GL( glBindBuffer(me->mTarget, me->mObjectID) );
	GL( glBufferSubData(me->mTarget, offset, len, data) );
#if 0
	GLint size = 0;
	GL( glGetBufferParameteriv(me->mTarget, GL_BUFFER_SIZE, &size) );
#endif
}

piVertexArray piRendererGLES::CreateVertexArray( int numStreams,
												 piBuffer vb0, const piRArrayLayout *streamLayout0,
												 piBuffer vb1, const piRArrayLayout *streamLayout1,
												 piBuffer eb, const IndexArrayFormat ebFormat )
{
	piIVertexArray *me = (piIVertexArray*)malloc(sizeof(piIVertexArray));
	if( !me )
		return nullptr;

	GL( glGenVertexArrays(1, &me->mObjectID) );
	if (!me->mObjectID)
		return nullptr;

	GL( glBindVertexArray(me->mObjectID) );

	me->mVB0 = vb0;
	me->mVB1 = vb1;
	me->mEB = eb;

	unsigned int aid = 0;

	for (int j = 0; j < numStreams; j++)
	{
		unsigned int sid = j;

		const piRArrayLayout * st = (j == 0) ? streamLayout0 : streamLayout1;
		piBuffer vb = (j == 0) ? vb0 : vb1;

		//GL( glBindBuffer(((piIBuffer *)vb)->mTarget, ((piIBuffer *)vb)->mObjectID) );
        glBindVertexBuffer(sid, ((piIBuffer*)vb)->mObjectID, 0, st->mStride);
        glVertexBindingDivisor(sid, st->mDivisor);
        GLuint offset = 0;
		const int num = st->mNumElements;
        for (int i = 0; i < num; i++)
		{
			GL( glEnableVertexAttribArray(aid) );
			GL( glVertexAttribFormat(aid, st->mEntry[i].mNumComponents, glType[st->mEntry[i].mType], static_cast<GLboolean>(st->mEntry[i].mNormalize), offset) );
            GL( glVertexAttribBinding(aid, sid));
			offset += st->mEntry[i].mNumComponents*glSizeof[st->mEntry[i].mType];
			aid++;
		}
	}

	if (eb && ((piIBuffer *)eb)->mObjectID)
	{
		me->mIndexArrayType = ebFormat;
		GL( glBindBuffer(((piIBuffer *)eb)->mTarget, ((piIBuffer *)eb)->mObjectID) );
	}

    GL(glBindVertexArray(0));

	// element buffer will be bound before drawElements
	return (piVertexArray)me;
}

void piRendererGLES::DestroyVertexArray(piVertexArray vme)
{
	piIVertexArray *me = (piIVertexArray*)vme;
	GL( glDeleteVertexArrays(1, &me->mObjectID) );
}

void piRendererGLES::AttachVertexArray(piVertexArray vme)
{
	piIVertexArray *me = (piIVertexArray*)vme;
	GL( glBindVertexArray(me->mObjectID) );

	piIBuffer * vb0 = (piIBuffer *)me->mVB0;
	piIBuffer * vb1 = (piIBuffer *)me->mVB1;
	piIBuffer * eb = (piIBuffer *)me->mEB;

#if 0 // Need to bind SSBO buffer.
	if (vb0 && vb0->mObjectID)
	{
		GL( glBindBuffer(vb0->mTarget, vb0->mObjectID) );
	}
	if (vb1 && vb1->mObjectID)
	{
		GL( glBindBuffer(vb1->mTarget, vb1->mObjectID) );
	}
#endif
	if (eb && eb->mObjectID)
	{
		GL( glBindBuffer(eb->mTarget, eb->mObjectID) );
	}
	mBoundVertexArray = vme;
}

void piRendererGLES::DettachVertexArray( void )
{
	GL( glBindVertexArray( 0 ) );
}

piVertexArray piRendererGLES::CreateVertexArray2(int numStreams, piBuffer vb0, const ArrayLayout2 *streamLayout0, piBuffer vb1, const ArrayLayout2 *streamLayout1, const void *shaderBinary, size_t shaderBinarySize, piBuffer ib, const IndexArrayFormat ebFormat)
{
	return nullptr;
}

void piRendererGLES::AttachVertexArray2(piVertexArray vme)
{
}

void piRendererGLES::DestroyVertexArray2(piVertexArray vme)
{
}

void piRendererGLES::AttachPixelPackBuffer(piBuffer obj)
{
}

void piRendererGLES::DettachPixelPackBuffer(void)
{
}

GLenum iPrimtive_pi2gl(piRenderer::PrimitiveType pt)
{
	GLenum glpt = GL_TRIANGLES;
	if (pt == piRenderer::PrimitiveType::Triangle)			   glpt = GL_TRIANGLES;
	else if (pt == piRenderer::PrimitiveType::Point)		   glpt = GL_POINTS;
	else if (pt == piRenderer::PrimitiveType::TriangleStrip)   glpt = GL_TRIANGLE_STRIP;
	else if (pt == piRenderer::PrimitiveType::LineStrip)	   glpt = GL_LINE_STRIP;
	else if (pt == piRenderer::PrimitiveType::Lines)		   glpt = GL_LINES;
	return glpt;
}

void piRendererGLES::DrawPrimitiveIndexed(PrimitiveType pt, uint32_t num, uint32_t numInstances, uint32_t baseVertex, uint32_t baseInstance, uint32_t baseIndex)
{
	GLenum glpt = iPrimtive_pi2gl(pt);
	piIVertexArray *va = (piIVertexArray*)mBoundVertexArray;
#if 0
    int vertexarraybound, elementarraybound, elementarraybuffersize, maxVertexAttributes, maxVaryingVectors;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexarraybound);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttributes);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVectors);
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &elementarraybuffersize);

    struct attrib {
        int isOn;
        int binding;
        int bindingBuffer;
        int divisor;
        int numComponents;
    } atts[16];

    for (unsigned i = 0; i < 16; ++i)
    {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &atts[i].isOn);
        if (atts[i].isOn)
        {
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &atts[i].bindingBuffer);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_BINDING, &atts[i].binding);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &atts[i].numComponents);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &atts[i].divisor);
        }
    }
#endif

    uint64_t indiceOffset = baseIndex * ((va->mIndexArrayType == IndexArrayFormat::UINT_32) ? 4 : 2 );

	// OpenGL ES 3.2 only!
	GL( glDrawElementsInstancedBaseVertex(
			glpt,
			num,
			(va->mIndexArrayType==IndexArrayFormat::UINT_32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
			(void*)indiceOffset,   // indices offset
			numInstances,
			baseVertex) );
}

void piRendererGLES::DrawPrimitiveNotIndexed(PrimitiveType pt, int first, int num, int numInstanced)
{
	GLenum glpt = iPrimtive_pi2gl(pt);
	GL( glDrawArraysInstanced(glpt, first, num, numInstanced) );
}


void piRendererGLES::DrawPrimitiveNotIndexedMultiple(PrimitiveType pt, const int *firsts, const int *counts, int num)
{
}


void piRendererGLES::DrawPrimitiveNotIndexedIndirect(PrimitiveType pt, piBuffer cmds, int num)
{

}

void piRendererGLES::DettachIndirectBuffer(void)
{
	GL( glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0) );
}

void piRendererGLES::DrawPrimitiveIndirect(PrimitiveType pt, piBuffer cmds, uint32_t offset, uint32_t num)
{
}

void piRendererGLES::DrawUnitQuad_XY( int numInstanced )
{
	this->AttachVertexArray( mVA[0] );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced) );
	this->DettachVertexArray();
}

void piRendererGLES::DrawUnitCube_XYZ_NOR(int numInstanced)
{
	this->AttachVertexArray(mVA[1]);
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 4, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 8, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 12, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 16, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 20, 4, numInstanced) );
	this->DettachVertexArray();
}

void piRendererGLES::DrawUnitCube_XYZ(int numInstanced)
{
	this->AttachVertexArray(mVA[2]);
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 4, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 8, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 12, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 16, 4, numInstanced) );
	GL( glDrawArraysInstanced(GL_TRIANGLE_STRIP, 20, 4, numInstanced) );
	this->DettachVertexArray();
}

void piRendererGLES::DrawTestSquare()
{
	GLfloat vertices[] = {
		-0.5, -0.5, 0, // bottom left corner
		-0.5,  0.5, 0, // top left corner
		0.5,  0.5, 0, // top right corner
		0.5, -0.5, 0}; // bottom right corner

	GLubyte indices[] = {
		0,1,2, // first triangle (bottom left - top left - top right)
		0,2,3}; // second triangle (bottom left - top right - bottom right)

	GLfloat texCoords[] = {
		0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0
	};

	GL( glBindVertexArray(0) );
	GL( glBindBuffer(GL_ARRAY_BUFFER, 0) );
	GL( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );

	// Test using color as texture.
	// GLuint texID;
	// GL( glGenTextures(1, &texID) );
	// GL( glBindTexture(GL_TEXTURE_2D, texID) );

	// int texData = 0xFF0000FF;

	// GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texData) );

	int posIndex = 0; // layout location bound in shader
	GL( glEnableVertexAttribArray(posIndex) );
	GL( glVertexAttribPointer(posIndex, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, vertices) );

	int texCoordIndex = 1;
	GL( glEnableVertexAttribArray(texCoordIndex) );
	GL( glVertexAttribPointer(texCoordIndex, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2, texCoords) );

	GL( glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices) );
}

void piRendererGLES::ExecuteCompute(int tx, int ty, int tz, int gsx, int gsy, int gsz)
{
	int ngx = tx / gsx; if( (ngx*gsx) < tx ) ngx++;
	int ngy = ty / gsy; if( (ngy*gsy) < ty ) ngy++;
	int ngz = tz / gsz; if( (ngz*gsz) < tz ) ngz++;

	GL( glDispatchCompute( ngx, ngy, ngz ) );
}

void piRendererGLES::SetLineWidth( float size )
{
	GL( glLineWidth( size ) );
}

void piRendererGLES::SetPointSize( bool mode, float size )
{
}

void piRendererGLES::GetTextureRes( piTexture vme, int *res )
{
	piITexture *me = (piITexture*)vme;
	res[0] = me->mInfo.mXres;
	res[1] = me->mInfo.mYres;
	res[2] = me->mInfo.mZres;

}

void piRendererGLES::GetTextureFormat( piTexture vme, Format *format )
{
	piITexture *me = (piITexture*)vme;
	format[0] = me->mInfo.mFormat;
}


void piRendererGLES::GetTextureInfo( piTexture vme, TextureInfo *info )
{
	piITexture *me = (piITexture*)vme;
	info[0] = me->mInfo;
	info->mDeleteMe = me->mObjectID;
}


void piRendererGLES::GetTextureSampling(piTexture vme, TextureFilter *rfilter, TextureWrap *rwrap)
{
	piITexture *me = (piITexture*)vme;
	rfilter[0] = me->mFilter;
	rwrap[0] = me->mWrap;
}


void piRendererGLES::GetTextureContent( piTexture vme, void *data, const Format fmt )
{
}

void piRendererGLES::GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres)
{
}

void piRendererGLES::PolygonOffset( bool mode, bool wireframe, float a, float b )
{
	if (wireframe)
	{
		return;
	}

	if( mode )
	{
		GL( glEnable( GL_POLYGON_OFFSET_FILL ) );
		GL( glPolygonOffset( a, b ) );
	}
	else
	{
		GL( glDisable( GL_POLYGON_OFFSET_FILL ) );
	}
}

void piRendererGLES::RenderMemoryBarrier(BarrierType type)
{
	GLbitfield bf = 0;

	if(static_cast<int>(type) & static_cast<int>(BarrierType::SHADER_STORAGE) ) bf |= GL_SHADER_STORAGE_BARRIER_BIT;
	if(static_cast<int>(type) & static_cast<int>(BarrierType::UNIFORM)		  ) bf |= GL_UNIFORM_BARRIER_BIT;
	if(static_cast<int>(type) & static_cast<int>(BarrierType::ATOMICS)		  ) bf |= GL_ATOMIC_COUNTER_BARRIER_BIT;
	if(static_cast<int>(type) & static_cast<int>(BarrierType::IMAGE)		  ) bf |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
	if(static_cast<int>(type) & static_cast<int>(BarrierType::COMMAND )		  ) bf |= GL_COMMAND_BARRIER_BIT;
	if(static_cast<int>(type) & static_cast<int>(BarrierType::TEXTURE)		  ) bf |= GL_TEXTURE_UPDATE_BARRIER_BIT;

	if(static_cast<int>(type) == static_cast<int>(BarrierType::ALL)) bf = GL_ALL_BARRIER_BITS;

	GL( glMemoryBarrier(bf) );
}


piQuery piRendererGLES::CreateQuery(piRenderer::QueryType type)
{
    piIQuery *me = (piIQuery*)malloc(sizeof(piIQuery));
    if (!me) return nullptr;

    me->mType = type;
    glGenQueries(1, &me->mObjectID);

    return reinterpret_cast<piQuery>(me);
}

void piRendererGLES::DestroyQuery(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    glDeleteQueries(1, &me->mObjectID);

}

void piRendererGLES::BeginQuery(piQuery vme)
{
	piIQuery *me = reinterpret_cast<piIQuery*>(vme);
	glBeginQuery(kGlQueryTypes[static_cast<int>(me->mType)], me->mObjectID);
}

void piRendererGLES::EndQuery(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    glEndQuery(kGlQueryTypes[static_cast<int>(me->mType)]);
}

uint64_t piRendererGLES::GetQueryResult(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    GLuint res = 0;
	glGetQueryObjectuiv(me->mObjectID, GL_QUERY_RESULT, &res);
    return res;
}

void piRendererGLES::CreateSyncObject(piBuffer &buffer)
{
}

bool piRendererGLES::CheckSyncObject(piBuffer &buffer)
{
    return false;
}


}
