//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include <stdio.h>

#include "piGL4X_Renderer.h"
#include "piGL4X_Ext.h"
#include "piGL4X_RenderContext.h"
#include "../../libBasics/piStr.h"
#include "../../libBasics/piTArray.h"
#include "../../libBasics/piString.h"

#define ENABLED_CPU_MIPMAPS 0

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
    unsigned int    mObjectID;
    GLuint64        mHandle;
    bool            mIsResident;
    bool            mIOwnIt;
    piRenderer::TextureInfo     mInfo;
    piRenderer::TextureFilter   mFilter;
    piRenderer::TextureWrap     mWrap;
}piITexture;

typedef struct
{
    unsigned int   mObjectID;
    piRenderer::IndexArrayFormat mIndexArrayType;
}piIVertexArray;

typedef struct
{
    unsigned int mObjectID;
    unsigned int mSize;
    piRenderer::BufferUse  mUse;
    void *mPtr;
    GLsync mSync;
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

static int unidades[32] = { GL_TEXTURE0,  GL_TEXTURE1,  GL_TEXTURE2,  GL_TEXTURE3,
                            GL_TEXTURE4,  GL_TEXTURE5,  GL_TEXTURE6,  GL_TEXTURE7,
                            GL_TEXTURE8,  GL_TEXTURE9,  GL_TEXTURE10, GL_TEXTURE11,
                            GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14, GL_TEXTURE15,
                            GL_TEXTURE16, GL_TEXTURE17, GL_TEXTURE18, GL_TEXTURE19,
                            GL_TEXTURE20, GL_TEXTURE21, GL_TEXTURE22, GL_TEXTURE23,
                            GL_TEXTURE24, GL_TEXTURE25, GL_TEXTURE26, GL_TEXTURE27,
                            GL_TEXTURE28, GL_TEXTURE29, GL_TEXTURE30, GL_TEXTURE31 };


static int format2gl( piRenderer::Format format, int *bpp, int *mode, int *moInternal, int *mode3, bool compressed )
{
    switch( format )
    {

    case piRenderer::Format::DS_24_8_UINT:        *bpp =  4; *mode = GL_DEPTH_COMPONENT;  *moInternal = GL_DEPTH_COMPONENT24;    *mode3 = GL_UNSIGNED_BYTE;  break;
    case piRenderer::Format::D1_32_FLOAT:         *bpp =  4; *mode = GL_DEPTH_COMPONENT;  *moInternal = GL_DEPTH_COMPONENT32F;   *mode3 = GL_FLOAT;          break;

    case piRenderer::Format::C1_8_UNORM:          *bpp =  1; *mode = GL_RED;              *moInternal = GL_R8;                   *mode3 = GL_UNSIGNED_BYTE;  if (compressed) *moInternal = GL_COMPRESSED_RED; break;
    case piRenderer::Format::C1_8_UINT:           *bpp =  1; *mode = GL_RED_INTEGER;      *moInternal = GL_R8UI;                 *mode3 = GL_UNSIGNED_BYTE;  break;
    case piRenderer::Format::C1_16_UINT:          *bpp =  2; *mode = GL_RED_INTEGER;      *moInternal = GL_R16UI;                *mode3 = GL_UNSIGNED_SHORT; break;
    case piRenderer::Format::C1_32_UINT:          *bpp =  4; *mode = GL_RED_INTEGER;      *moInternal = GL_R32UI;                *mode3 = GL_UNSIGNED_INT;   break;
    case piRenderer::Format::C1_16_FLOAT:         *bpp =  2; *mode = GL_RED;              *moInternal = GL_R16F;                 *mode3 = GL_FLOAT;          break;
    case piRenderer::Format::C1_32_FLOAT:         *bpp =  4; *mode = GL_RED;              *moInternal = GL_R32F;                 *mode3 = GL_FLOAT;          break;

    case piRenderer::Format::C2_16_FLOAT:         *bpp =  4; *mode = GL_RG;               *moInternal = GL_RG16F;                *mode3 = GL_FLOAT;          break;
    case piRenderer::Format::C2_8_UNORM:          *bpp =  2; *mode = GL_RG; 	          *moInternal = GL_RG8;                  *mode3 = GL_UNSIGNED_BYTE;  if (compressed) *moInternal = GL_COMPRESSED_RGB; break;

    case piRenderer::Format::C3_11_11_10_FLOAT:   *bpp =  4; *mode = GL_RGBA; 	          *moInternal = GL_R11F_G11F_B10F;       *mode3 = GL_UNSIGNED_BYTE; break;

    case piRenderer::Format::C4_8_UNORM:          *bpp =  4; *mode = GL_RGBA; 	          *moInternal = GL_RGBA8;                *mode3 = GL_UNSIGNED_BYTE;  if (compressed) *moInternal = GL_COMPRESSED_RGBA; break;
    case piRenderer::Format::C4_8_UNORM_SRGB:     *bpp =  4; *mode = GL_SRGB_ALPHA; 	  *moInternal = GL_SRGB8_ALPHA8;         *mode3 = GL_UNSIGNED_BYTE;  if (compressed) *moInternal = GL_COMPRESSED_SRGB_ALPHA; break;
    case piRenderer::Format::C4_8_UINT:	          *bpp =  4; *mode = GL_RGBA_INTEGER;	  *moInternal = GL_RGBA8UI;			  *mode3 = GL_UNSIGNED_BYTE;  break;
    case piRenderer::Format::C4_10_10_10_2_UNORM: *bpp =  4; *mode = GL_RGBA; 	          *moInternal = GL_RGB10_A2;             *mode3 = GL_UNSIGNED_BYTE; break;
    case piRenderer::Format::C4_16_FLOAT:         *bpp =  8; *mode = GL_RGBA;             *moInternal = GL_RGBA16F;              *mode3 = GL_FLOAT;          break;
    case piRenderer::Format::C4_16_SINT:          *bpp =  8; *mode = GL_RGBA_INTEGER;	  *moInternal = GL_RGBA16I;			  *mode3 = GL_SHORT;	  break;
    case piRenderer::Format::C4_32_FLOAT:         *bpp = 16; *mode = GL_RGBA;             *moInternal = GL_RGBA32F;              *mode3 = GL_FLOAT;          break;
    case piRenderer::Format::C4_32_UINT:          *bpp = 16; *mode = GL_RGBA_INTEGER;	  *moInternal = GL_RGBA32UI;			  *mode3 = GL_UNSIGNED_INT;	  break;
    case piRenderer::Format::C4_32_SINT:          *bpp = 16; *mode = GL_RGBA_INTEGER;	  *moInternal = GL_RGBA32I;			  *mode3 = GL_INT;	  break;

    case piRenderer::Format::DXT1:                *bpp = 4; *mode = GL_RGBA;	          *moInternal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;			  *mode3 = GL_UNSIGNED_BYTE;	  break;
    case piRenderer::Format::DXT5:                *bpp = 4; *mode = GL_RGBA;	          *moInternal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;			  *mode3 = GL_UNSIGNED_BYTE;	  break;


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

//static const unsigned int filter2gl[] = { GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
static const unsigned int wrap2gl[]   = { GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRROR_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };
static const unsigned int glType[]    = { GL_UNSIGNED_BYTE, GL_FLOAT, GL_INT, GL_DOUBLE, GL_HALF_FLOAT };
static const unsigned int glSizeof[]  = { 1, 4, 4, 8, 2};

//---------------------------------------------

piRendererGL4X::piRendererGL4X():piRenderer()
{
}

piRendererGL4X::~piRendererGL4X()
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
-1.0f,  1.0f, -1.0f,  -1.0f, 0.0f, 0.0f,
-1.0f,  1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,

 1.0f,  1.0f, -1.0f,   1.0f, 0.0f, 0.0f,
 1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 0.0f,
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

-1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
-1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
 1.0f,  1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
 1.0f, -1.0f, 1.0f,    0.0f, 0.0f, 1.0f,

-1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
-1.0f,  1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
 1.0f, -1.0f, -1.0f,   0.0f, 0.0f, -1.0f,
 1.0f,  1.0f, -1.0f,   0.0f, 0.0f, -1.0f };


void CALLBACK piRendererGL4X::DebugLog( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *vme )
{
    piRendererGL4X *me = (piRendererGL4X*)vme;

    if( !me->mReporter ) return;

    const char *sources = "Unknown";
    if( source==GL_DEBUG_SOURCE_API_ARB             ) sources = "API";
    if( source==GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB   ) sources = "OS";
    if( source==GL_DEBUG_SOURCE_SHADER_COMPILER_ARB ) sources = "Shader Compiler";
    if( source==GL_DEBUG_SOURCE_THIRD_PARTY_ARB     ) sources = "Third Party";
    if( source==GL_DEBUG_SOURCE_APPLICATION_ARB     ) sources = "Application";

    const char *types = "Unknown";
    if( type==GL_DEBUG_TYPE_ERROR_ARB               ) types = "Error";
    if( type==GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB ) types = "Deprecated Behavior";
    if( type==GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB  ) types = "Undefined Behavior";
    if( type==GL_DEBUG_TYPE_PORTABILITY_ARB         ) types = "Portability";
    if( type==GL_DEBUG_TYPE_PERFORMANCE_ARB         ) types = "Performance";

    int  severitiID = 0;
    const char *severities = "Unknown";
    if( severity==GL_DEBUG_SEVERITY_HIGH_ARB   ) { severitiID = 2; severities = "High"; }
    if( severity==GL_DEBUG_SEVERITY_MEDIUM_ARB ) { severitiID = 1; severities = "Medium"; }
    if( severity==GL_DEBUG_SEVERITY_LOW_ARB    ) { severitiID = 0; severities = "Low"; }

    if( severity!=GL_DEBUG_SEVERITY_HIGH_ARB ) return;

    char tmp[2048];
    pisprintf( tmp, sizeof(tmp), "Renderer Error, source = \"%s\", type = \"%s\", severity = \"%s\", description = \"%s\"", sources, types, severities, message );


    me->mReporter->Error( tmp, severitiID );
}

void piRendererGL4X::PrintInfo( int showExtensions ) //0=none, 1=inline, 2=mulitline
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
            strcat( str, (char const*)oglGetStringi(GL_EXTENSIONS, i) );
            strcat( str, " " );
        }
    }
    else if( showExtensions==2 )
    {
        for( int i=0; i<nume; i++ )
        {
            strcat( str, (char const*)oglGetStringi(GL_EXTENSIONS, i) );
            strcat( str, "\n" );
        }
    }
    mReporter->Info( str );
    free( str );
}

bool piRendererGL4X::Initialize(int id, const void **hwnd, int num, bool disableVSync, bool disableErrors, piReporter *reporter, bool createDevice, void *device)
{
    mID = id;
    mBindedTarget = nullptr;
    mBoundVertexArray = nullptr;
    mReporter = reporter;

    if (createDevice )
    {
        mRC = new piGL4X_RenderContext();
        if (!mRC)
            return false;

        if (!mRC->Create(hwnd, num, disableVSync, true, true, disableErrors))
        {
            mRC->Delete();
            return false;
        }

        mRC->Enable();
    }
    else if( device == nullptr)
    {
        mRC = new piGL4X_RenderContext();
        if (!mRC->CreateFromCurrent())
            return false;
    }
    else
    {
        //mRC = piGL4XContext::Copy(device);
        return false;
    }

    mExt = piGL4X_Ext_Init(reporter);
    if( !mExt )
        return false;

    // find useful features
    int nume = 0; glGetIntegerv(GL_NUM_EXTENSIONS, &nume);
    mFeatureVertexViewport = false;
    mFeatureViewportArray  = false;
    for (int i = 0; i<nume; i++)
    {
        if( strcmp( (const char*)oglGetStringi(GL_EXTENSIONS, i), "GL_ARB_viewport_array"             ) == 0) mFeatureViewportArray  = true;
        if( strcmp( (const char*)oglGetStringi(GL_EXTENSIONS, i), "GL_ARB_shader_viewport_layer_array") == 0) mFeatureVertexViewport = true;
    }


    int maxZMultisample, maxCMultisample, maxGSInvocations, maxTextureUnits, maxVerticesPerPatch, maxViewportDimensions;
    glGetIntegerv( GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxZMultisample );
    glGetIntegerv( GL_MAX_COLOR_TEXTURE_SAMPLES, &maxCMultisample );
    glGetIntegerv( GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &maxGSInvocations );
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits );
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxVerticesPerPatch );
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &maxViewportDimensions);


    if( reporter )
    {
    char str[256];
    sprintf( str, "Num Texture Units: %d", maxTextureUnits ); reporter->Info( str );
    sprintf( str, "Max Vertex per Patch: %d", maxVerticesPerPatch); reporter->Info(str);
    sprintf( str, "Max GS invocations: %d", maxGSInvocations); reporter->Info(str);
    sprintf(str, "Max viewport dimensions: %d", maxViewportDimensions); reporter->Info(str);
    }

    //--- texture management ---

    mMemCurrent = 0;
    mMemPeak = 0;
    mNumCurrent = 0;
    mNumPeak = 0;


    //////////////

    mVBO[0] = this->CreateBuffer(verts2f,   sizeof(verts2f),   BufferType::Static, BufferUse::Vertex);
    mVBO[1] = this->CreateBuffer(verts3f3f, sizeof(verts3f3f), BufferType::Static, BufferUse::Vertex);
    mVBO[2] = this->CreateBuffer(verts3f,   sizeof(verts3f),   BufferType::Static, BufferUse::Vertex);
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

    mCurrentPerformanceQuery = 0;
    mPerfQueries[0] = this->CreateQuery(QueryType::TimeElapsed);
    mPerfQueries[1] = this->CreateQuery(QueryType::TimeElapsed);
    if (!mPerfQueries[0] || !mPerfQueries[1])
        return false;

    // set log
    if( reporter )
    {
        oglDebugMessageCallback( DebugLog, this );
        oglDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE,GL_DONT_CARE, 0, 0, GL_TRUE );
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
    }

    glDisable(GL_DITHER);
    glDepthFunc(GL_LEQUAL);
    glHint( GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST );
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    glDisable(GL_FRAMEBUFFER_SRGB);

    mCurrentRasterState.mDepthClamp = true;
    mCurrentRasterState.mCullMode = piRenderer::CullMode::NONE;
    mCurrentRasterState.mFrontSideIsCounterClockWise = true;

    PrintInfo(1);

    return true;
}

void *piRendererGL4X::GetContext(void)
{
    return (void*)mRC;
}

bool piRendererGL4X::SupportsFeature(RendererFeature feature)
{
    if (feature == RendererFeature::VIEWPORT_ARRAY)  return mFeatureViewportArray;
    if (feature == RendererFeature::VERTEX_VIEWPORT) return mFeatureVertexViewport;
    return false;
}

piRenderer::API piRendererGL4X::GetAPI(void)
{
    return API::GL;
}

void piRendererGL4X::SetActiveWindow( int id )
{
    mRC->SetActiveWindow( id );
}

void piRendererGL4X::Enable(void)
{
    mRC->Enable();
}
void piRendererGL4X::Disable(void)
{
    mRC->Disable(false);
}


void piRendererGL4X::Report( void )
{
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


void piRendererGL4X::Deinitialize( void )
{
    //--- texture management ---

    this->DestroyQuery( mPerfQueries[0] );
    this->DestroyQuery( mPerfQueries[1] );

    this->DestroyVertexArray(mVA[0]);
    this->DestroyVertexArray(mVA[1]);
    this->DestroyVertexArray(mVA[2]);
    this->DestroyBuffer(mVBO[0]);
    this->DestroyBuffer(mVBO[1]);
    this->DestroyBuffer(mVBO[2]);

    piGL4X_Ext_Free( (NGLEXTINFO*)mExt );

    if (mRC != nullptr)
    {
        mRC->Disable(false);
        mRC->Destroy();
        mRC->Delete();
    }
    delete mRC;
}

void piRendererGL4X::SwapBuffers( void )
{
    //glFlush();
    mRC->SwapBuffers();
}

void piRendererGL4X::StartPerformanceMeasure(void)
{
    this->BeginQuery(mPerfQueries[mCurrentPerformanceQuery&1] );
}

void piRendererGL4X::EndPerformanceMeasure(void)
{
    this->EndQuery(mPerfQueries[mCurrentPerformanceQuery&1]);
}

uint64_t piRendererGL4X::GetPerformanceMeasure(void)
{
    mCurrentPerformanceQuery++; // note the increment before the fetch -> this effectively implements the ping pong between the queries
    return (mCurrentPerformanceQuery == 0) ? 0 : this->GetQueryResult(mPerfQueries[mCurrentPerformanceQuery & 1]);
}



static int check_framebuffer( NGLEXTINFO *mExt )
{
    GLenum status;
    status = oglCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch( status )
        {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            return 0;
        default:
            return 0;
        }
    if( status!=GL_FRAMEBUFFER_COMPLETE )
        return 0;
    return 1;
}


void piRendererGL4X::SetShadingSamples( int shadingSamples )
{
    piIRTarget *rt = (piIRTarget*)mBindedTarget;

    if( shadingSamples>1 && rt!=NULL )
    {
        glEnable( GL_SAMPLE_SHADING );
        oglMinSampleShading( (float)shadingSamples/(float)rt->mSamples );
    }
    else
    {
        glDisable( GL_SAMPLE_SHADING );
    }

}

piRTarget piRendererGL4X::CreateRenderTarget( piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf )
{
    const piITexture *tex[4] = { (piITexture*)vtex0, (piITexture*)vtex1, (piITexture*)vtex2, (piITexture*)vtex3 };
    const piITexture *zbu = (piITexture*)zbuf;

    piIRTarget *me = (piIRTarget*)malloc( sizeof(piIRTarget) );
    if( !me )
        return nullptr;

    me->mObjectID = 0;

    bool hasLayers = false;

    bool found = false;
    for( int i=0; i<4; i++ )
    {
        if( !tex[i] ) continue;
        me->mSamples = tex[i]->mInfo.mMultisample;
        me->mXres = tex[i]->mInfo.mXres;
        me->mYres = tex[i]->mInfo.mYres;
        //hasLayers = (tex[i]->mInfo.mType == piTEXTURE_CUBE);
        //hasLayers = (tex[i]->mInfo.mType == piTEXTURE_2D_ARRAY);
        found = true;
        break;
    }
    if( !found )
    {
        if( zbu )
        {
        me->mSamples = zbu->mInfo.mMultisample;
        me->mXres = zbu->mInfo.mXres;
        me->mYres = zbu->mInfo.mYres;
        found = true;
        }
    }

    if (!found) return nullptr;


    oglCreateFramebuffers(1, (GLuint*)&me->mObjectID);


    if( zbu )
    {
        if (hasLayers )
            oglNamedFramebufferTextureLayer(me->mObjectID, GL_DEPTH_ATTACHMENT, zbu->mObjectID, 0, 0);
        else
            oglNamedFramebufferTexture(me->mObjectID, GL_DEPTH_ATTACHMENT, zbu->mObjectID, 0);
    }
    else
    {
        if (hasLayers)
            oglNamedFramebufferTextureLayer(me->mObjectID, GL_DEPTH_ATTACHMENT, 0, 0, 0);
        else
            oglNamedFramebufferTexture(me->mObjectID, GL_DEPTH_ATTACHMENT, 0, 0);
    }

    GLenum       mMRT[4];
    int          mNumMRT = 0;
    for( int i=0; i<4; i++ )
    {
        if( tex[i] )
        {
            if (hasLayers)
                oglNamedFramebufferTextureLayer(me->mObjectID, GL_COLOR_ATTACHMENT0 + i, tex[i]->mObjectID, 0, 0);
            else
                oglNamedFramebufferTexture(me->mObjectID, GL_COLOR_ATTACHMENT0 + i, tex[i]->mObjectID, 0);
            mMRT[i] = GL_COLOR_ATTACHMENT0 + i;
            mNumMRT++;
        }
        else
        {
            if (hasLayers)
                oglNamedFramebufferTextureLayer(me->mObjectID, GL_COLOR_ATTACHMENT0 + i, 0, 0, 0);
            else
                oglNamedFramebufferTexture(me->mObjectID, GL_COLOR_ATTACHMENT0 + i, 0, 0);
            mMRT[i] = 0;
        }
    }

    oglNamedFramebufferDrawBuffers(me->mObjectID, mNumMRT, mMRT);


    GLenum st = oglCheckNamedFramebufferStatus(me->mObjectID, GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE)
        return nullptr;

    return (piRTarget)me;
}

void piRendererGL4X::DestroyRenderTarget( piRTarget obj )
{
    piIRTarget *me = (piIRTarget*)obj;

    oglDeleteFramebuffers( 1, (GLuint*)&me->mObjectID );
}

void piRendererGL4X::RenderTargetSampleLocations(piRTarget vdst, const float *locations )
{
/*
    const piIRTarget *dst = (piIRTarget*)vdst;
    if( locations==nullptr )
    {
        oglNamedFramebufferParameteri(dst->mObjectID, GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_ARB, 0);
    }
    else
    {
        oglNamedFramebufferParameteri(dst->mObjectID, GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_ARB, 1);
        oglNamedFramebufferSampleLocationsfv(dst->mObjectID, 0, dst->mSamples, locations);
    }
*/
}

void piRendererGL4X::RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location)
{
    const piIRTarget *dst = (piIRTarget*)vdst;
    oglGetMultisamplefv( GL_SAMPLE_LOCATION_ARB, id, location);
}

void piRendererGL4X::BlitRenderTarget( piRTarget vdst, piRTarget vsrc, bool color, bool depth )
{
    const piIRTarget *src = (piIRTarget*)vsrc;
    const piIRTarget *dst = (piIRTarget*)vdst;

    int flag = 0;
    if( color ) flag += GL_COLOR_BUFFER_BIT;
    if( depth ) flag += GL_DEPTH_BUFFER_BIT;

    if (dst != nullptr)
    {
        const GLenum mMRT[1] = { GL_COLOR_ATTACHMENT0 };
        oglNamedFramebufferDrawBuffers(dst->mObjectID, 1, mMRT);

        oglBlitNamedFramebuffer(src->mObjectID, dst->mObjectID,
            0, 0, dst->mXres, dst->mYres,
            0, 0, dst->mXres, dst->mYres,
            flag, GL_NEAREST);
    }
    else
    {
        oglBlitNamedFramebuffer(src->mObjectID, 0,
            0, 0, src->mXres, src->mYres,
            //0, 0, src->mXres, src->mYres,
            0, src->mYres-1, src->mXres, 0,
            flag, GL_NEAREST);
    }
}


bool piRendererGL4X::SetRenderTarget( piRTarget obj )
{
    if( obj==NULL )
    {
        mBindedTarget = NULL;
        oglBindFramebuffer( GL_FRAMEBUFFER, 0 );
        const GLenum zeros[4] = { 0, 0, 0, 0 };// { GL_NONE, GL_NONE, GL_NONE, GL_NONE };// segun la especificacion...
        oglDrawBuffers(4, zeros);
        glDrawBuffer(GL_BACK);
        //glReadBuffer(GL_BACK);
        //glDisable(GL_MULTISAMPLE);
    }
    else
    {
        piIRTarget *me = (piIRTarget*)obj;
        mBindedTarget = obj;
        oglBindFramebuffer( GL_FRAMEBUFFER, me->mObjectID );
        //glEnable(GL_FRAMEBUFFER_SRGB);
        if( me->mSamples>1 )
        {
            glEnable(GL_MULTISAMPLE);
        }
        else
        {
            glDisable(GL_MULTISAMPLE);
        }
    }

    return true;
}



void piRendererGL4X::SetViewport( int id, const int *vp )
{
    //glViewport( vp[0], vp[1], vp[2], vp[3] );
    oglViewportIndexedf(id, float(vp[0]), float(vp[1]), float(vp[2]), float(vp[3]) );

    mNumViewports = ((id+1)>=mNumViewports) ? (id+1) : mNumViewports;
    mViewports[6*id+0] = float(vp[0]);
    mViewports[6*id+1] = float(vp[1]);
    mViewports[6*id+2] = float(vp[2]);
    mViewports[6*id+3] = float(vp[3]);
    mViewports[6*id+4] = 0.0f;
    mViewports[6*id+5] = 0.0f;

}

void piRendererGL4X::SetViewports(int num, const float *viewports) // x,y,w,h,zmin,zmax // 0.0, 0.0, 800.0, 600.0, 0.0, 1.0
{
    mNumViewports = num;
    float vp[4 * 16];
    memcpy( mViewports, viewports, num*6*sizeof(float) );
    for (int i = 0; i < num; i++)
    {
        vp[4 * i + 0] = viewports[6 * i + 0];
        vp[4 * i + 1] = viewports[6 * i + 1];
        vp[4 * i + 2] = viewports[6 * i + 2];
        vp[4 * i + 3] = viewports[6 * i + 3];
    }
    oglViewportArrayv(0, num, vp);
}

void piRendererGL4X::GetViewports(int *num, float *viewports) // x,y,w,h,zmin,zmax
{
    *num = mNumViewports;
    memcpy( viewports, mViewports, mNumViewports*6*sizeof(float) );
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



piTexture piRendererGL4X::CreateTextureFromID(unsigned int id, TextureFilter filter )
{
    piITexture *me = (piITexture*)malloc(sizeof(piITexture));
    if (!me)
        return nullptr;

    me->mObjectID = id;
    me->mFilter = filter;
    me->mIOwnIt = false;
    me->mHandle = 0;
    me->mIsResident = false;


    me->mInfo.mMultisample = 0;

    int data[16];
    memset(data, 0, 16 * 4);
    oglGetTextureParameteriv(id, GL_TEXTURE_IMMUTABLE_FORMAT, data+0);
    oglGetTextureParameteriv(id, GL_TEXTURE_IMMUTABLE_LEVELS, data+1 );
    oglGetTextureParameteriv(id, GL_TEXTURE_TARGET, data + 2);
    oglGetTextureParameteriv(id, GL_TEXTURE_WRAP_S, data + 3);
    oglGetTextureParameteriv(id, GL_TEXTURE_VIEW_NUM_LEVELS, data+4 );

         if( data[2]==GL_TEXTURE_1D) me->mInfo.mType = TextureType::T1D;
    else if( data[2]==GL_TEXTURE_2D) me->mInfo.mType = TextureType::T2D;
    else if( data[2]==GL_TEXTURE_3D) me->mInfo.mType = TextureType::T3D;
    else if( data[2]==GL_TEXTURE_CUBE_MAP) me->mInfo.mType = TextureType::TCUBE;
    else if( data[2]==GL_TEXTURE_2D_ARRAY) me->mInfo.mType = TextureType::T2D_ARRAY;
    else if( data[2]==GL_TEXTURE_CUBE_MAP_ARRAY) me->mInfo.mType = TextureType::TCUBE_ARRAY;
    else if( data[2]==GL_TEXTURE_2D_MULTISAMPLE) me->mInfo.mType = TextureType::T2D;
    else if( data[2]==GL_TEXTURE_2D_MULTISAMPLE_ARRAY) me->mInfo.mType = TextureType::T2D_ARRAY;
    else return nullptr;

         if( data[3]==GL_CLAMP ) me->mWrap = TextureWrap::CLAMP;
    else if( data[3]==GL_REPEAT) me->mWrap = TextureWrap::REPEAT;
    else if( data[3]==GL_CLAMP_TO_EDGE) me->mWrap = TextureWrap::CLAMP_TO_BORDER;
    else return nullptr;

    if (data[0] == 0)
    {
        // not immutable texture
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_WIDTH,   data+0 );  // 1920
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_HEIGHT,  data+1 );  // 1080
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_DEPTH,   data+2 );  //    1
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_SAMPLES, data+3);
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_INTERNAL_FORMAT, data+4);  // 0 ????????????????
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_COMPRESSED, data + 5);
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_RED_SIZE,   data + 6);  // 8
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_GREEN_SIZE, data + 7);  // 8
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_BLUE_SIZE,  data + 8);  // 8
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_ALPHA_SIZE, data + 9);  // 8
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_RED_TYPE,   data + 10); // GL_UNSIGNED_NORMALIZED
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_GREEN_TYPE, data + 11); // GL_UNSIGNED_NORMALIZED
        oglGetTextureLevelParameteriv(id, 0, GL_TEXTURE_BLUE_TYPE,  data + 12); // GL_UNSIGNED_NORMALIZED

        me->mInfo.mXres = data[0];
        me->mInfo.mYres = data[1];
        me->mInfo.mZres = data[2];

        if( data[3]==GL_RGBA8)
        {
            me->mInfo.mFormat = Format::C4_8_UNORM;
        }
    }


    mNumCurrent++;

    return (piTexture)me;
}

#if ENABLED_CPU_MIPMAPS==1
static void iDownsample(uint8_t *dstBuffer, const int dstXres, const int dstYres, const uint8_t *srcBuffer, const int srcXres, const int srcYres )
{
    // this function will happily accept a SSE/AVX optimization pass
    for( int j=0; j<dstYres; j++ )
    for( int i=0; i<dstXres; i++ )
    {
        const int xa = srcXres*(i+0)/dstXres;
        const int ya = srcYres*(j+0)/dstYres;
        const int xb = srcXres*(i+1)/dstXres;
        const int yb = srcYres*(j+1)/dstYres;
        
        int tot[4] = { 0, 0, 0, 0 };
        for( int y=ya; y<yb; y++ )
        for( int x=xa; x<xb; x++ )
        {
            const int off = 4*(srcXres*y+x);
            tot[0] += srcBuffer[ off+0 ];
            tot[1] += srcBuffer[ off+1 ];
            tot[2] += srcBuffer[ off+2 ];
            tot[3] += srcBuffer[ off+3 ];
        }

        const int numsam = (xb-xa)*(yb-ya);
        const int numsam2 = numsam>>1;
        const int doff = 4*(dstXres*j+i);
        dstBuffer[ doff+0 ] = (numsam2+tot[0])/numsam;
        dstBuffer[ doff+1 ] = (numsam2+tot[1])/numsam;
        dstBuffer[ doff+2 ] = (numsam2+tot[2])/numsam;
        dstBuffer[ doff+3 ] = (numsam2+tot[3])/numsam;
    }
}
#endif

piTexture piRendererGL4X::CreateTexture( const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer )
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
        if (info->mMultisample>1)
        {
            oglCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &me->mObjectID);
            oglTextureStorage2DMultisample(me->mObjectID, info->mMultisample, moInternal, info->mXres, info->mYres, GL_FALSE);
        }
        else
        {
            oglCreateTextures(GL_TEXTURE_2D, 1, &me->mObjectID);
            switch (filter)
            {
            case piRenderer::TextureFilter::NONE:
            {
                if (moInternal == GL_DEPTH_COMPONENT24)
                {
                    oglTextureStorage2D(me->mObjectID, 1, GL_DEPTH_COMPONENT24, info->mXres, info->mYres);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
                }
                else
                {
                    oglTextureStorage2D(me->mObjectID, 1, moInternal, info->mXres, info->mYres);
                    if (buffer)
                    {
                        const int rowsize = info->mXres*bpp;
                        if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer);
                    }
                }
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                break;
            }
            case piRenderer::TextureFilter::LINEAR:
            {
                if (moInternal == GL_DEPTH_COMPONENT24)
                {
                    oglTextureStorage2D(me->mObjectID, 1, GL_DEPTH_COMPONENT24, info->mXres, info->mYres);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
                }
                else if( info->mFormat==piRenderer::Format::DXT1 || info->mFormat==piRenderer::Format::DXT5 )
                {
                    int blockSize = (info->mFormat==piRenderer::Format::DXT1) ? 8 : 16;
                    for (uint32_t level = 0, offset = 0, width = info->mXres, height = info->mYres; level < info->mNumMips; ++level)
                    {
                        GLsizei levelSize = (width < 4 || height < 4) ? blockSize : blockSize * (width / 4) * (height / 4);
                        oglCompressedTextureImage2D(me->mObjectID, GL_TEXTURE_2D, level, moInternal, width, height, 0, levelSize, ((uint8_t*)buffer) + offset);
                        offset += levelSize;
                        width /= 2;
                        height /= 2;
                    }
                }
                else
                {
                    oglTextureStorage2D(me->mObjectID, 1, moInternal, info->mXres, info->mYres);
                    if (buffer)
                    {
                        const int rowsize = info->mXres*bpp;
                        if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer);
                    }
                }
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                {
                    //const GLint swizzleMask[] = {GL_ALPHA, GL_RED, GL_GREEN, GL_BLUE};
                    //oglTextureParameteriv(me->mObjectID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
                }
                break;
            }
            case piRenderer::TextureFilter::PCF:
            {
                if (moInternal == GL_DEPTH_COMPONENT24)
                {
                    oglTextureStorage2D(me->mObjectID, 1, GL_DEPTH_COMPONENT24, info->mXres, info->mYres);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                }
                else
                {
                    return nullptr;
                    //oglTextureStorage2D(me->mObjectID, 1, moInternal, info->mXres, info->mYres);
                    //if (buffer) oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, mode, mode3, buffer);
                }
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                break;
            }
            case piRenderer::TextureFilter::MIPMAP:
            {
                int numMipmaps = 0;
                if( info->mFormat==piRenderer::Format::DXT1 || info->mFormat==piRenderer::Format::DXT5 )
                {
                    numMipmaps = info->mNumMips;
                    int blockSize = (info->mFormat==piRenderer::Format::DXT1) ? 8 : 16;
                    for (uint32_t level = 0, offset = 0, width = info->mXres, height = info->mYres; level < info->mNumMips; ++level)
                    {
                        GLsizei levelSize = (width < 4 || height < 4) ? blockSize : blockSize * (width / 4) * (height / 4);
                        oglCompressedTextureImage2D(me->mObjectID, GL_TEXTURE_2D, level, moInternal, width, height, 0, levelSize, ((uint8_t*)buffer) + offset);
                        offset += levelSize;
                        width /= 2;
                        height /= 2;
                    }
                }
                else
                {
                    numMipmaps = ilog2i( (info->mXres<info->mYres)?info->mXres : info->mYres);
                    oglTextureStorage2D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres);
                    bool buildGPUMipmaps = true;
                    if (buffer)
                    {
                        int xres = info->mXres;
                        int yres = info->mYres;
                        const int rowsize = xres*bpp;
                        if( (rowsize & 3) == 0 ) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        oglTextureSubImage2D(me->mObjectID, 0, 0, 0, xres, yres, moFormat, moType, buffer);

                        #if ENABLED_CPU_MIPMAPS==1
                        // Compute mimaps by hand - doing it in the GPU causes problems often due to bugs in the
                        // driver that corrupt the GL state. Also, here we downsample always from the mip 0, rather
                        // than cascading. It's slower, but produces higher quality mips.
                        if( info->mFormat==piRenderer::Format::C4_8_UNORM )
                        {
                            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                            buildGPUMipmaps = false;
                            uint8_t *tmp = (uint8_t*)malloc( xres*yres*4 );
                            if( !tmp ) return nullptr;
                            for( int i=1; i<numMipmaps; i++ )
                            {
                                xres >>= 1;
                                yres >>= 1;
                                iDownsample( tmp, xres, yres, (uint8_t*)buffer, info->mXres, info->mYres );
                                oglTextureSubImage2D(me->mObjectID, i, 0, 0, xres, yres, moFormat, moType, tmp);
                            }
                            free( tmp );
                        }
                        #endif
                    }
                    if( buildGPUMipmaps )
                        oglGenerateTextureMipmap(me->mObjectID);
                }
                
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                if (aniso>1.0001f)
                    oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, aniso);
                
                break;
            }
            case piRenderer::TextureFilter::NONE_MIPMAP:
            {
                const int numMipmaps = ilog2i(info->mXres);
                oglTextureStorage2D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres);
                if (buffer)
                {
                    const int rowsize = info->mXres*bpp;
                    if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, moFormat, moType, buffer);
                }
                oglGenerateTextureMipmap(me->mObjectID);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                if (aniso>1.0001f)
                    oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, aniso);
                break;
            }
            }
        }
    }
    else if (info->mType == piRenderer::TextureType::T3D)
    {
        oglCreateTextures(GL_TEXTURE_3D, 1, &me->mObjectID);
        if (filter != piRenderer::TextureFilter::MIPMAP)
        {
            oglTextureStorage3D(me->mObjectID, 1, moInternal, info->mXres, info->mYres, info->mZres);
            if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            const int numMipmaps = ilog2i(info->mXres);
            oglTextureStorage3D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres, info->mZres);
            if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer);
            oglGenerateTextureMipmap(me->mObjectID);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_R, wrap);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
        if (aniso>1.0001f)
            oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, aniso);
    }
    else if (info->mType == piRenderer::TextureType::TCUBE)
    {
        if (filter == piRenderer::TextureFilter::MIPMAP)
        {
            const int numMipmaps = ilog2i(info->mXres);
            oglCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &me->mObjectID);
            oglTextureStorage2D(me->mObjectID, 1, moInternal, info->mXres, info->mYres);
            if (buffer)
            {
                oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, moFormat, moType, buffer);
                oglGenerateTextureMipmap(me->mObjectID);
            }
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            oglCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &me->mObjectID);
            oglTextureStorage2D(me->mObjectID, 1, moInternal, info->mXres, info->mYres);
            if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, moFormat, moType, buffer);
        }
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, aniso);
    }
    else if (info->mType == piRenderer::TextureType::TCUBE_ARRAY)
    {
        if (filter == piRenderer::TextureFilter::MIPMAP)
        {
            const int numMipmaps = ilog2i(info->mXres);
            oglCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &me->mObjectID);
            oglTextureStorage3D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres, 6);
            if (buffer)
            {
                oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, moFormat, moType, buffer);
                oglGenerateTextureMipmap(me->mObjectID);
            }
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            oglCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &me->mObjectID);
            oglTextureStorage3D(me->mObjectID, 1, moInternal, info->mXres, info->mYres, 6);
            if (buffer)
                oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, moFormat, moType, buffer);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglTextureParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, aniso);
    }
    else if (info->mType == piRenderer::TextureType::T2D_ARRAY)
    {

        if (info->mMultisample>1)
        {
            oglCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &me->mObjectID);
            oglTextureStorage3DMultisample(me->mObjectID, info->mMultisample, moInternal, info->mXres, info->mYres, info->mZres, GL_FALSE);
        }
        else
        {
            oglCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &me->mObjectID);
            oglTextureStorage3D(me->mObjectID, 1, moInternal, info->mXres, info->mYres, info->mZres);
            if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, moFormat, moType, buffer);

            if (filter == piRenderer::TextureFilter::PCF)
            {
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
            else if (filter == piRenderer::TextureFilter::NONE)
            {
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            else
            {
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_R, wrap);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
            oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
        }

    }

    me->mHandle = oglGetTextureHandle(me->mObjectID);

    mMemCurrent += piTexture_GetMem( me );
    mNumCurrent += 1;
    if( mNumCurrent>mNumPeak ) mNumPeak = mNumCurrent;
    if( mMemCurrent>mMemPeak ) mMemPeak = mMemCurrent;

    return (piTexture)me;
}

piTexture piRendererGL4X::CreateTexture2(const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer, int bindUsage)
{
    return this->CreateTexture(key, info, compress, filter, wrap1, aniso, buffer);
}

void piRendererGL4X::ComputeMipmaps( piTexture vme )
{
    piITexture *me = (piITexture*)vme;
    if( me->mFilter!= piRenderer::TextureFilter::MIPMAP ) return;

    oglGenerateTextureMipmap(me->mObjectID);
}


void piRendererGL4X::DestroyTexture( piTexture vme )
{
    piITexture *me = (piITexture*)vme;

    if( me->mIOwnIt)
    {
        glDeleteTextures(1, &me->mObjectID);
        mMemCurrent -= piTexture_GetMem(me);
    }
    mNumCurrent--;

    free(me);
}


void piRendererGL4X::AttachTextures( int num,
                                     piTexture vt0, piTexture vt1, piTexture vt2, piTexture vt3, piTexture vt4, piTexture vt5, piTexture vt6, piTexture vt7,
                                     piTexture vt8, piTexture vt9, piTexture vt10, piTexture vt11, piTexture vt12, piTexture vt13, piTexture vt14, piTexture vt15 )
{
    piITexture *t[16] = { (piITexture*)vt0, (piITexture*)vt1, (piITexture*)vt2, (piITexture*)vt3, (piITexture*)vt4, (piITexture*)vt5, (piITexture*)vt6, (piITexture*)vt7,
                          (piITexture*)vt8, (piITexture*)vt9, (piITexture*)vt10, (piITexture*)vt11, (piITexture*)vt12, (piITexture*)vt13, (piITexture*)vt14, (piITexture*)vt15 };

    GLuint texIDs[16];
    for (int i = 0; i<num; i++)
        texIDs[i] = (t[i]) ? t[i]->mObjectID : 0;

    oglBindTextures( 0, num, texIDs );
}

void piRendererGL4X::AttachTextures( int num, piTexture *vt, int offset )
{
    GLuint texIDs[16];
    for (int i = 0; i<num; i++)
    {
        texIDs[i] = (vt[i]==nullptr) ? 0 : ((piITexture*)vt[i])->mObjectID;
    }

    oglBindTextures( offset, num, texIDs );
}


void piRendererGL4X::DettachTextures( void )
{
#if 0
    GLuint texIDs[6] = { 0, 0, 0, 0, 0, 0 };
    oglBindTextures( 0, 6, texIDs );
#endif
}

void piRendererGL4X::AttachImage(int unit, piTexture texture, int level, bool layered, int layer, Format format)
{
    int mode, moInternal, mode3, bpp;

    if (!format2gl(format, &bpp, &mode, &moInternal, &mode3, false))
        return;

    oglBindImageTexture(unit, ((piITexture*)texture)->mObjectID, level, layered, layer, GL_READ_WRITE, moInternal);
}

void piRendererGL4X::ClearTexture( piTexture vme, int level, const void *data )
{
    piITexture *me = (piITexture*)vme;

    int mode, mode2, mode3, bpp;
    if( !format2gl( me->mInfo.mFormat, &bpp, &mode, &mode2, &mode3, false ) )
        return;

    oglActiveTexture( unidades[0] );
    if( me->mInfo.mType==piRenderer::TextureType::T2D )
    {
        oglClearTexImage( me->mObjectID, level, mode, mode3, data );
    }
    else if( me->mInfo.mType== piRenderer::TextureType::T2D_ARRAY )
    {
        oglClearTexSubImage( me->mObjectID, level, 0, 0, 0, me->mInfo.mXres, me->mInfo.mYres, me->mInfo.mZres, mode, mode3, data );
    }
}

void piRendererGL4X::UpdateTexture( piTexture vme, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer )
{
    piITexture *me = (piITexture*)vme;

    int fFormat, fInternal, fType, bpp;
    if( !format2gl( me->mInfo.mFormat, &bpp, &fFormat, &fInternal, &fType, false ) )
        return;

    if( me->mInfo.mType== piRenderer::TextureType::T2D )
    {
        oglTextureSubImage2D( me->mObjectID, 0, x0, y0, xres, yres, fFormat, fType, buffer);
        if (me->mFilter == piRenderer::TextureFilter::MIPMAP)
            oglGenerateTextureMipmap(me->mObjectID);
    }
    else if( me->mInfo.mType== piRenderer::TextureType::T2D_ARRAY )
    {
        oglTextureSubImage3D( me->mObjectID, 0, x0, y0, z0, xres, yres, zres, fFormat, fType, buffer);
    }
    else if (me->mInfo.mType == piRenderer::TextureType::TCUBE)
    {
        oglTextureSubImage3D(me->mObjectID, 0, x0, y0, z0, xres, yres, zres, fFormat, fType, buffer);
        if (me->mFilter == piRenderer::TextureFilter::MIPMAP)
            oglGenerateTextureMipmap(me->mObjectID);
    }
}

void piRendererGL4X::MakeResident( piTexture vme )
{
    piITexture *me = (piITexture*)vme;
    if( me->mIsResident ) return;
    oglMakeTextureHandleResident( me->mHandle );
    me->mIsResident = true;
}
void piRendererGL4X::MakeNonResident( piTexture vme )
{
    piITexture *me = (piITexture*)vme;
    if( !me->mIsResident ) return;
    oglMakeTextureHandleNonResident( me->mHandle );
    me->mIsResident = false;
}
uint64_t piRendererGL4X::GetTextureHandle( piTexture vme )
{
    piITexture *me = (piITexture*)vme;
    return me->mHandle;
}


//==================================================


piSampler piRendererGL4X::CreateSampler(TextureFilter filter, TextureWrap wrap, float maxAnisotrop)
{
    piISampler *me = (piISampler*)malloc( sizeof(piISampler) );
    if( !me )
        return nullptr;

    oglGenSamplers( 1, &me->mObjectID );
    //oglCreateSamplers( 1, &me->mObjectID );

    int glwrap = wrap2gl[ static_cast<int>(wrap) ];

    if (filter == piRenderer::TextureFilter::NONE)
    {
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotrop);
    }
    else if (filter == piRenderer::TextureFilter::LINEAR)
    {
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotrop);
    }
    else if (filter == piRenderer::TextureFilter::MIPMAP)
    {
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotrop);
    }
    else // if (filter == piRenderer::TextureFilter::PCF)
    {
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        oglSamplerParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MIN_LOD, -1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_LOD, 1000.f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_LOD_BIAS, 0.0f);
        oglSamplerParameterf(me->mObjectID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotrop);
    }

    oglSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_R, glwrap );
    oglSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_S, glwrap );
    oglSamplerParameteri( me->mObjectID, GL_TEXTURE_WRAP_T, glwrap );

    return (piSampler)me;
}

void piRendererGL4X::DestroySampler( piSampler obj )
{
    piISampler *me = (piISampler*)obj;
    oglDeleteSamplers( 1, &me->mObjectID );
}


void piRendererGL4X::AttachSamplers(int num, piSampler vt0, piSampler vt1, piSampler vt2, piSampler vt3, piSampler vt4, piSampler vt5, piSampler vt6, piSampler vt7)
{
    piISampler *t[8] = { (piISampler*)vt0, (piISampler*)vt1, (piISampler*)vt2, (piISampler*)vt3, (piISampler*)vt4, (piISampler*)vt5, (piISampler*)vt6, (piISampler*)vt7 };

    GLuint texIDs[8];
    for( int i=0; i<num; i++ )
    {
        texIDs[i] = ( t[i] ) ? t[i]->mObjectID : 0;
    }
    oglBindSamplers( 0, num, texIDs );
}

void piRendererGL4X::DettachSamplers( void )
{
    GLuint texIDs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    oglBindSamplers( 0, 8, texIDs );
}

//===========================================================================================================================================
static const char *versionStr = "#version 450 core\n";
static const char *vsExtraStr = "#extension GL_ARB_shader_draw_parameters : enable\n#extension GL_ARB_bindless_texture : enable\n";
static const char *fsExtraStr =                                                    "#extension GL_ARB_bindless_texture : enable\n";

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
/*
piShader piRendererGL4X::CreateVertexShader(const piShaderOptions *options, const char *vs )
{
    piIShader *me = (piIShader*)malloc(sizeof(piIShader));
    if (!me)
        return nullptr;

    char optionsStr[80 * 64] = { 0 };
    if (options != nullptr)
    {
        if (!createOptionsString(optionsStr, 80 * 64, options))
            return nullptr;
    }

    const GLchar *vstrings[4] = { versionStr, vsExtraStr, optionsStr, vs };

    GLuint vertProg = oglCreateShaderProgramv(GL_VERTEX_SHADER, 4, vstrings);

    return me;
}*/


piShader piRendererGL4X::CreateShader( const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
{
    piIShader *me = (piIShader*)malloc( sizeof(piIShader) );
    if( !me )
        return nullptr;


    //glEnable( GL_DEPTH_TEST );

    const char *vtext = vs;
    const char *ctext = cs;
    const char *etext = es;
    const char *gtext = gs;
    const char *ftext = fs;

    me->mProgID = oglCreateProgram();

    const int mVertShaderID = vs?oglCreateShader( GL_VERTEX_SHADER ):-1;
    const int mCtrlShaderID = cs?oglCreateShader( GL_TESS_CONTROL_SHADER ):-1;
    const int mEvalShaderID = es?oglCreateShader( GL_TESS_EVALUATION_SHADER ):-1;
    const int mGeomShaderID = gs?oglCreateShader( GL_GEOMETRY_SHADER ):-1;
    const int mFragShaderID = fs?oglCreateShader( GL_FRAGMENT_SHADER ):-1;

    char optionsStr[80*64] = { 0 };
    if (options != nullptr)
    {
        if( !createOptionsString(optionsStr, 80*64, options) )
            return nullptr;
    }

    const GLchar *vstrings[4] = { versionStr, vsExtraStr, optionsStr, vtext };
    const GLchar *cstrings[4] = { versionStr, vsExtraStr, optionsStr, ctext };
    const GLchar *estrings[4] = { versionStr, vsExtraStr, optionsStr, etext };
    const GLchar *gstrings[4] = { versionStr, vsExtraStr, optionsStr, gtext };
    const GLchar *fstrings[4] = { versionStr, fsExtraStr, optionsStr, ftext };


    if( vs ) oglShaderSource(mVertShaderID, 4, vstrings, 0);
    if( cs ) oglShaderSource(mCtrlShaderID, 4, cstrings, 0);
    if( es ) oglShaderSource(mEvalShaderID, 4, estrings, 0);
    if( gs ) oglShaderSource(mGeomShaderID, 4, gstrings, 0);
    if( fs ) oglShaderSource(mFragShaderID, 4, fstrings, 0);


    int result = 0;

    //--------
    if( vs )
    {
        oglCompileShader( mVertShaderID );
        oglGetShaderiv( mVertShaderID, GL_COMPILE_STATUS, &result );
        if( !result )
        {
            if( error ) { error[0]='V'; error[1]='S'; error[2]=':'; oglGetShaderInfoLog( mVertShaderID, 1024, NULL, (char *)(error+3) ); }
            return nullptr;
        }
    }
    //--------
    if( cs )
    {
        oglCompileShader( mCtrlShaderID );
        oglGetShaderiv( mCtrlShaderID, GL_COMPILE_STATUS, &result );
        if( !result )
        {
            if( error ) { error[0]='C'; error[1]='S'; error[2]=':'; oglGetShaderInfoLog( mCtrlShaderID, 1024, NULL, (char *)(error+3) ); }
            return nullptr;
        }
    }
    //--------
    if( es )
    {
        oglCompileShader( mEvalShaderID );
        oglGetShaderiv( mEvalShaderID, GL_COMPILE_STATUS, &result );
        if( !result )
        {
            if( error ) { error[0]='E'; error[1]='S'; error[2]=':'; oglGetShaderInfoLog( mEvalShaderID, 1024, NULL, (char *)(error+3) ); }
            return nullptr;
        }
    }
    //--------
    if( gs )
    {
        oglCompileShader( mGeomShaderID );
        oglGetShaderiv( mGeomShaderID, GL_COMPILE_STATUS, &result );
        if( !result )
        {
            if( error ) { error[0]='G'; error[1]='S'; error[2]=':'; oglGetShaderInfoLog( mGeomShaderID, 1024, NULL, (char *)(error+3) ); }
            return nullptr;
        }
    }
    //--------
    if( fs )
    {
        oglCompileShader( mFragShaderID );
        oglGetShaderiv( mFragShaderID, GL_COMPILE_STATUS, &result );
        if( !result )
        {
            if( error ) { error[0]='F'; error[1]='S'; error[2]=':'; oglGetShaderInfoLog( mFragShaderID, 1024, NULL, (char *)(error+3) ); }
            return nullptr;
        }
    }
    //--------
    if( vs ) oglAttachShader( me->mProgID, mVertShaderID );
    if( cs ) oglAttachShader( me->mProgID, mCtrlShaderID );
    if( es ) oglAttachShader( me->mProgID, mEvalShaderID );
    if( gs ) oglAttachShader( me->mProgID, mGeomShaderID );
    if( fs ) oglAttachShader( me->mProgID, mFragShaderID );

    //--------

    oglLinkProgram( me->mProgID );
    oglGetProgramiv( me->mProgID, GL_LINK_STATUS, &result );
    if( !result )
    {
        if( error ) { error[0]='L'; error[1]='I'; error[2]=':'; oglGetProgramInfoLog( me->mProgID, 1024, NULL, (char *)(error+3) ); }
        return nullptr;
    }

    if( vs ) oglDeleteShader( mVertShaderID );
    if( cs ) oglDeleteShader( mCtrlShaderID );
    if( es ) oglDeleteShader( mEvalShaderID );
    if( gs ) oglDeleteShader( mGeomShaderID );
    if( fs ) oglDeleteShader( mFragShaderID );

    return (piShader)me;
}
/*
piShader piRendererGL4X::CreateShaderRaw(const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
{
    piIShader *me = (piIShader*)malloc(sizeof(piIShader));
    if (!me)
        return nullptr;

    //glEnable( GL_DEPTH_TEST );

    const char *vtext = vs;
    const char *ctext = cs;
    const char *etext = es;
    const char *gtext = gs;
    const char *ftext = fs;

    me->mProgID = oglCreateProgram();

    const int mVertShaderID = vs ? oglCreateShader(GL_VERTEX_SHADER) : -1;
    const int mCtrlShaderID = cs ? oglCreateShader(GL_TESS_CONTROL_SHADER) : -1;
    const int mEvalShaderID = es ? oglCreateShader(GL_TESS_EVALUATION_SHADER) : -1;
    const int mGeomShaderID = gs ? oglCreateShader(GL_GEOMETRY_SHADER) : -1;
    const int mFragShaderID = fs ? oglCreateShader(GL_FRAGMENT_SHADER) : -1;

    const GLchar *vstrings[4] = { vtext };
    const GLchar *cstrings[3] = { ctext };
    const GLchar *estrings[3] = { etext };
    const GLchar *gstrings[3] = { gtext };
    const GLchar *fstrings[4] = { ftext };

    if (vs) oglShaderSource(mVertShaderID, 1, vstrings, 0);
    if (cs) oglShaderSource(mCtrlShaderID, 1, cstrings, 0);
    if (es) oglShaderSource(mEvalShaderID, 1, estrings, 0);
    if (gs) oglShaderSource(mGeomShaderID, 1, gstrings, 0);
    if (fs) oglShaderSource(mFragShaderID, 1, fstrings, 0);


    int result = 0;

    //--------
    if (vs)
    {
        oglCompileShader(mVertShaderID);
        oglGetShaderiv(mVertShaderID, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            if (error) { error[0] = 'V'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mVertShaderID, 1024, NULL, (char *)(error + 3)); }
            return nullptr;
        }
    }
    //--------
    if (cs)
    {
        oglCompileShader(mCtrlShaderID);
        oglGetShaderiv(mCtrlShaderID, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            if (error) { error[0] = 'C'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mCtrlShaderID, 1024, NULL, (char *)(error + 3)); }
            return nullptr;
        }
    }
    //--------
    if (es)
    {
        oglCompileShader(mEvalShaderID);
        oglGetShaderiv(mEvalShaderID, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            if (error) { error[0] = 'E'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mEvalShaderID, 1024, NULL, (char *)(error + 3)); }
            return nullptr;
        }
    }
    //--------
    if (gs)
    {
        oglCompileShader(mGeomShaderID);
        oglGetShaderiv(mGeomShaderID, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            if (error) { error[0] = 'G'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mGeomShaderID, 1024, NULL, (char *)(error + 3)); }
            return nullptr;
        }
    }
    //--------
    if (fs)
    {
        oglCompileShader(mFragShaderID);
        oglGetShaderiv(mFragShaderID, GL_COMPILE_STATUS, &result);
        if (!result)
        {
            if (error) { error[0] = 'F'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mFragShaderID, 1024, NULL, (char *)(error + 3)); }
            return nullptr;
        }
    }
    //--------
    if (vs) oglAttachShader(me->mProgID, mVertShaderID);
    if (cs) oglAttachShader(me->mProgID, mCtrlShaderID);
    if (es) oglAttachShader(me->mProgID, mEvalShaderID);
    if (gs) oglAttachShader(me->mProgID, mGeomShaderID);
    if (fs) oglAttachShader(me->mProgID, mFragShaderID);

    //--------

    oglLinkProgram(me->mProgID);
    oglGetProgramiv(me->mProgID, GL_LINK_STATUS, &result);
    if (!result)
    {
        if (error) { error[0] = 'L'; error[1] = 'I'; error[2] = ':'; oglGetProgramInfoLog(me->mProgID, 1024, NULL, (char *)(error + 3)); }
        return nullptr;
    }

    if (vs) oglDeleteShader(mVertShaderID);
    if (cs) oglDeleteShader(mCtrlShaderID);
    if (es) oglDeleteShader(mEvalShaderID);
    if (gs) oglDeleteShader(mGeomShaderID);
    if (fs) oglDeleteShader(mFragShaderID);

    return (piShader)me;
}
*/

piShader piRendererGL4X::CreateShaderBinary(const piShaderOptions *options, const uint8_t *vs, const int vs_len, const uint8_t *cs, const int cs_len, const uint8_t *es, const int es_len, const uint8_t *gs, const int gs_len, const uint8_t *fs, const int fs_len, char *error)
{
    return nullptr;
}

piShader piRendererGL4X::CreateCompute(const piShaderOptions *options, const char *cs, char *error)
{
    if (!cs)
        return nullptr;

    piIShader *me = (piIShader*)malloc(sizeof(piIShader));
    if (!me)
        return nullptr;

    const char *ctext = cs;

    char optionsStr[80 * 64] = { 0 };
    if (options != nullptr) createOptionsString(optionsStr, 80*64, options);

    me->mProgID = oglCreateProgram();

    const int mShaderID = oglCreateShader(GL_COMPUTE_SHADER);

    const GLchar *vstrings[3] = { versionStr, optionsStr, ctext };

    oglShaderSource(mShaderID, 3, vstrings, 0);


    int result = 0;

    //--------
    oglCompileShader(mShaderID);
    oglGetShaderiv(mShaderID, GL_COMPILE_STATUS, &result);

    if (!result)
    {
        if (error) { error[0] = 'C'; error[1] = 'S'; error[2] = ':'; oglGetShaderInfoLog(mShaderID, 1024, NULL, (char *)(error + 3)); }
        return(0);
    }

    //--------
    oglAttachShader(me->mProgID, mShaderID);

    //--------

    oglLinkProgram(me->mProgID);
    oglGetProgramiv(me->mProgID, GL_LINK_STATUS, &result);
    if (!result)
    {
        if (error) { error[0] = 'L'; error[1] = 'I'; error[2] = ':'; oglGetProgramInfoLog(me->mProgID, 1024, NULL, (char *)(error + 3)); }
        return(0);
    }

    oglDeleteShader(mShaderID);

    return (piShader)me;
}


void piRendererGL4X::DestroyShader( piShader vme )
{
    piIShader *me = (piIShader *)vme;
    oglDeleteProgram( me->mProgID );
    free(me);
}

void piRendererGL4X::AttachShader( piShader vme )
{
    piIShader *me = (piIShader *)vme;

    //if (me->mProgID==this->mB

    oglUseProgram( me->mProgID );
    //glEnable( GL_VERTEX_PROGRAM_POINT_SIZE );
}

void piRendererGL4X::DettachShader( void )
{
    //glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    oglUseProgram( 0 );
}

void piRendererGL4X::AttachShaderConstants(piBuffer obj, int unit)
{
    piIBuffer *me = (piIBuffer *)obj;
    oglBindBufferRange(GL_UNIFORM_BUFFER, unit, me->mObjectID, 0, me->mSize);
}

void piRendererGL4X::AttachShaderBuffer(piBuffer obj, int unit)
{
    piIBuffer *me = (piIBuffer *)obj;
    oglBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, me->mObjectID );
}

void piRendererGL4X::DettachShaderBuffer(int unit)
{
    oglBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, 0);
}

void piRendererGL4X::AttachAtomicsBuffer(piBuffer obj, int unit)
{
    piIBuffer *me = (piIBuffer *)obj;
    oglBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, me->mObjectID);
}

void piRendererGL4X::DettachAtomicsBuffer(int unit)
{
    oglBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, 0);
}

void piRendererGL4X::SetShaderConstant4F(const unsigned int pos, const float *value, int num)
{
    oglUniform4fv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant3F(const unsigned int pos, const float *value, int num)
{
    oglUniform3fv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant2F(const unsigned int pos, const float *value, int num)
{
    oglUniform2fv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant1F(const unsigned int pos, const float *value, int num)
{
    oglUniform1fv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant1I(const unsigned int pos, const int *value, int num)
{
    oglUniform1iv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num)
{
    oglUniform1uiv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant2UI(const unsigned int pos, const unsigned int *value, int num)
{
    oglUniform2uiv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant3UI(const unsigned int pos, const unsigned int *value, int num)
{
    oglUniform3uiv(pos,num,value);
}
void piRendererGL4X::SetShaderConstant4UI(const unsigned int pos, const unsigned int *value, int num)
{
    oglUniform4uiv(pos,num,value);
}


void piRendererGL4X::SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose)
{
    oglUniformMatrix4fv(pos,num,transpose,value);
    //oglProgramUniformMatrix4fv( ((piIShader *)mBindedShader)->mProgID, pos, num, transpose, value ); // can do without binding!
}
void piRendererGL4X::SetShaderConstantSampler(const unsigned int pos, int unit)
{
    oglUniform1i(pos,unit);
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

void piRendererGL4X::SetBlending( int buf, BlendEquation equRGB, BlendOperations srcRGB, BlendOperations dstRGB,
                                           BlendEquation equALP, BlendOperations srcALP, BlendOperations dstALP )
{
    oglBlendEquationSeparatei(buf, r2gl_blendEqua[static_cast<int>(equRGB)], r2gl_blendEqua[static_cast<int>(equALP)]);
    oglBlendFuncSeparatei(    buf, r2gl_blendMode[static_cast<int>(srcRGB)], r2gl_blendMode[static_cast<int>(dstRGB)],
                                   r2gl_blendMode[static_cast<int>(srcALP)], r2gl_blendMode[static_cast<int>(dstALP)]);
}

void piRendererGL4X::SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z )
{
    glDepthMask( z?GL_TRUE:GL_FALSE );
    oglColorMaski( 0, c0, c0, c0, c0 );
    oglColorMaski( 1, c0, c0, c0, c0 );
    oglColorMaski( 2, c0, c0, c0, c0 );
    oglColorMaski( 3, c0, c0, c0, c0 );
}

void piRendererGL4X::SetState( piState state, bool value )
{
    if( state==piSTATE_WIREFRAME )
    {
        if( value ) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        else        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
    else if( state==piSTATE_FRONT_FACE )
    {
        if( !value ) glFrontFace( GL_CW );
        else         glFrontFace( GL_CCW );
        mCurrentRasterState.mFrontSideIsCounterClockWise = value;
    }
    else if (state == piSTATE_DEPTH_TEST)
    {
        if (value) glEnable(GL_DEPTH_TEST);
        else       glDisable(GL_DEPTH_TEST);
    }
    else if( state== piSTATE_CULL_FACE )
    {
        if( value ) glEnable(GL_CULL_FACE);
        else        glDisable(GL_CULL_FACE);
        mCurrentRasterState.mCullMode = (value == true) ? piRenderer::CullMode::FRONT : piRenderer::CullMode::NONE;
    }
    else if( state == piSTATE_ALPHA_TO_COVERAGE )
    {
        if (value)  { glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);  glEnable(GL_SAMPLE_ALPHA_TO_ONE); glDisable(GL_SAMPLE_COVERAGE ); }
        else        { glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE); glDisable(GL_SAMPLE_ALPHA_TO_ONE);  glDisable(GL_SAMPLE_COVERAGE); }
    }
    else if( state == piSTATE_DEPTH_CLAMP )
    {
        if( value ) glEnable( GL_DEPTH_CLAMP);
        else        glDisable( GL_DEPTH_CLAMP);
        mCurrentRasterState.mDepthClamp = value;
    }
    else if( state == piSTATE_VIEWPORT_FLIPY )
    {
        if( value ) oglClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);         // DX
        else        oglClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE); // GL
    }
    else if( state == piSTATE_BLEND )
    {
        if (value) glEnable(GL_BLEND);
        else       glDisable(GL_BLEND);
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

piRasterState piRendererGL4X::CreateRasterState(bool wireframe, bool frontIsCounterClockWise, CullMode cullMode, bool depthClamp, bool multiSample)
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

void piRendererGL4X::SetRasterState(const piRasterState vme)
{
    piIRasterState *me = (piIRasterState*)vme;

    if (me->mDesc.depthClamp != mCurrentRasterState.mDepthClamp)
    {
        if (me->mDesc.depthClamp) glEnable(GL_DEPTH_CLAMP);  else glDisable(GL_DEPTH_CLAMP);
        mCurrentRasterState.mDepthClamp = me->mDesc.depthClamp;
    }

    if (me->mDesc.cullMode != mCurrentRasterState.mCullMode)
    {
               if (me->mDesc.cullMode == piRenderer::CullMode::NONE)   { glDisable(GL_CULL_FACE); }
        else   if (me->mDesc.cullMode == piRenderer::CullMode::FRONT)  { glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); }
        else /*if (me->mDesc.cullMode == piRenderer::CullMode::BACK)*/ { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); }

        mCurrentRasterState.mCullMode = me->mDesc.cullMode;
    }

    if (me->mDesc.mFrontSideIsCounterClockWise != mCurrentRasterState.mFrontSideIsCounterClockWise)
    {
        if (me->mDesc.mFrontSideIsCounterClockWise) glFrontFace(GL_CCW); else glFrontFace(GL_CW);
        mCurrentRasterState.mFrontSideIsCounterClockWise = me->mDesc.mFrontSideIsCounterClockWise;
    }
}

void piRendererGL4X::DestroyRasterState(piRasterState vme)
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

piBlendState piRendererGL4X::CreateBlendState(bool alphaToCoverage, bool enabled0)
{
    piIBlendState *me = (piIBlendState*)malloc(sizeof(piIBlendState));
    if (!me)
        return nullptr;

    me->mDesc.mAlphaToCoverage = alphaToCoverage;
    me->mDesc.mIndependentBlendEnable = false;
    me->mDesc.mRenderTarget[0].mBlendEnabled = enabled0;

    return (piBlendState)me;
}

void piRendererGL4X::SetBlendState(const piBlendState vme)
{
    piIBlendState *me = (piIBlendState*)vme;

    if (me->mDesc.mAlphaToCoverage)
    {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glEnable(GL_SAMPLE_ALPHA_TO_ONE);
        glDisable(GL_SAMPLE_COVERAGE);
    }
    else
    {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glDisable(GL_SAMPLE_ALPHA_TO_ONE);
        glDisable(GL_SAMPLE_COVERAGE);
    }
}

void piRendererGL4X::DestroyBlendState(piBlendState vme)
{
    piIBlendState *me = (piIBlendState*)vme;
    free(me);
}

//-------------------------
struct piIDepthState
{
    int dummy;
};
piDepthState piRendererGL4X::CreateDepthState(bool depthEnable, bool enabled0)
{
    piIDepthState *me = (piIDepthState*)malloc(sizeof(piIDepthState));
    if (!me)
        return nullptr;

    return (piDepthState)me;
}

void piRendererGL4X::SetDepthState(const piDepthState vme)
{
    piIDepthState *me = (piIDepthState*)vme;
}

void piRendererGL4X::DestroyDepthState(piDepthState vme)
{
    piIDepthState *me = (piIDepthState*)vme;
    free(me);
}

//-------------------------
void piRendererGL4X::Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 )
{
    if( mBindedTarget == NULL )
    {
        int mode = 0;
        if( color0 ) { mode |= GL_COLOR_BUFFER_BIT;   glClearColor( color0[0], color0[1], color0[2], color0[3] ); }
        if( depth0 ) { mode |= GL_DEPTH_BUFFER_BIT;   glClearDepth( 1.0f ); }
        glClear( mode );
    }
    else
    {
        float z = 1.0f;
        if( color0 ) oglClearBufferfv( GL_COLOR, 0, color0 );
        if( color1 ) oglClearBufferfv( GL_COLOR, 1, color1 );
        if( color2 ) oglClearBufferfv( GL_COLOR, 2, color2 );
        if( color3 ) oglClearBufferfv( GL_COLOR, 3, color3 );
        if( depth0 ) oglClearBufferfv( GL_DEPTH, 0, &z );
    }
//    glClearBufferfi( GL_DEPTH_STENCIL, 0, z, s );
}

//-----------------------

piBuffer piRendererGL4X::CreateBuffer(const void *data, unsigned int amount, BufferType mode, BufferUse use)
{
    piIBuffer *me = (piIBuffer*)malloc(sizeof(piIBuffer));
    if (!me)
        return nullptr;

    oglCreateBuffers(1, &me->mObjectID);

    if (mode == BufferType::Dynamic)
    {
        oglNamedBufferStorage(me->mObjectID, amount, data, GL_DYNAMIC_STORAGE_BIT);


        //GL_MAP_UNSYNCHRONIZED_BIT|GL_MAP_WRITE_BIT


        //GL_DYNAMIC_STORAGE_BIT   // can call glBufferSubData to copy data after initialzation, from the client. Without it, only the GPU can update the buffer wutg glCopyBufferSubData, glClearBufferSubData or shaders
        //GL_MAP_READ_BIT          // the CPU can query a pointer for reading
        //GL_MAP_WRITE_BIT         // the CPU can query a pointer for writing

        //GL_MAP_PERSISTENT_BIT|GL_MAP_READ_BIT						// to request CPU and GPU cna operate simultaenously on the buffer
        //GL_MAP_PERSISTENT_BIT|GL_MAP_WRITE_BIT					// to request CPU and GPU cna operate simultaenously on the buffer
        //GL_MAP_PERSISTENT_BIT|GL_MAP_READ_BIT|GL_MAP_WRITE_BIT    // to request CPU and GPU cna operate simultaenously on the buffer

        // add GL_MAP_COHERENT_BIT to those to make the DMA move data from CPU to GPU automatically while the buffers are mapped in the CPU

    }
    else
    {
        oglNamedBufferStorage(me->mObjectID, amount, data, 0);
    }
    me->mSize = amount;
    me->mUse = use;
    me->mPtr = nullptr;
    me->mSync = 0;

    mMemCurrent += amount;
    mNumCurrent += 1;
    if (mNumCurrent>mNumPeak) mNumPeak = mNumCurrent;
    if (mMemCurrent>mMemPeak) mMemPeak = mMemCurrent;

    return (piBuffer)me;
}

piBuffer piRendererGL4X::CreateBufferMapped_Start(void **ptr, unsigned int amount, BufferUse use )
{
    piIBuffer *me = (piIBuffer*)malloc(sizeof(piIBuffer));
    if (!me)
        return nullptr;

    oglCreateBuffers(1, &me->mObjectID);

    oglNamedBufferStorage(me->mObjectID, amount, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    me->mPtr = oglMapNamedBufferRange(me->mObjectID, 0, amount, GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
    if (me->mPtr == nullptr )
        return 0;
    me->mSync = oglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    me->mSize = amount;
    me->mUse = use;

    *ptr = me->mPtr;

    mMemCurrent += amount;
    mNumCurrent += 1;
    if (mNumCurrent>mNumPeak) mNumPeak = mNumCurrent;
    if (mMemCurrent>mMemPeak) mMemPeak = mMemCurrent;

    return (piBuffer)me;
}

void piRendererGL4X::CreateBufferMapped_End(piBuffer vme)
{
    piIBuffer *me = (piIBuffer*)vme;
    oglUnmapNamedBuffer(me->mObjectID);
}

void piRendererGL4X::DestroyBuffer(piBuffer vme)
{
    piIBuffer *me = (piIBuffer*)vme;
    oglDeleteBuffers(1, &me->mObjectID );
    mMemCurrent -= me->mSize;
    mNumCurrent -= 1;
    free(me);
}

piBuffer piRendererGL4X::CreateStructuredBuffer(const void *data, unsigned int numElements, unsigned int elementSize, BufferType mode, BufferUse use)
{
    return CreateBuffer(data, numElements*elementSize, mode, use);
}

void piRendererGL4X::UpdateBuffer(piBuffer obj, const void *data, int offset, int len, bool invalidate)
{
    piIBuffer *me = (piIBuffer *)obj;
    oglNamedBufferSubData(me->mObjectID, offset, len, data);

    if( invalidate )
        oglInvalidateBufferData(me->mObjectID);

    /*
    while(1)
    {
        GLenum waitReturn = oglClientWaitSync(me->mSync, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED)
            break;
    }
    memcpy(me->mPtr, data, len);
*/
    //void *ptr = oglMapNamedBufferRange(me->mObjectID, 0, len, GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    //memcpy(ptr, data, len);
    //oglUnmapNamedBuffer( me->mObjectID );
}



piVertexArray piRendererGL4X::CreateVertexArray( int numStreams,
                                                 piBuffer vb0, const piRArrayLayout *streamLayout0,
                                                 piBuffer vb1, const piRArrayLayout *streamLayout1,
                                                 piBuffer eb, const IndexArrayFormat ebFormat )
{
    piIVertexArray *me = (piIVertexArray*)malloc(sizeof(piIVertexArray));
    if( !me )
        return nullptr;

    oglCreateVertexArrays(1, &me->mObjectID);
    if (!me->mObjectID)
        return nullptr;

    unsigned int aid = 0;

    if (numStreams > 0)
    {
        for (int j = 0; j < numStreams; j++)
        {
            unsigned int sid = j;

            //me->mStreams[j] = (j == 0) *streamLayout0 : *streamLayout1;

            const piRArrayLayout * st = (j == 0) ? streamLayout0 : streamLayout1;
            piBuffer vb = (j == 0) ? vb0 : vb1;

            int offset = 0;
            const int num = st->mNumElements;
            for (int i = 0; i < num; i++)
            {
                oglEnableVertexArrayAttrib(me->mObjectID, aid);
                oglVertexArrayAttribFormat(me->mObjectID, aid, st->mEntry[i].mNumComponents, glType[st->mEntry[i].mType], st->mEntry[i].mNormalize, offset);
                oglVertexArrayAttribBinding(me->mObjectID, aid, sid);
                offset += st->mEntry[i].mNumComponents*glSizeof[st->mEntry[i].mType];
                aid++;
            }
            oglVertexArrayVertexBuffer(me->mObjectID, sid, ((piIBuffer*)vb)->mObjectID, 0, st->mStride);
            oglVertexArrayBindingDivisor(me->mObjectID, sid, st->mDivisor);
        }
    }
    else
    {
        //oglDisableVertexArrayAttrib(me->mObjectID, aid);
        //oglVertexArrayVertexBuffer(me->mObjectID, 0, 0, 0, 0);
    }

    if (eb != nullptr)
    {

        me->mIndexArrayType = ebFormat;
        oglVertexArrayElementBuffer(me->mObjectID, ((piIBuffer*)eb)->mObjectID);
    }

    return (piVertexArray)me;
}

void piRendererGL4X::DestroyVertexArray(piVertexArray vme)
{
    piIVertexArray *me = (piIVertexArray*)vme;
    oglDeleteVertexArrays(1, &me->mObjectID);
}

void piRendererGL4X::AttachVertexArray(piVertexArray vme)
{
    piIVertexArray *me = (piIVertexArray*)vme;
    oglBindVertexArray(me->mObjectID);
    mBoundVertexArray = vme;
}

void piRendererGL4X::DettachVertexArray( void )
{
    oglBindVertexArray( 0 );
}

piVertexArray piRendererGL4X::CreateVertexArray2(int numStreams, piBuffer vb0, const ArrayLayout2 *streamLayout0, piBuffer vb1, const ArrayLayout2 *streamLayout1, const void *shaderBinary, size_t shaderBinarySize, piBuffer ib, const IndexArrayFormat ebFormat)
{
    return nullptr;
}
void piRendererGL4X::AttachVertexArray2(piVertexArray vme)
{
}

void piRendererGL4X::DestroyVertexArray2(piVertexArray vme)
{
}

void piRendererGL4X::AttachPixelPackBuffer(piBuffer obj)
{
    piIBuffer *me = (piIBuffer*)obj;
    oglBindBuffer(GL_PIXEL_PACK_BUFFER, me->mObjectID);
}

void piRendererGL4X::DettachPixelPackBuffer(void)
{
    oglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}


GLenum piRendererGL4X::iPrimtive_pi2gl(PrimitiveType pt)
{
    GLenum glpt = GL_TRIANGLES;
         if (pt == PrimitiveType::Triangle)        glpt = GL_TRIANGLES;
    else if (pt == PrimitiveType::Point)           glpt = GL_POINTS;
    else if (pt == PrimitiveType::TriangleStrip)   glpt = GL_TRIANGLE_STRIP;
    else if (pt == PrimitiveType::LineStrip)       glpt = GL_LINE_STRIP;
    else if (pt == PrimitiveType::TriPatch)      { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 3); }
    else if (pt == PrimitiveType::QuadPatch)     { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 4); }
    else if (pt == PrimitiveType::LinesAdj)        glpt = GL_LINES_ADJACENCY;
    else if (pt == PrimitiveType::LineStripAdj)    glpt = GL_LINE_STRIP_ADJACENCY;
    else if (pt == PrimitiveType::Patch16)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 16); }
    else if (pt == PrimitiveType::Patch32)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 32); }
    else if (pt == PrimitiveType::Lines)           glpt = GL_LINES;
    return glpt;
}



void piRendererGL4X::DrawPrimitiveIndexed(PrimitiveType pt, uint32_t num, uint32_t numInstances, uint32_t baseVertex, uint32_t baseInstance, uint32_t baseIndex)
{
    GLenum glpt = iPrimtive_pi2gl(pt);
    piIVertexArray *va = (piIVertexArray*)mBoundVertexArray;

#if 0
    int vertexarraybound, elementarraybound, elementarraybuffersize, maxVertexAttributes, maxVaryingVectors, vaabb;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexarraybound);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttributes);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVectors);
    oglGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &elementarraybuffersize);

    struct attrib {
        int isOn;
        int binding;
        int bindingBuffer;
        int divisor;
        int numComponents;
    } atts[16];

    for (unsigned i = 0; i < 16; ++i)
    {
        oglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &atts[i].isOn);
        if (atts[i].isOn)
        {
            oglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &atts[i].bindingBuffer);
            oglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_BINDING, &atts[i].binding);
            oglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &atts[i].numComponents);
            oglGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &atts[i].divisor);
        }
    }
#endif
    oglDrawElementsInstancedBaseVertexBaseInstance( glpt, num, (va->mIndexArrayType==IndexArrayFormat::UINT_32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
                (void*)(uint64_t)(baseIndex*((va->mIndexArrayType == IndexArrayFormat::UINT_32)?4:2)),   // indices
                numInstances,      // prim count
                baseVertex,      // base vertex
                baseInstance);     // base instance
}

void piRendererGL4X::DrawPrimitiveNotIndexed(PrimitiveType pt, int first, int num, int numInstanced)
{
    GLenum glpt = iPrimtive_pi2gl(pt);
    oglDrawArraysInstanced(glpt, first, num, numInstanced);
}


void piRendererGL4X::DrawPrimitiveNotIndexedMultiple(PrimitiveType pt, const int *firsts, const int *counts, int num)
{
    GLenum glpt = iPrimtive_pi2gl(pt);
    oglMultiDrawArrays(glpt,firsts,counts,num);
}


void piRendererGL4X::DrawPrimitiveNotIndexedIndirect(PrimitiveType pt, piBuffer cmds, int num)
{
    piIBuffer *buf = (piIBuffer *)cmds;

    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf->mObjectID);

    GLenum glpt = iPrimtive_pi2gl(pt);

    oglMultiDrawArraysIndirect(glpt, 0, num, sizeof(piDrawArraysIndirectCommand));
    //oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void piRendererGL4X::DettachIndirectBuffer(void)
{
    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void piRendererGL4X::DrawPrimitiveIndirect(PrimitiveType pt, piBuffer cmds, uint32_t offset, uint32_t num)
{
    piIBuffer *buf = (piIBuffer *)cmds;

    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf->mObjectID);

    GLenum glpt = iPrimtive_pi2gl(pt);

    piIVertexArray *va = (piIVertexArray*)mBoundVertexArray;
    oglMultiDrawElementsIndirect(glpt, (va->mIndexArrayType == IndexArrayFormat::UINT_32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, reinterpret_cast<void*>(static_cast<uint64_t>(offset)), num, sizeof(piDrawElementsIndirectCommand));
    //oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void piRendererGL4X::DrawUnitQuad_XY( int numInstanced )
{
    this->AttachVertexArray( mVA[0] );
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced);
    this->DettachVertexArray();
}

void piRendererGL4X::DrawUnitCube_XYZ_NOR(int numInstanced)
{
    this->AttachVertexArray(mVA[1]);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 4, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 8, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 12, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 16, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 20, 4, numInstanced);
    this->DettachVertexArray();
}

void piRendererGL4X::DrawUnitCube_XYZ(int numInstanced)
{
    this->AttachVertexArray(mVA[2]);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 4, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 8, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 12, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 16, 4, numInstanced);
    oglDrawArraysInstanced(GL_TRIANGLE_STRIP, 20, 4, numInstanced);
    this->DettachVertexArray();
}

void piRendererGL4X::ExecuteCompute(int tx, int ty, int tz, int gsx, int gsy, int gsz)
{
    int ngx = tx / gsx; if( (ngx*gsx) < tx ) ngx++;
    int ngy = ty / gsy; if( (ngy*gsy) < ty ) ngy++;
    int ngz = tz / gsz; if( (ngz*gsz) < tz ) ngz++;

    oglDispatchCompute( ngx, ngy, ngz );
}

void piRendererGL4X::SetLineWidth( float size )
{
    glLineWidth( size );
}

void piRendererGL4X::SetPointSize( bool mode, float size )
{
    if( mode )
    {
        glEnable( GL_PROGRAM_POINT_SIZE );
        glPointSize( size );
    }
    else
        glDisable( GL_PROGRAM_POINT_SIZE );

}

void piRendererGL4X::GetTextureRes( piTexture vme, int *res )
{
    piITexture *me = (piITexture*)vme;
    res[0] = me->mInfo.mXres;
    res[1] = me->mInfo.mYres;
    res[2] = me->mInfo.mZres;

}

void piRendererGL4X::GetTextureFormat( piTexture vme, Format *format )
{
    piITexture *me = (piITexture*)vme;
    format[0] = me->mInfo.mFormat;
}


void piRendererGL4X::GetTextureInfo( piTexture vme, TextureInfo *info )
{
    piITexture *me = (piITexture*)vme;
    info[0] = me->mInfo;
    info->mDeleteMe = me->mObjectID;
}


void piRendererGL4X::GetTextureSampling(piTexture vme, TextureFilter *rfilter, TextureWrap *rwrap)
{
    piITexture *me = (piITexture*)vme;
    rfilter[0] = me->mFilter;
    rwrap[0] = me->mWrap;
}


void piRendererGL4X::GetTextureContent( piTexture vme, void *data, const Format fmt )
{
    piITexture *me = (piITexture*)vme;
    int     	 mode, mode2, mode3, bpp;

    //if( !format2gl( me->mInfo.mFormat, &bpp, &mode, &mode2, &mode3, me->mInfo.mCompressed ) )
    if( !format2gl( fmt, &bpp, &mode, &mode2, &mode3, false ) )
        return;

    //glPixelStorei(GL_UNPACK_SWAP_BYTES, true);

    oglGetTextureImage(me->mObjectID, 0, mode, mode3, me->mInfo.mXres*me->mInfo.mYres*me->mInfo.mZres * bpp, data);
}

void piRendererGL4X::GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres)
{
    piITexture *me = (piITexture*)vme;
    int     	 exteriorFormat, internalFormat, ftype, bpp;

    if (!format2gl(me->mInfo.mFormat, &bpp, &exteriorFormat, &internalFormat, &ftype, false))
        return;

    oglGetTextureSubImage( me->mObjectID,
                           0,
                           x, y, z, xres, yres, zres,
                           exteriorFormat, ftype,
                           xres*yres*zres*bpp, data );
}

//------------

/*
void piRendererGL4X::SetAttribute1F( int pos, const float data )
{
    oglVertexAttrib1f( pos, data );
}

void piRendererGL4X::SetAttribute2F( int pos, const float *data )
{
    oglVertexAttrib2fv( pos, data );
}

void piRendererGL4X::SetAttribute3F( int pos, const float *data )
{
    oglVertexAttrib3fv( pos, data );
}

void piRendererGL4X::SetAttribute4F( int pos, const float *data )
{
    oglVertexAttrib4fv( pos, data );
}
*/

void piRendererGL4X::PolygonOffset( bool mode, bool wireframe, float a, float b )
{
    if( mode )
    {
        glEnable( wireframe?GL_POLYGON_OFFSET_LINE:GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( a, b );
    }
    else
    {
        glDisable( wireframe?GL_POLYGON_OFFSET_LINE:GL_POLYGON_OFFSET_FILL );
    }
}

void piRendererGL4X::RenderMemoryBarrier(BarrierType type)
{
    GLbitfield bf = 0;

    if(static_cast<int>(type) & static_cast<int>(BarrierType::SHADER_STORAGE) ) bf |= GL_SHADER_STORAGE_BARRIER_BIT;
    if(static_cast<int>(type) & static_cast<int>(BarrierType::UNIFORM)        ) bf |= GL_UNIFORM_BARRIER_BIT;
    if(static_cast<int>(type) & static_cast<int>(BarrierType::ATOMICS)        ) bf |= GL_ATOMIC_COUNTER_BARRIER_BIT;
    if(static_cast<int>(type) & static_cast<int>(BarrierType::IMAGE)          ) bf |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    if(static_cast<int>(type) & static_cast<int>(BarrierType::COMMAND )       ) bf |= GL_COMMAND_BARRIER_BIT;
    if(static_cast<int>(type) & static_cast<int>(BarrierType::TEXTURE)        ) bf |= GL_TEXTURE_UPDATE_BARRIER_BIT;

    if(static_cast<int>(type) == static_cast<int>(BarrierType::ALL)) bf = GL_ALL_BARRIER_BITS;

    oglMemoryBarrier(bf);
}

piQuery piRendererGL4X::CreateQuery(piRenderer::QueryType type)
{
    piIQuery *me = (piIQuery*)malloc(sizeof(piIQuery));
    if (!me) return nullptr;

    me->mType = type;
    oglGenQueries(1, &me->mObjectID);

    return reinterpret_cast<piQuery>(me);
}

void piRendererGL4X::DestroyQuery(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    oglDeleteQueries(1, &me->mObjectID);

}

static GLenum kGlQueryTypes[] = { GL_TIME_ELAPSED , GL_TIMESTAMP, GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED };

void piRendererGL4X::BeginQuery(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);

    oglBeginQuery(kGlQueryTypes[static_cast<int>(me->mType)], me->mObjectID );
}

void piRendererGL4X::EndQuery(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    oglEndQuery(kGlQueryTypes[static_cast<int>(me->mType)]);
}

uint64_t piRendererGL4X::GetQueryResult(piQuery vme)
{
    piIQuery *me = reinterpret_cast<piIQuery*>(vme);
    uint64_t res = 0;
    oglGetQueryObjectui64v(me->mObjectID, GL_QUERY_RESULT, &res);
    return res;
}

void piRendererGL4X::CreateSyncObject(piBuffer &buffer)
{
    piIBuffer* me = (piIBuffer*)buffer;
    me->mSync = oglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool piRendererGL4X::CheckSyncObject(piBuffer &buffer)
{
    piIBuffer* me = (piIBuffer*)buffer;
    int result = oglClientWaitSync(me->mSync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000);
    if (result == GL_WAIT_FAILED || result == GL_TIMEOUT_EXPIRED)
    {
        return false;
    }
    return true;
}

}
