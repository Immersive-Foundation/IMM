#include <d3d11.h>

#include <malloc.h>
#include <stdio.h>


#include "piDX11_Renderer.h"
#include "../../libBasics/piStr.h"
#include "../../libBasics/piDebug.h"

namespace ImmCore {

	typedef struct
	{
		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;
	}piIShader;

	typedef struct
	{
		D3D11_RASTERIZER_DESC mDesc;
		ID3D11RasterizerState* mObject;
	}piIRasterState;

	typedef struct
	{
		D3D11_BLEND_DESC mDesc;
		ID3D11BlendState* mObject;
	}piIBlendState;

	typedef struct
	{
		D3D11_DEPTH_STENCIL_DESC mDesc;
		ID3D11DepthStencilState* mObject;
	}piIDepthState;

	typedef struct
	{
		ID3D11Buffer *mObject;
		unsigned int  mSize;
		piRenderer::BufferUse     mUse;

		ID3D11ShaderResourceView *mView; // for resource buffers only

	}piIBuffer;

	typedef struct
	{
		ID3D11InputLayout* mInputLayout;
		int            mNumStreams;
		int            mStride[16];
		piBuffer       mBuffer[16];
		piBuffer       mIndexBuffer;
		piRenderer::IndexArrayFormat mIndexArrayType;
	}piIVertexArray2;


	typedef struct
	{
		ID3D11Texture2D *mObjectID;
		ID3D11ShaderResourceView* mViewShader;
        ID3D11RenderTargetView  * mViewRenderTarget;
        ID3D11DepthStencilView  * mViewDepthStencil;

		//ID3D11SamplerState* CubesTexSamplerState;
		DXGI_FORMAT mDXFormat; // tmp, remove later
		//unsigned int    mObjectID;
		//GLuint64        mHandle;
		//bool            mIsResident;
		piRenderer::TextureInfo   mInfo;
		piRenderer::TextureFilter mFilter;
		piRenderer::TextureWrap   mWrap;
	}piITexture;
	/*
	typedef struct
	{
		wchar_t     *mKey;
		piITexture  *mTexture;
		int         mReference;
	}TextureSlot;
	*/


	typedef struct
	{
		ID3D11SamplerState * mObjectID;
	}piISampler;

	typedef struct
	{
        piITexture *mColor[4];
        piITexture *mDepth;
        //ID3D11RenderTargetView* mRenderTargetView0;

        //ID3D11DepthStencilView* mRenderTargetViewZ;

		//unsigned int mObjectID;
		//unsigned int mSamples;
		//unsigned int mXres;
		//unsigned int mYres;
	}piIRTarget;

    //---------------------------------------------

    static inline DXGI_FORMAT iDxFormatEnum(const piRenderer::Format f)
    {
        if (f == piRenderer::Format::C3_11_11_10_FLOAT) return DXGI_FORMAT_R11G11B10_FLOAT;

        if (f == piRenderer::Format::C4_8_UNORM)        return DXGI_FORMAT_R8G8B8A8_UNORM;
        if (f == piRenderer::Format::C4_8_UINT)         return DXGI_FORMAT_R8G8B8A8_UINT;
        if (f == piRenderer::Format::C4_8_UNORM_SRGB)   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        if (f == piRenderer::Format::D1_32_FLOAT)       return DXGI_FORMAT_D32_FLOAT;
        if (f == piRenderer::Format::DS_24_8_UINT)      return DXGI_FORMAT_D24_UNORM_S8_UINT;

        if (f == piRenderer::Format::C1_32_FLOAT)       return DXGI_FORMAT_R32_FLOAT;
        if (f == piRenderer::Format::C2_32_FLOAT)       return DXGI_FORMAT_R32G32_FLOAT;
        if (f == piRenderer::Format::C3_32_FLOAT)       return DXGI_FORMAT_R32G32B32_FLOAT;
        if (f == piRenderer::Format::C4_32_FLOAT)       return DXGI_FORMAT_R32G32B32A32_FLOAT;

        if (f == piRenderer::Format::C1_16_FLOAT)       return DXGI_FORMAT_R16_FLOAT;
        if (f == piRenderer::Format::C2_16_FLOAT)       return DXGI_FORMAT_R16G16_FLOAT;
      //if (f == piRenderer::Format::C3_16_FLOAT)       return DXGI_FORMAT_R16G16B16_FLOAT;
        if (f == piRenderer::Format::C4_16_FLOAT)       return DXGI_FORMAT_R16G16B16A16_FLOAT;
        if (f == piRenderer::Format::C1_8_UNORM)        return DXGI_FORMAT_R8_UNORM;

        piAssert(false);
        return DXGI_FORMAT_UNKNOWN;
    }
    static inline int iDxFormatSize(const piRenderer::Format f)
    {
        if (f == piRenderer::Format::C3_11_11_10_FLOAT) return 4;

        if (f == piRenderer::Format::C4_8_UNORM)        return 4;
        if (f == piRenderer::Format::C4_8_UINT)         return 4;
        if (f == piRenderer::Format::C4_8_UNORM_SRGB)   return 4;

        if (f == piRenderer::Format::D1_32_FLOAT)       return 4;

        if (f == piRenderer::Format::C1_32_FLOAT)       return 4;
        if (f == piRenderer::Format::C2_32_FLOAT)       return 8;
        if (f == piRenderer::Format::C3_32_FLOAT)       return 12;
        if (f == piRenderer::Format::C4_32_FLOAT)       return 16;

        if (f == piRenderer::Format::C1_16_FLOAT)       return 2;
        if (f == piRenderer::Format::C2_16_FLOAT)       return 4;
        //if (f == piRenderer::Format::C3_16_FLOAT)       return DXGI_FORMAT_R16G16B16_FLOAT;
        if (f == piRenderer::Format::C4_16_FLOAT)       return 8;


        piAssert(false);
        return 0;
    }


    /*
static struct iFormatInfo
{
	DXGI_FORMAT format;
	int         size;
}pi_to_dx_lut[] = {
	DXGI_FORMAT_R32G32B32A32_FLOAT,	 16,
	DXGI_FORMAT_R32G32B32A32_UINT, 	 16,
	DXGI_FORMAT_R32G32B32A32_SINT, 	 16,
	DXGI_FORMAT_R16G16B16A16_FLOAT,  8,
	DXGI_FORMAT_R16G16B16A16_UNORM,  8,
	DXGI_FORMAT_R16G16B16A16_UINT,   8,
	DXGI_FORMAT_R16G16B16A16_SNORM,  8,
	DXGI_FORMAT_R16G16B16A16_SINT,   8,
	DXGI_FORMAT_R8G8B8A8_UNORM,	     4,     // C4_8_UNORM = 8
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 4,
	DXGI_FORMAT_R8G8B8A8_UINT, 	     4,
	DXGI_FORMAT_R8G8B8A8_SNORM,      4,
	DXGI_FORMAT_R8G8B8A8_SINT,	     4,

	DXGI_FORMAT_R32G32B32_FLOAT,     12,     // C3_32_FLOAT = 13
	DXGI_FORMAT_R32G32B32_UINT,      12,
	DXGI_FORMAT_R32G32B32_SINT,      12,

	DXGI_FORMAT_R32G32_FLOAT, 8,
	DXGI_FORMAT_R32G32_UINT,  8,
	DXGI_FORMAT_R32G32_SINT,  8,
	DXGI_FORMAT_R16G16_FLOAT, 4,
	DXGI_FORMAT_R16G16_UNORM, 4, // 20
	DXGI_FORMAT_R16G16_UINT,  4,
	DXGI_FORMAT_R16G16_SNORM,  4,
	DXGI_FORMAT_R16G16_SINT, 4,
	DXGI_FORMAT_R8G8_UNORM, 2,
	DXGI_FORMAT_R8G8_UINT,  2,
	DXGI_FORMAT_R8G8_SNORM,  2,
	DXGI_FORMAT_R8G8_SINT, 2,

	DXGI_FORMAT_R32_FLOAT, 4,
	DXGI_FORMAT_R32_UINT,  4,
	DXGI_FORMAT_R32_SINT, 4, // 30
	DXGI_FORMAT_R16_FLOAT, 2,
	DXGI_FORMAT_R16_UNORM, 2,
	DXGI_FORMAT_R16_UINT,  2,
	DXGI_FORMAT_R16_SNORM,  2,
	DXGI_FORMAT_R16_SINT, 2,
	DXGI_FORMAT_R8_UNORM, 1,
	DXGI_FORMAT_R8_UINT,  1,
	DXGI_FORMAT_R8_SNORM,  1,
	DXGI_FORMAT_R8_SINT, 1,

    DXGI_FORMAT_D16_UNORM, 2,       // 39
    DXGI_FORMAT_D24_UNORM_S8_UINT,  4, // 40
    DXGI_FORMAT_D24_UNORM_S8_UINT, 4, // piFORMAT_D32 = 41
    //DXGI_FORMAT_D32_FLOAT_S8X24_UINT, 4, // piFORMAT_D32 = 41
    DXGI_FORMAT_D32_FLOAT, 4,       // piFORMAT_D32F  = 42

    DXGI_FORMAT_R10G10B10A2_UNORM, 4, // piFORMAT_C4I1010102_BGRA  = 43
	DXGI_FORMAT_R10G10B10A2_UINT, 4,  // piFORMAT_C4I1010102_RGBA = 44
	DXGI_FORMAT_R11G11B10_FLOAT, 4, // piFORMAT_C3I111110_BGRA = 45
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP, 4

};

static inline DXGI_FORMAT iDxFormatEnum(const piRenderer::Format f) {	return pi_to_dx_lut[static_cast<int>(f)].format; }
static inline int         iDxFormatSize(const piRenderer::Format f) { return pi_to_dx_lut[static_cast<int>(f)].size; }
*/

piRendererDX11::piRendererDX11():piRenderer()
{
}

piRendererDX11::~piRendererDX11()
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

static const uint8_t verts3f_shader[] = {
	68,  88,  66,  67, 189, 162, 205, 104, 207,  94,  59, 251, 222, 243,  64,   5, 140, 246, 240,  73,   1,   0,   0,   0,	112,   1,   0,   0,   4,   0,	0,   0,  48,   0,   0,   0,	176,   0,   0,   0,   8,   1,	0,   0,  60,   1,   0,   0,	65, 111, 110,  57, 120,   0,	0,   0, 120,   0,   0,   0,	0,   2, 254, 255,  80,   0,	0,   0,  40,   0,   0,   0,
	0,   0,  36,   0,   0,   0,	36,   0,   0,   0,  36,   0,	0,   0,  36,   0,   1,   0,	36,   0,   0,   0,   0,   0,	1,   2, 254, 255,  81,   0,	0,   5,   1,   0,  15, 160,	0,   0, 128,  63,   0,   0,	0,   0,   0,   0,   0,   0,	0,   0,   0,   0,  31,   0,	0,   2,   5,   0,   0, 128,	0,   0,  15, 144,   2,   0,	0,   3,   0,   0,   3, 192,
	0,   0, 228, 144,   0,   0,	228, 160,   4,   0,   0,   4,	0,   0,  12, 192,   0,   0,	170, 144,   1,   0,  68, 160,	1,   0,  20, 160, 255, 255,	0,   0,  83,  72,  68,  82,	80,   0,   0,   0,  64,   0,	1,   0,  20,   0,   0,   0,	95,   0,   0,   3, 114,  16,	16,   0,   0,   0,   0,   0,	103,   0,   0,   4, 242,  32,	16,   0,   0,   0,   0,   0,
	1,   0,   0,   0,  54,   0,	0,   5, 114,  32,  16,   0,	0,   0,   0,   0,  70,  18,	16,   0,   0,   0,   0,   0,	54,   0,   0,   5, 130,  32,	16,   0,   0,   0,   0,   0,	1,  64,   0,   0,   0,   0,	128,  63,  62,   0,   0,   1,	73,  83,  71,  78,  44,   0,	0,   0,   1,   0,   0,   0,	8,   0,   0,   0,  32,   0,	0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   3,   0,	0,   0,   0,   0,   0,   0,	7,   7,   0,   0,  80,  79,	83,  73,  84,  73,  79,  78,	0, 171, 171, 171,  79,  83,	71,  78,  44,   0,   0,   0,	1,   0,   0,   0,   8,   0,	0,   0,  32,   0,   0,   0,	0,   0,   0,   0,   1,   0,	0,   0,   3,   0,   0,   0,	0,   0,   0,   0,  15,   0,	0,   0,  83,  86,  95,  80, 	111, 115, 105, 116, 105, 111,	110,   0 };

static const uint8_t verts2f_shader[] = {
	68,  88,  66,  67, 159, 162,	246, 202,  15,  88,  64, 198,	125, 187,  76, 224,  15, 167,	73, 231,   1,   0,   0,   0,	116,   1,   0,   0,   4,   0,	0,   0,  48,   0,   0,   0,	168,   0,   0,   0,  12,   1,	0,   0,  64,   1,   0,   0,	65, 111, 110,  57, 112,   0,	0,   0, 112,   0,   0,   0,	0,   2, 254, 255,  72,   0,	0,   0,  40,   0,   0,   0,
	0,   0,  36,   0,   0,   0,	36,   0,   0,   0,  36,   0,	0,   0,  36,   0,   1,   0,	36,   0,   0,   0,   0,   0,	1,   2, 254, 255,  81,   0,	0,   5,   1,   0,  15, 160,	0,   0,   0,   0,   0,   0,	128,  63,   0,   0,   0,   0,	0,   0,   0,   0,  31,   0,	0,   2,   5,   0,   0, 128,	0,   0,  15, 144,   2,   0,	0,   3,   0,   0,   3, 192,
	0,   0, 228, 144,   0,   0,	228, 160,   1,   0,   0,   2,	0,   0,  12, 192,   1,   0,	68, 160, 255, 255,   0,   0,	83,  72,  68,  82,  92,   0,	0,   0,  64,   0,   1,   0,	23,   0,   0,   0,  95,   0,	0,   3,  50,  16,  16,   0,	0,   0,   0,   0, 103,   0,0,   4, 242,  32,  16,   0,	0,   0,   0,   0,   1,   0,	0,   0,  54,   0,   0,   5,
	50,  32,  16,   0,   0,   0,	0,   0,  70,  16,  16,   0,	0,   0,   0,   0,  54,   0,	0,   8, 194,  32,  16,   0,	0,   0,   0,   0,   2,  64,	0,   0,   0,   0,   0,   0,	0,   0,   0,   0,   0,   0,	0,   0,   0,   0, 128,  63,	62,   0,   0,   1,  73,  83,	71,  78,  44,   0,   0,   0,	1,   0,   0,   0,   8,   0,	0,   0,  32,   0,   0,   0,	0,   0,   0,   0,   0,   0,
	0,   0,   3,   0,   0,   0,	0,   0,   0,   0,   3,   3,	0,   0,  80,  79,  83,  73,	84,  73,  79,  78,   0, 171,	171, 171,  79,  83,  71,  78,	44,   0,   0,   0,   1,   0,	0,   0,   8,   0,   0,   0,	32,   0,   0,   0,   0,   0,	0,   0,   1,   0,   0,   0,	3,   0,   0,   0,   0,   0,	0,   0,  15,   0,   0,   0,	83,  86,  95,  80, 111, 115, 105, 116, 105, 111, 110,   0 };

static const uint8_t verts3f3f_shader[] = {
	68,  88,  66,  67,  66, 156,	239,   2, 148, 164, 214,  45,	99,  75,  41, 149, 189,  58,	114, 236,   1,   0,   0,   0,	140,   1,   0,   0,   4,   0,	0,   0,  48,   0,   0,   0,	176,   0,   0,   0,   8,   1,	0,   0,  88,   1,   0,   0,	65, 111, 110,  57, 120,   0,	0,   0, 120,   0,   0,   0,	0,   2, 254, 255,  80,   0,	0,   0,  40,   0,   0,   0,
	0,   0,  36,   0,   0,   0,	36,   0,   0,   0,  36,   0,	0,   0,  36,   0,   1,   0,	36,   0,   0,   0,   0,   0,	1,   2, 254, 255,  81,   0,	0,   5,   1,   0,  15, 160,	0,   0, 128,  63,   0,   0,	0,   0,   0,   0,   0,   0,	0,   0,   0,   0,  31,   0,	0,   2,   5,   0,   0, 128,	0,   0,  15, 144,   2,   0,	0,   3,   0,   0,   3, 192,	0,   0, 228, 144,   0,   0,
	228, 160,   4,   0,   0,   4,	0,   0,  12, 192,   0,   0,	170, 144,   1,   0,  68, 160,	1,   0,  20, 160, 255, 255,	0,   0,  83,  72,  68,  82,	80,   0,   0,   0,  64,   0,	1,   0,  20,   0,   0,   0,	95,   0,   0,   3, 114,  16,	16,   0,   0,   0,   0,   0,	103,   0,   0,   4, 242,  32,	16,   0,   0,   0,   0,   0,	1,   0,   0,   0,  54,   0,	0,   5, 114,  32,  16,   0,
	0,   0,   0,   0,  70,  18,	16,   0,   0,   0,   0,   0,	54,   0,   0,   5, 130,  32,	16,   0,   0,   0,   0,   0,	1,  64,   0,   0,   0,   0,	128,  63,  62,   0,   0,   1,	73,  83,  71,  78,  72,   0,	0,   0,   2,   0,   0,   0,	8,   0,   0,   0,  56,   0,	0,   0,   0,   0,   0,   0,	0,   0,   0,   0,   3,   0,	0,   0,   0,   0,   0,   0,	7,   7,   0,   0,  65,   0,
	0,   0,   0,   0,   0,   0,	0,   0,   0,   0,   3,   0,	0,   0,   1,   0,   0,   0,	7,   0,   0,   0,  80,  79,	83,  73,  84,  73,  79,  78,	0,  78,  79,  82,  77,  65,	76,   0,  79,  83,  71,  78,	44,   0,   0,   0,   1,   0,	0,   0,   8,   0,   0,   0,	32,   0,   0,   0,   0,   0,	0,   0,   1,   0,   0,   0,	3,   0,   0,   0,   0,   0,	0,   0,  15,   0,   0,   0,
	83,  86,  95,  80, 111, 115,	105, 116, 105, 111, 110,   0 };

/*
void CALLBACK piRendererDX11::DebugLog( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *vme )
{
	piRendererDX11 *me = (piRendererDX11*)vme;

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

void piRendererDX11::PrintInfo( void )
{
	if( !mReporter ) return;

	char *str = (char*)malloc( 65536 );
    if( !str ) return;

    int nume = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &nume);

    sprintf( str, "OpenGL %s\nGLSL %s\n%s by %s\n%d extensions\n", (const char*)glGetString( GL_VERSION ), (const char*)glGetString( GL_SHADING_LANGUAGE_VERSION ),
                                                                   (const char*)glGetString( GL_RENDERER ), (const char*)glGetString( GL_VENDOR ), nume );
#if 1
    for( int i=0; i<nume; i++ )
    {
        strcat( str, (char const*)oglGetStringi(GL_EXTENSIONS, i) );
        strcat( str, "\n" );
    }
#endif
    mReporter->Info( str );
    free( str );
}
*/
bool piRendererDX11::Initialize(int id, const void **hwnd, int num, bool disableVSync, bool disableErrors, piReporter *reporter, bool createDevice, void *device)
{
    mID = id;

	if (device != nullptr)
	{
		mOwnsDevice = false;

		mDevice = device;
		((ID3D11Device*)device)->GetImmediateContext( (ID3D11DeviceContext**)&mDeviceContext);
	}
	else
	{
		mOwnsDevice = true;

		DXGI_SWAP_CHAIN_DESC scd;
		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
		scd.BufferCount = 1;                                    // one back buffer
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
		scd.OutputWindow = HWND(hwnd[0]);                       // the window to be used
		scd.SampleDesc.Count = 1;                               // how many multisamples
		scd.Windowed = TRUE;                                    // windowed/full-screen mode

		D3D_FEATURE_LEVEL FeatureLevels = D3D_FEATURE_LEVEL_11_0;

		ID3D11Device *dev = nullptr;
		ID3D11DeviceContext *devcon = nullptr;
		IDXGISwapChain *swapchain = nullptr;

		HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
#if defined(DEBUG)
            NULL,//D3D11_CREATE_DEVICE_DEBUG,
#else
            NULL,
#endif
			NULL,
			NULL,
			D3D11_SDK_VERSION,
			&scd,
			&swapchain,
			&dev,
			&FeatureLevels,
			&devcon);

		if (FAILED(hr))
			return false;

		mDevice = (void*)dev;
		mDeviceContext = (void*)devcon;
		mSwapChain = (void*)swapchain;

        // get the address of the back buffer
        ID3D11Texture2D *pBackBuffer;
        swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
       // mBackBufferTexture = (void*)pBackBuffer;

        // use the back buffer address to create the render target
        ID3D11RenderTargetView *backbuffer;
        dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
        pBackBuffer->Release();
        mBackBufferTarget = (void*)backbuffer;
	}



	ID3D11Device *dev = (ID3D11Device*)mDevice;
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)mDeviceContext;


	mReporter = reporter;

    mBindedTarget = nullptr;
    /*
    mMngTexSlots = nullptr;

	mRC = new piGL4X_RenderContext();
    if( !mRC )
        return false;

    if (!mRC->Create(hwnd, num, disableVSync, true, true))
    {
        mRC->Delete();
        return false;
	}

    mRC->Enable();

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
    mMngTexMax = 512;
	*/
	mMngTexMemCurrent = 0;
    mMngTexMemPeak = 0;
    mMngTexNumCurrent = 0;
    mMngTexNumPeak = 0;

    mVBO[0] = this->CreateBuffer(verts2f,   sizeof(verts2f),   BufferType::Static, BufferUse::Vertex); if (!mVBO[0]) return false;
    mVBO[1] = this->CreateBuffer(verts3f3f, sizeof(verts3f3f), BufferType::Static, BufferUse::Vertex); if (!mVBO[1]) return false;
    mVBO[2] = this->CreateBuffer(verts3f,   sizeof(verts3f),   BufferType::Static, BufferUse::Vertex); if (!mVBO[2]) return false;

    const ArrayLayout2 lay0 = { 1, { { "POSITION", Format::C2_32_FLOAT } } };
    const ArrayLayout2 lay1 = { 2, { { "POSITION", Format::C3_32_FLOAT }, { "NORMAL", Format::C3_32_FLOAT } } };
    const ArrayLayout2 lay2 = { 1, { { "POSITION", Format::C3_32_FLOAT } } };

	mVA[0] = this->CreateVertexArray2(1, mVBO[0], &lay0, nullptr, nullptr, verts2f_shader, sizeof(verts2f_shader), nullptr, piRenderer::IndexArrayFormat::UINT_16); if (!mVA[0]) return false;
	mVA[1] = this->CreateVertexArray2(1, mVBO[1], &lay1, nullptr, nullptr, verts3f3f_shader, sizeof(verts3f3f_shader), nullptr, piRenderer::IndexArrayFormat::UINT_16); if (!mVA[1]) return false;
	mVA[2] = this->CreateVertexArray2(1, mVBO[2], &lay2, nullptr, nullptr, verts3f_shader, sizeof(verts3f_shader), nullptr, piRenderer::IndexArrayFormat::UINT_16); if (!mVA[2]) return false;


	/*
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

	PrintInfo();
	*/
    return true;
}


void piRendererDX11::Deinitialize(void)
{
	if (mOwnsDevice)
	{
		ID3D11Device *dev = (ID3D11Device*)mDevice;
		ID3D11DeviceContext *devcon = (ID3D11DeviceContext*)mDeviceContext;
		IDXGISwapChain *swapchain = (IDXGISwapChain*)mSwapChain;

		swapchain->Release();
		dev->Release();
		devcon->Release();
	}
	else
	{
		ID3D11DeviceContext *devcon = (ID3D11DeviceContext*)mDeviceContext;
		devcon->Release();
	}
	//--- texture management ---
	/*
	if( mMngTexSlots!=nullptr) free( mMngTexSlots );
	*/
	this->DestroyVertexArray2(mVA[0]);
	this->DestroyVertexArray2(mVA[1]);
	this->DestroyVertexArray2(mVA[2]);
	this->DestroyBuffer(mVBO[0]);
	this->DestroyBuffer(mVBO[1]);
	this->DestroyBuffer(mVBO[2]);

	/*
	piGL4X_Ext_Free( (NGLEXTINFO*)mExt );
	mRC->Disable( false );
	mRC->Destroy();
	mRC->Delete();
	delete mRC;
	*/
}

void *piRendererDX11::GetContext(void)
{
	return (void*)mDeviceContext;
}
bool piRendererDX11::SupportsFeature(RendererFeature feature)
{
//	if (feature == VIEWPORT_ARRAY)  return mFeatureViewportArray;
	//if (feature == VERTEX_VIEWPORT) return mFeatureVertexViewport;
	return false;
}

piRenderer::API piRendererDX11::GetAPI(void)
{
	return API::DX;
}

void piRendererDX11::SetActiveWindow( int id )
{
	//mRC->SetActiveWindow( id );
}

void piRendererDX11::Enable(void)
{
    //mRC->Enable();
}
void piRendererDX11::Disable(void)
{
    //mRC->Disable(false);
}


void piRendererDX11::Report( void )
{
	if( !mReporter ) return;
	/*
	mReporter->Begin( mMngTexMemCurrent, mMngTexMemPeak, mMngTexNumCurrent, mMngTexNumPeak );
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
    }
    mReporter->End();
	*/
}



void piRendererDX11::SwapBuffers( void )
{
    IDXGISwapChain *swapchain = (IDXGISwapChain*)mSwapChain;
    swapchain->Present(0, 0);

	//mRC->SwapBuffers();
}


void piRendererDX11::StartPerformanceMeasure(void)
{
}

void piRendererDX11::EndPerformanceMeasure(void)
{
}

uint64_t piRendererDX11::GetPerformanceMeasure(void)
{
    return 0;
}


void piRendererDX11::SetShadingSamples( int shadingSamples )
{
	/*
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
	*/
}

piRTarget piRendererDX11::CreateRenderTarget( piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;


	piIRTarget *me = (piIRTarget*)malloc(sizeof(piIRTarget));
	if (!me)
		return nullptr;

    me->mColor[0] = (piITexture*)vtex0;
    me->mColor[1] = (piITexture*)vtex1;
    me->mColor[2] = (piITexture*)vtex2;
    me->mColor[3] = (piITexture*)vtex3;
    me->mDepth = (piITexture*)zbuf;

    return (piRTarget)me;
}

void piRendererDX11::DestroyRenderTarget( piRTarget obj )
{
    piIRTarget *me = (piIRTarget*)obj;

    free(me);
}

void piRendererDX11::RenderTargetSampleLocations(piRTarget vdst, const float *locations )
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

void piRendererDX11::RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location)
{
    //const piIRTarget *dst = (piIRTarget*)vdst;
    //oglGetMultisamplefv( GL_SAMPLE_LOCATION_ARB, id, location);
}

void piRendererDX11::BlitRenderTarget( piRTarget vdst, piRTarget vsrc, bool color, bool depth )
{
	/*
    const piIRTarget *src = (piIRTarget*)vsrc;
    const piIRTarget *dst = (piIRTarget*)vdst;

    int flag = 0;
    if( color ) flag += GL_COLOR_BUFFER_BIT;
    if( depth ) flag += GL_DEPTH_BUFFER_BIT;

	const GLenum mMRT[1] = { GL_COLOR_ATTACHMENT0 };
	oglNamedFramebufferDrawBuffers(dst->mObjectID, 1, mMRT);

	oglBlitNamedFramebuffer( src->mObjectID, dst->mObjectID,
	                         0, 0, dst->mXres, dst->mYres,
                             0, 0, dst->mXres, dst->mYres,
                             flag, GL_NEAREST );
    */
}


bool piRendererDX11::SetRenderTarget( piRTarget obj )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)mDeviceContext;

	if( obj==NULL )
    {
        // set the render target as the back buffer
        ID3D11RenderTargetView *backbuffer = (ID3D11RenderTargetView *)mBackBufferTarget;
        ctx->OMSetRenderTargets(1, &backbuffer, NULL);
        mBindedTarget = nullptr;
    }
    else
    {
        mBindedTarget = obj;

        piIRTarget *me = (piIRTarget*)obj;

        ID3D11DepthStencilView *zt = { (me->mDepth == nullptr) ? nullptr : me->mDepth->mViewDepthStencil };
        ID3D11RenderTargetView *ct[4];
        int num = 0;
        for (int i = 0; i < 4; i++)
        {
            if (me->mColor[i]==nullptr) break;
            ct[num] = me->mColor[i]->mViewRenderTarget;
            num++;
        }

        ctx->OMSetRenderTargets(num, ct, zt);
	}

    return true;
}



void piRendererDX11::SetViewport( int id, const int *vp )
{
    ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)mDeviceContext;

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = float(vp[0]);
    viewport.TopLeftY = float(vp[1]);;
    viewport.Width = float(vp[2]);
    viewport.Height = float(vp[3]);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    ctx->RSSetViewports(1, &viewport);

//    oglViewportIndexedf(id, float(vp[0]), float(vp[1]), float(vp[2]), float(vp[3]) );
}

void piRendererDX11::SetViewports(int num, const float *viewports) // x,y,w,h,zmin,zmax // 0.0, 0.0, 800.0, 600.0, 0.0, 1.0
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)mDeviceContext;

	/*
	D3D11_VIEWPORT vp[4];
	for (int i = 0; i < num; i++)
	{
		vp[i].TopLeftX = viewports[6 * num + 0];
		vp[i].TopLeftY = viewports[6 * num + 1];
		vp[i].Width    = viewports[6 * num + 2];
		vp[i].Height   = viewports[6 * num + 3];
		vp[i].MinDepth = viewports[6 * num + 4];
		vp[i].MaxDepth = viewports[6 * num + 5];
	}
	ctx->RSSetViewports(num, vp);
	*/

	ctx->RSSetViewports(num, (D3D11_VIEWPORT*)viewports );
}

void piRendererDX11::GetViewports(int *num, float *viewports) // x,y,w,h,zmin,zmax // 0.0, 0.0, 800.0, 600.0, 0.0, 1.0
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;
	ID3D11DeviceContext* ctx = (ID3D11DeviceContext*)mDeviceContext;

	ctx->RSGetViewports( (UINT*)num, (D3D11_VIEWPORT*)viewports);
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

/*
static piITexture *piITexture_Create( const piTextureInfo *info, piTextureFilter rfilter, piTextureWrap rwrap, float aniso, void *buffer, void *mExt )
{
    int mode, moInternal, mode3, bpp;

    if (!format2gl(info->mFormat, &bpp, &mode, &moInternal, &mode3, info->mCompressed))
        return nullptr;

    piITexture *me = (piITexture*)malloc( sizeof(piITexture) );
    if( !me )
        return nullptr;

    me->mHandle = 0;
    me->mIsResident = false;
	me->mInfo = *info;
    me->mFilter = rfilter;
    me->mWrap = rwrap;


    //const int filter = filter2gl[ rfilter ];
    const int wrap = wrap2gl[ rwrap ];

    if( info->mType==piTEXTURE_2D )
    {
        if (info->mMultisample>1)
        {
            oglCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &me->mObjectID);
            oglTextureStorage2DMultisample(me->mObjectID, info->mMultisample, moInternal, info->mXres, info->mYres, GL_FALSE);
        }
        else
        {
            oglCreateTextures(GL_TEXTURE_2D, 1, &me->mObjectID);
            switch (rfilter)
    	    {
            case piFILTER_NONE:
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
						oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, mode, mode3, buffer);
					}
                }
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                break;

            case piFILTER_LINEAR:
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
							oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, mode, mode3, buffer);
						}
                    }
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
    	    	    break;

            case piFILTER_PCF:
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

            case piFILTER_MIPMAP:
            {
				const int numMipmaps = ilog2i(info->mXres );
                oglTextureStorage2D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres);
				if (buffer)
				{
					const int rowsize = info->mXres*bpp;
					if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, mode, mode3, buffer);
				}
                oglGenerateTextureMipmap(me->mObjectID);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                if( aniso>1.0001f )
                oglTextureParameterf(me->mObjectID, 0x84FE, aniso); //  GL_TEXTURE_MAX_ANISOTROPY_EXT
    	    	break;
            }
            case piFILTER_NONE_MIPMAP:
            {
                    const int numMipmaps = ilog2i(info->mXres );
                    oglTextureStorage2D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres);
					if (buffer)
					{
						const int rowsize = info->mXres*bpp;
						if ((rowsize & 3) == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4); else glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
						oglTextureSubImage2D(me->mObjectID, 0, 0, 0, info->mXres, info->mYres, mode, mode3, buffer);
					}
                    oglGenerateTextureMipmap(me->mObjectID);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, numMipmaps);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
                    oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
                    if( aniso>1.0001f )
                    oglTextureParameterf(me->mObjectID, 0x84FE, aniso); //  GL_TEXTURE_MAX_ANISOTROPY_EXT
    	    	    break;
            }
    	    }
        }
    }
    else if(  info->mType==piTEXTURE_3D )
    {
        oglCreateTextures(GL_TEXTURE_3D, 1, &me->mObjectID);
        oglTextureStorage3D(me->mObjectID, 1, moInternal, info->mXres, info->mYres, info->mZres);
        if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, mode, mode3, buffer);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_R, wrap);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_S, wrap);
        oglTextureParameteri(me->mObjectID, GL_TEXTURE_WRAP_T, wrap);
    }
    else if( info->mType==piTEXTURE_CUBE )
    {
        if (rfilter == piFILTER_MIPMAP)
        {
            const int numMipmaps = ilog2i(info->mXres);
            oglCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &me->mObjectID);
            oglTextureStorage3D(me->mObjectID, numMipmaps, moInternal, info->mXres, info->mYres, 6);
            if (buffer)
            {
                oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, mode, mode3, buffer);
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
                oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, 6, mode, mode3, buffer);
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
        oglTextureParameterf(me->mObjectID, 0x84FE, aniso);
    }
    else if( info->mType==piTEXTURE_2D_ARRAY )
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
            //if (buffer) oglTextureSubImage3D(me->mObjectID, 0, 0, 0, 0, info->mXres, info->mYres, info->mZres, mode, mode3, buffer);

            if( rfilter==piFILTER_PCF )
            {
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_BASE_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAX_LEVEL, 0);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                oglTextureParameteri(me->mObjectID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

	me->mHandle = oglGetTextureHandle( me->mObjectID );


    return me;
}
*/
piTexture piRendererDX11::CreateTextureFromID(unsigned int id, TextureFilter filter )
{
	/*
    piITexture *me = (piITexture*)malloc(sizeof(piITexture));
    if (!me)
        return nullptr;

    me->mObjectID = id;
    //me->mInfo = *info;
    me->mFilter = filter;
    //me->mWrap = rwrap;
	return me;
	*/
	return nullptr;
}

piTexture piRendererDX11::CreateTexture(const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer)
{
    ID3D11Device *dev = (ID3D11Device*)mDevice;


    piITexture * me = (piITexture*)malloc(sizeof(piITexture));
    if (!me)
        return nullptr;

    me->mInfo.mXres = info->mXres;
    me->mInfo.mYres = info->mYres;
    me->mInfo.mZres = info->mZres;
    me->mInfo.mMultisample = info->mMultisample;
    me->mInfo.mType = info->mType;
    me->mInfo.mDeleteMe = info->mDeleteMe;
    me->mInfo.mFormat = info->mFormat;


    me->mFilter = filter;
    me->mWrap = wrap1;
    me->mDXFormat = iDxFormatEnum(me->mInfo.mFormat);
    me->mViewShader = nullptr;
    me->mViewDepthStencil = nullptr;
    me->mViewRenderTarget = nullptr;

    if (info->mType == TextureType::T2D)
    {
        // temp code, remove this
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // So. For mipmapping there are two options.
        // A. use GPU to compute mimaps:
        //      1. set miplevels to 0
        //      2. set usage to DEFAULT
        //      3. set bind flags to D3D11_BIND_SHADER_RESOURCE AND D3D11_BIND_RENDER_TARGET
        //      4. set misc flags to D3D11_RESOURCE_MISC_GENERATE_MIPS
        //      5. create an empty texture with CreateTexture2D()
        //      6. copy the texture through the context
        //      7. generate the texture through the context
        //
        // B. use the GPU to compute the mips
        //      1. set miplevels to N
        //      2. set usage to D3D11_USAGE_IMMUTABLE
        //      3. set bind flags to just D3D11_BIND_SHADER_RESOURCE
        //      4. set misc flags to 0
        //      5. create the  texture normally with CreateTexture2D(), but passing an
        //         array of D3D11_SUBRESOURCE_DATA, one per mip level, with a different
        //         data pointer.

        if (filter == TextureFilter::MIPMAP)BindFlags |= D3D11_BIND_RENDER_TARGET;

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = info->mXres;
        desc.Height = info->mYres;
        desc.MipLevels = (filter== TextureFilter::MIPMAP) ? 0 : 1;
        desc.ArraySize = 1;
        desc.Format = me->mDXFormat;
        desc.SampleDesc.Count = info->mMultisample;
        desc.SampleDesc.Quality = 0;
        desc.Usage = (filter == TextureFilter::MIPMAP) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;// D3D11_USAGE_DEFAULT;// D3D11_USAGE_DYNAMIC; // D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = BindFlags;
        desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE
        desc.MiscFlags = (filter == TextureFilter::MIPMAP)?D3D11_RESOURCE_MISC_GENERATE_MIPS:0;

        if (buffer == nullptr || filter == TextureFilter::MIPMAP)
        {
            HRESULT hr = dev->CreateTexture2D(&desc, NULL, &me->mObjectID);
            if (FAILED(hr))
                return nullptr;
        }
        else
        {
            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = buffer;
            data.SysMemPitch = info->mXres * iDxFormatSize(me->mInfo.mFormat);
            data.SysMemSlicePitch = 0;
            HRESULT hr = dev->CreateTexture2D(&desc, &data, &me->mObjectID);
            if (FAILED(hr))
                return nullptr;
        }
        if (filter == TextureFilter::MIPMAP)
        {
            ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
            int rowPitch = info->mXres * iDxFormatSize(me->mInfo.mFormat);
            ctx->UpdateSubresource(me->mObjectID, 0, NULL, buffer, rowPitch, 0);
        }

        {
            D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
            ZeroMemory(&vdesc, sizeof(vdesc));
            vdesc.Format = desc.Format;
            vdesc.Texture2D.MostDetailedMip = 0;
            vdesc.Texture2D.MipLevels = (filter==TextureFilter::MIPMAP)?-1:1;

            if (info->mMultisample <= 1)
            {
                vdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            }
            else
            {
                vdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }

            HRESULT hr = dev->CreateShaderResourceView(me->mObjectID, &vdesc, &me->mViewShader);
            if (FAILED(hr))
                return nullptr;
        }

        if (filter == TextureFilter::MIPMAP)
        {
            ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
            ctx->GenerateMips(me->mViewShader);
        }


        //ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
        //ctx->UpdateSubresource(me->mObjectID, 0, NULL, buffer, info->mXres * iDxFormatSize(me->mInfo.mFormat), 0);
        //ctx->GenerateMips(me->mViewShader);

    }
    //-----------------------

    //mMngTexMemCurrent += piTexture_GetMem(me);
    mMngTexNumCurrent += 1;

    if (mMngTexNumCurrent > mMngTexNumPeak) mMngTexNumPeak = mMngTexNumCurrent;
    if (mMngTexMemCurrent > mMngTexMemPeak) mMngTexMemPeak = mMngTexMemCurrent;

    return (piTexture)me;
}

piTexture piRendererDX11::CreateTexture2(const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer, int bindUsage)
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piITexture * me = (piITexture*)malloc(sizeof(piITexture));
	if (!me)
		return nullptr;
	me->mInfo = *info;
	me->mFilter = filter;
	me->mWrap = wrap1;
    me->mDXFormat = iDxFormatEnum(info->mFormat);
    me->mViewShader = nullptr;
    me->mViewDepthStencil = nullptr;
    me->mViewRenderTarget = nullptr;

	if (info->mType == TextureType::T2D)
	{
        UINT BindFlags = 0;
        if (bindUsage & 1) BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        //if (bindUsage & 2) BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        //if (bindUsage & 4) BindFlags |= D3D11_BIND_RENDER_TARGET;

        if (bindUsage & 2)
        {
            if (info->mFormat == piRenderer::Format::D1_32_FLOAT ||
                info->mFormat == piRenderer::Format::D1_16_UNORM ||
                info->mFormat == piRenderer::Format::DS_24_8_UINT ||
                info->mFormat == piRenderer::Format::DS_32_8_UINT)
            {
                BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            else
            {
                BindFlags |= D3D11_BIND_RENDER_TARGET;
            }
        }


		D3D11_TEXTURE2D_DESC desc;
		desc.Width = info->mXres;
		desc.Height = info->mYres;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = me->mDXFormat;
		desc.SampleDesc.Count = info->mMultisample;
		desc.SampleDesc.Quality = 0;
        desc.MipLevels = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;// D3D11_USAGE_DYNAMIC; // D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = BindFlags;
		desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

        //s = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		if (buffer == nullptr)
		{
			HRESULT hr = dev->CreateTexture2D(&desc, NULL, &me->mObjectID);
			if (FAILED(hr))
				return nullptr;
		}
		else
		{
			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = buffer;
            data.SysMemPitch = info->mXres * iDxFormatSize(me->mInfo.mFormat);
			data.SysMemSlicePitch = 0;
			HRESULT hr = dev->CreateTexture2D(&desc, &data, &me->mObjectID);
			if (FAILED(hr))
				return nullptr;
		}



        if (bindUsage & 1)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
            ZeroMemory(&vdesc, sizeof(vdesc));
            vdesc.Format = desc.Format;
            vdesc.Texture2D.MostDetailedMip = 0;
            vdesc.Texture2D.MipLevels = (filter == TextureFilter::MIPMAP) ? -1 : 1;
            vdesc.ViewDimension = (me->mInfo.mMultisample>1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;

            //HRESULT hr = dev->CreateShaderResourceView(me->mObjectID, /*&vdesc*/NULL, &me->mView);
            HRESULT hr = dev->CreateShaderResourceView(me->mObjectID, &vdesc, &me->mViewShader);
            if (FAILED(hr))
                return nullptr;
        }

        if (bindUsage & 2)
        {
            if (info->mFormat == piRenderer::Format::D1_32_FLOAT ||
                info->mFormat == piRenderer::Format::D1_16_UNORM ||
                info->mFormat == piRenderer::Format::DS_24_8_UINT ||
                info->mFormat == piRenderer::Format::DS_32_8_UINT)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC desc;
                ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
                desc.Format = me->mDXFormat;
                desc.ViewDimension = (info->mMultisample > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
                HRESULT hr = dev->CreateDepthStencilView(me->mObjectID, &desc, &me->mViewDepthStencil);
                if (FAILED(hr))
                    return nullptr;
            }
            else
            {
                D3D11_RENDER_TARGET_VIEW_DESC desc;
                ZeroMemory(&desc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
                desc.Format = me->mDXFormat;
                desc.Texture2D.MipSlice = 0;
                desc.ViewDimension = (info->mMultisample > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;

                HRESULT hr = dev->CreateRenderTargetView(me->mObjectID, &desc, &me->mViewRenderTarget);
                if (FAILED(hr))
                    return nullptr;
            }
        }
	}
	//-----------------------

	//mMngTexMemCurrent += piTexture_GetMem(me);
	mMngTexNumCurrent += 1;

	if (mMngTexNumCurrent > mMngTexNumPeak) mMngTexNumPeak = mMngTexNumCurrent;
	if (mMngTexMemCurrent > mMngTexMemPeak) mMngTexMemPeak = mMngTexMemCurrent;

	return (piTexture)me;
}


void piRendererDX11::ComputeMipmaps( piTexture vme )
{
    ID3D11Device *dev = (ID3D11Device*)mDevice;
    ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

    piITexture *me = (piITexture*)vme;
    //if( me->mFilter!=piFILTER_MIPMAP ) return;
    //oglGenerateTextureMipmap(me->mObjectID);

    ctx->GenerateMips( me->mViewShader);
}


void piRendererDX11::DestroyTexture( piTexture vme )
{

}


void piRendererDX11::AttachTextures( int num,
                                     piTexture vt0, piTexture vt1, piTexture vt2, piTexture vt3, piTexture vt4, piTexture vt5, piTexture vt6, piTexture vt7,
                                     piTexture vt8, piTexture vt9, piTexture vt10, piTexture vt11, piTexture vt12, piTexture vt13, piTexture vt14, piTexture vt15 )
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	piITexture *me0 = (piITexture*)vt0;

	ctx->PSSetShaderResources(0, 1, &me0->mViewShader);

	/*
    piITexture *t[16] = { (piITexture*)vt0, (piITexture*)vt1, (piITexture*)vt2, (piITexture*)vt3, (piITexture*)vt4, (piITexture*)vt5, (piITexture*)vt6, (piITexture*)vt7,
                          (piITexture*)vt8, (piITexture*)vt9, (piITexture*)vt10, (piITexture*)vt11, (piITexture*)vt12, (piITexture*)vt13, (piITexture*)vt14, (piITexture*)vt15 };

	GLuint texIDs[16];
    for (int i = 0; i<num; i++)
        texIDs[i] = (t[i]) ? t[i]->mObjectID : 0;

    oglBindTextures( 0, num, texIDs );*/
}


void piRendererDX11::AttachTextures( int num, piTexture *vt, int offset )
{
}

void piRendererDX11::DettachTextures( void )
{
    ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

    ID3D11ShaderResourceView* nulls[] = { nullptr, nullptr, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 1, nulls);
}

void piRendererDX11::AttachImage(int unit, piTexture texture, int level, bool layered, int layer, Format format)
{
	/*
    int mode, moInternal, mode3, bpp;

    if (!format2gl(format, &bpp, &mode, &moInternal, &mode3, false))
        return;

    oglBindImageTexture(unit, ((piITexture*)texture)->mObjectID, level, layered, layer, GL_READ_WRITE, moInternal);
	*/
}

void piRendererDX11::ClearTexture( piTexture vme, int level, const void *data )
{
	/*
    piITexture *me = (piITexture*)vme;

    int mode, mode2, mode3, bpp;
    if( !format2gl( me->mInfo.mFormat, &bpp, &mode, &mode2, &mode3, me->mInfo.mCompressed ) )
        return;

    oglActiveTexture( unidades[0] );
    if( me->mInfo.mType==piTEXTURE_2D )
    {
		oglClearTexImage( me->mObjectID, level, mode, mode3, data );
    }
    else if( me->mInfo.mType==piTEXTURE_2D_ARRAY )
    {
        oglClearTexSubImage( me->mObjectID, level, 0, 0, 0, me->mInfo.mXres, me->mInfo.mYres, me->mInfo.mZres, mode, mode3, data );
    }*/

}

void piRendererDX11::UpdateTexture( piTexture vme, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer )
{
	/*
    piITexture *me = (piITexture*)vme;

    int fFormat, fInternal, fType, bpp;
    if( !format2gl( me->mInfo.mFormat, &bpp, &fFormat, &fInternal, &fType, me->mInfo.mCompressed ) )
        return;

    if( me->mInfo.mType==piTEXTURE_2D )
    {
	    oglTextureSubImage2D( me->mObjectID, 0, x0, y0, xres, yres, fFormat, fType, buffer);
	    if (me->mFilter == piFILTER_MIPMAP)
		    oglGenerateTextureMipmap(me->mObjectID);
    }
    else if( me->mInfo.mType==piTEXTURE_2D_ARRAY )
    {
	    oglTextureSubImage3D( me->mObjectID, 0, x0, y0, z0, xres, yres, zres, fFormat, fType, buffer);
    }*/
}

void piRendererDX11::MakeResident( piTexture vme )
{
	/*
    piITexture *me = (piITexture*)vme;
    if( me->mIsResident ) return;
    oglMakeTextureHandleResident( me->mHandle );
    me->mIsResident = true;*/
}
void piRendererDX11::MakeNonResident( piTexture vme )
{
	/*
    piITexture *me = (piITexture*)vme;
    if( !me->mIsResident ) return;
    oglMakeTextureHandleNonResident( me->mHandle );
    me->mIsResident = false;*/
}
uint64_t piRendererDX11::GetTextureHandle( piTexture vme )
{
    //piITexture *me = (piITexture*)vme;
    //return me->mHandle;
	return 0;
}


//==================================================
static D3D11_TEXTURE_ADDRESS_MODE r2d_wrap(piRenderer::TextureWrap wrap)
{
	if (wrap == piRenderer::TextureWrap::CLAMP)           return D3D11_TEXTURE_ADDRESS_CLAMP;
	if (wrap == piRenderer::TextureWrap::CLAMP_TO_BORDER) return D3D11_TEXTURE_ADDRESS_BORDER;
	if (wrap == piRenderer::TextureWrap::MIRROR_CLAMP)    return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
	if (wrap == piRenderer::TextureWrap::MIRROR_REPEAT)   return D3D11_TEXTURE_ADDRESS_MIRROR;
	if (wrap == piRenderer::TextureWrap::REPEAT)          return D3D11_TEXTURE_ADDRESS_WRAP;
	return D3D11_TEXTURE_ADDRESS_CLAMP;
}

piSampler piRendererDX11::CreateSampler(TextureFilter filter, TextureWrap wrap, float maxAnisotrop)
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piISampler *me = (piISampler*)malloc( sizeof(piISampler) );
    if( !me )
        return nullptr;

	D3D11_SAMPLER_DESC desc;
	memset(&desc, 0, sizeof(D3D11_SAMPLER_DESC));

	     if (filter == TextureFilter::MIPMAP ) desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	else if (filter == TextureFilter::LINEAR ) desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	else if (filter == TextureFilter::NONE)    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	desc.AddressU = r2d_wrap(wrap);
	desc.AddressV = r2d_wrap(wrap);
	desc.AddressW = r2d_wrap(wrap);
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = int(maxAnisotrop);
	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	desc.BorderColor[0] = 0;
	desc.BorderColor[1] = 0;
	desc.BorderColor[2] = 0;
	desc.BorderColor[3] = 0;
	desc.MinLOD = 0;
	desc.MaxLOD =  (filter == TextureFilter::LINEAR)?0.0f:D3D11_FLOAT32_MAX;

	HRESULT hr = dev->CreateSamplerState(&desc, &me->mObjectID);
	if (FAILED(hr))
		return nullptr;

    return (piSampler)me;
}

void piRendererDX11::DestroySampler( piSampler obj )
{
    piISampler *me = (piISampler*)obj;
    me->mObjectID->Release();
}


void piRendererDX11::AttachSamplers(int num, piSampler vt0, piSampler vt1, piSampler vt2, piSampler vt3, piSampler vt4, piSampler vt5, piSampler vt6, piSampler vt7)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	piISampler *me0 = (piISampler*)vt0;

	ctx->PSSetSamplers(0, 1, &me0->mObjectID);

	/*
    piISampler *t[8] = { (piISampler*)vt0, (piISampler*)vt1, (piISampler*)vt2, (piISampler*)vt3, (piISampler*)vt4, (piISampler*)vt5, (piISampler*)vt6, (piISampler*)vt7 };

    GLuint texIDs[8];
    for( int i=0; i<num; i++ )
    {
        texIDs[i] = ( t[i] ) ? t[i]->mObjectID : 0;
    }
    oglBindSamplers( 0, num, texIDs );
	*/
}

void piRendererDX11::DettachSamplers( void )
{
    //GLuint texIDs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    //oglBindSamplers( 0, 8, texIDs );
}

//===========================================================================================================================================
/*
static const char *versionStr = "#version 440 core\n";

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
}*/

piShader piRendererDX11::CreateShader(const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
{
	return nullptr;
}

//piShader piRendererDX11::CreateShaderRaw(const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error)
//{
//    return nullptr;
//}

piShader piRendererDX11::CreateShaderBinary( const piShaderOptions *options, const uint8_t *vs, const int vs_len,
	                                                                         const uint8_t *cs, const int cs_len,
	                                                                         const uint8_t *es, const int es_len,
		                                                                     const uint8_t *gs, const int gs_len,
	                                                                         const uint8_t *fs, const int fs_len,
	                                                                         char *error)
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIShader *me = (piIShader*)malloc( sizeof(piIShader) );
    if( !me )
         return nullptr;

	me->mVertexShader = nullptr;
	me->mPixelShader = nullptr;


	HRESULT hr;
	hr = dev->CreateVertexShader(vs, vs_len, nullptr, &me->mVertexShader);
	if (FAILED(hr)) return nullptr;

	hr = dev->CreatePixelShader(fs, fs_len, nullptr, &me->mPixelShader);
	if (FAILED(hr)) return nullptr;

	/*
	ID3D10Blob* errorMessage = nullptr;
	ID3D10Blob* vertexShaderBuffer;
	hr = D3DX11CompileFromMemory(vs, pistrlen(vs), "tmp.hlsl",
		nullptr, // defines
		nullptr, // include
		"VS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_ROW_MAJOR, 0, nullptr, &vertexShaderBuffer, &errorMessage,
		nullptr // hresult
	);
	if (FAILED(hr))
	{
		if (errorMessage)
		{
			//OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
			errorMessage->Release();
		}
		return nullptr;
	}
	*/

    return (piShader)me;
}


piShader piRendererDX11::CreateCompute(const piShaderOptions *options, const char *cs, char *error)
{
    if (!cs)
        return nullptr;
	/*
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
	*/
	return nullptr;
}


void piRendererDX11::DestroyShader( piShader vme )
{
    piIShader *me = (piIShader *)vme;

	if (me->mVertexShader) me->mVertexShader->Release();
	if (me->mPixelShader) me->mPixelShader->Release();

	free(me);
}

void piRendererDX11::AttachShader( piShader vme )
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

    piIShader *me = (piIShader *)vme;

	ctx->VSSetShader(me->mVertexShader, NULL, 0);
	ctx->PSSetShader(me->mPixelShader, NULL, 0);

}

void piRendererDX11::DettachShader( void )
{
    //oglUseProgram( 0 );
}

void piRendererDX11::AttachShaderConstants(piBuffer obj, int unit)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	piIBuffer *me = (piIBuffer *)obj;
	piAssert(me->mUse == BufferUse::Constant);
	ctx->VSSetConstantBuffers(unit, 1, &me->mObject);
	ctx->PSSetConstantBuffers(unit, 1, &me->mObject);
}

void piRendererDX11::AttachShaderBuffer(piBuffer obj, int unit)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	piIBuffer *me = (piIBuffer *)obj;

	piAssert(me->mUse == BufferUse::ShaderResource);

	ctx->VSSetShaderResources(unit, 1, &me->mView);
	ctx->PSSetShaderResources(unit, 1, &me->mView);
}

void piRendererDX11::DettachShaderBuffer(int unit)
{
    //oglBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, 0);
}

void piRendererDX11::AttachAtomicsBuffer(piBuffer obj, int unit)
{
    //piIBuffer *me = (piIBuffer *)obj;
    //oglBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, me->mObjectID);
}

void piRendererDX11::DettachAtomicsBuffer(int unit)
{
    //oglBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, unit, 0);
}

void piRendererDX11::SetShaderConstant4F(const unsigned int pos, const float *value, int num)
{
	//oglUniform4fv(pos,num,value);
}
void piRendererDX11::SetShaderConstant3F(const unsigned int pos, const float *value, int num)
{
	//oglUniform3fv(pos,num,value);
}
void piRendererDX11::SetShaderConstant2F(const unsigned int pos, const float *value, int num)
{
	//oglUniform2fv(pos,num,value);
}
void piRendererDX11::SetShaderConstant1F(const unsigned int pos, const float *value, int num)
{
	//oglUniform1fv(pos,num,value);
}
void piRendererDX11::SetShaderConstant1I(const unsigned int pos, const int *value, int num)
{
	//oglUniform1iv(pos,num,value);
}
void piRendererDX11::SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num)
{
	//oglUniform1uiv(pos,num,value);
}
void piRendererDX11::SetShaderConstant2UI(const unsigned int pos, const unsigned int *value, int num)
{
}

void piRendererDX11::SetShaderConstant3UI(const unsigned int pos, const unsigned int *value, int num)
{
}

void piRendererDX11::SetShaderConstant4UI(const unsigned int pos, const unsigned int *value, int num)
{
}

void piRendererDX11::SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose)
{
	//oglUniformMatrix4fv(pos,num,transpose,value);
}
void piRendererDX11::SetShaderConstantSampler(const unsigned int pos, int unit)
{
	//oglUniform1i(pos,unit);
}
/*
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
};*/

void piRendererDX11::SetBlending( int buf, BlendEquation equRGB, BlendOperations srcRGB, BlendOperations dstRGB,
                                           BlendEquation equALP, BlendOperations srcALP, BlendOperations dstALP )
{
    //oglBlendEquationSeparatei(buf, r2gl_blendEqua[equRGB], r2gl_blendEqua[equALP]);
    //oglBlendFuncSeparatei(    buf, r2gl_blendMode[srcRGB], r2gl_blendMode[dstRGB],
      //                             r2gl_blendMode[srcALP], r2gl_blendMode[dstALP]);
}

void piRendererDX11::SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z )
{
    //glDepthMask( z?GL_TRUE:GL_FALSE );
    //oglColorMaski( 0, c0, c0, c0, c0 );
    //oglColorMaski( 1, c0, c0, c0, c0 );
    //oglColorMaski( 2, c0, c0, c0, c0 );
    //oglColorMaski( 3, c0, c0, c0, c0 );
}

void piRendererDX11::SetState( piState state, bool value )
{
}

//----------------------------------------------------

piRasterState piRendererDX11::CreateRasterState(bool wireframe, bool frontIsCounterClockWise, CullMode cullMode, bool depthClamp, bool multiSample )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIRasterState *me = (piIRasterState*)malloc(sizeof(piIRasterState));
	if (!me)
		return nullptr;

	me->mDesc.FillMode = (wireframe) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	me->mDesc.CullMode = (cullMode!=CullMode::NONE)  ? ((cullMode==CullMode::FRONT) ? D3D11_CULL_FRONT : D3D11_CULL_BACK)  : D3D11_CULL_NONE;
    me->mDesc.DepthClipEnable = !depthClamp;
	me->mDesc.FrontCounterClockwise = frontIsCounterClockWise;
	me->mDesc.DepthBias = 0;
	me->mDesc.DepthBiasClamp = 0.0f;
	me->mDesc.SlopeScaledDepthBias = 0.0f;
	me->mDesc.ScissorEnable = false;
	me->mDesc.MultisampleEnable = multiSample;
	me->mDesc.AntialiasedLineEnable = false;

	HRESULT hr = dev->CreateRasterizerState(&me->mDesc, &me->mObject);
	if (FAILED(hr)) return nullptr;

	return (piRasterState)me;
}

void piRendererDX11::SetRasterState(const piRasterState vme)
{
	piIRasterState *me = (piIRasterState*)vme;
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	ctx->RSSetState(me->mObject);
}

void piRendererDX11::DestroyRasterState( piRasterState vme)
{
	piIRasterState *me = (piIRasterState*)vme;
	me->mObject->Release();
	free(me);
}

//----------------------------------------------------
piBlendState piRendererDX11::CreateBlendState(bool alphaToCoverage, bool enabled0 )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIBlendState *me = (piIBlendState*)malloc(sizeof(piIBlendState));
	if (!me)
		return nullptr;

	memset(&me->mDesc, 0, sizeof(D3D11_BLEND_DESC));
	me->mDesc.AlphaToCoverageEnable = alphaToCoverage;
	me->mDesc.IndependentBlendEnable = false;
	me->mDesc.RenderTarget[0].BlendEnable = enabled0;
	me->mDesc.RenderTarget[0].RenderTargetWriteMask = 0xF;

	HRESULT hr = dev->CreateBlendState(&me->mDesc, &me->mObject);
	if (FAILED(hr)) return nullptr;

	return (piBlendState)me;
}

void piRendererDX11::SetBlendState(const piBlendState vme)
{
	piIBlendState *me = (piIBlendState*)vme;
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	ctx->OMSetBlendState(me->mObject, NULL, 0xFFFFFFFF);
}

void piRendererDX11::DestroyBlendState(piBlendState vme)
{
	piIBlendState *me = (piIBlendState*)vme;
	me->mObject->Release();
	free(me);
}

//----------------------------------------------------
piDepthState piRendererDX11::CreateDepthState(bool depthEnable, bool lessEqual)
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIDepthState *me = (piIDepthState*)malloc(sizeof(piIDepthState));
	if (!me)
		return nullptr;

	memset(&me->mDesc, 0, sizeof(D3D11_DEPTH_STENCIL_DESC));

	me->mDesc.DepthEnable = depthEnable;
	me->mDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	me->mDesc.DepthFunc = lessEqual ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_GREATER_EQUAL;
	me->mDesc.StencilEnable = false;

	HRESULT hr = dev->CreateDepthStencilState(&me->mDesc, &me->mObject);
	if (FAILED(hr)) return nullptr;

	return (piDepthState)me;
}

void piRendererDX11::SetDepthState(const piDepthState vme)
{
	piIDepthState *me = (piIDepthState*)vme;
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	ctx->OMSetDepthStencilState(me->mObject, 0);
}

void piRendererDX11::DestroyDepthState(piDepthState vme)
{
	piIDepthState *me = (piIDepthState*)vme;
	me->mObject->Release();
	free(me);
}


void piRendererDX11::Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

    ID3D11RenderTargetView *tv = (mBindedTarget == nullptr) ? (ID3D11RenderTargetView *)mBackBufferTarget :
        ((piIRTarget*)mBindedTarget)->mColor[0]->mViewRenderTarget;

    ctx->ClearRenderTargetView(tv, color0);

    if (depth0)
    {
        ID3D11DepthStencilView *dsv = ((piIRTarget*)mBindedTarget)->mDepth->mViewDepthStencil;
        ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

//-----------------------

piBuffer piRendererDX11::CreateBuffer(const void *data, unsigned int amount, BufferType mode, BufferUse use )
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIBuffer *me = (piIBuffer*)malloc(sizeof(piIBuffer));
	if (!me)
		return nullptr;

	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_DEFAULT;

desc.Usage = (mode == BufferType::Dynamic) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;

	desc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_READ
/*
	if (mode == BufferType::Dynamic && (use== BufferUse::Index || use== BufferUse::ShaderResource) )
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
*/

	desc.ByteWidth = (use!=BufferUse::Constant) ? amount : (amount + 15) & 0xfffffff0; // must be multiple of 16 for constant buffers
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.BindFlags = 0;

	if ((static_cast<int>(use) & static_cast<int>(BufferUse::Vertex)) != 0) desc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
	if ((static_cast<int>(use) & static_cast<int>(BufferUse::Index)) != 0) desc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
	if ((static_cast<int>(use) & static_cast<int>(BufferUse::Constant)) != 0) desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
	if ((static_cast<int>(use) & static_cast<int>(BufferUse::ShaderResource)) != 0) desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if ((static_cast<int>(use) & static_cast<int>(BufferUse::DrawCommands)) != 0) desc.BindFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

	if (data == nullptr)
	{
		HRESULT hr = dev->CreateBuffer(&desc, nullptr, &me->mObject);
		if (FAILED(hr)) return nullptr;
	}
	else
	{
		D3D11_SUBRESOURCE_DATA dataDesc;
		dataDesc.pSysMem = data;
		dataDesc.SysMemPitch = 0;
		dataDesc.SysMemSlicePitch = 0;
		HRESULT hr = dev->CreateBuffer(&desc, &dataDesc, &me->mObject);
		if (FAILED(hr)) return nullptr;
	}

	me->mUse = use;
	me->mSize = amount;
	me->mView = nullptr;

	return (piBuffer)me;
}

piBuffer piRendererDX11::CreateStructuredBuffer(const void *data, unsigned int numElements, unsigned int elementSize, BufferType mode, BufferUse use)
{
	if ((static_cast<int>(use) & static_cast<int>(BufferUse::ShaderResource)) == 0)
		return nullptr;

	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIBuffer *me = (piIBuffer*)malloc(sizeof(piIBuffer));
	if (!me)
		return nullptr;

	const unsigned int amount = numElements * elementSize;
	D3D11_BUFFER_DESC desc;
	desc.Usage =D3D11_USAGE_DEFAULT;

desc.Usage = (mode == BufferType::Dynamic) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;

	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = amount;// (amount + 15) & 0xfffffff0; // must be multiple of 16 for constant buffers
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = elementSize;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (data == nullptr)
	{
		HRESULT hr = dev->CreateBuffer(&desc, nullptr, &me->mObject);
		if (FAILED(hr)) return nullptr;
	}
	else
	{
		D3D11_SUBRESOURCE_DATA dataDesc;
		dataDesc.pSysMem = data;
		dataDesc.SysMemPitch = 0;
		dataDesc.SysMemSlicePitch = 0;
		HRESULT hr = dev->CreateBuffer(&desc, &dataDesc, &me->mObject);
		if (FAILED(hr)) return nullptr;
	}


	D3D11_BUFFER_DESC descBuf; ZeroMemory(&descBuf, sizeof(descBuf)); me->mObject->GetDesc(&descBuf);


	D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
	ZeroMemory(&vdesc, sizeof(vdesc));
	vdesc.Format = DXGI_FORMAT_UNKNOWN;
	vdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	vdesc.BufferEx.FirstElement = 0;
	vdesc.BufferEx.NumElements = numElements;

	HRESULT hr = dev->CreateShaderResourceView(me->mObject, &vdesc, &me->mView);
	if (FAILED(hr)) return nullptr;

	me->mUse = use;
	me->mSize = amount;

	return (piBuffer)me;
}

piBuffer piRendererDX11::CreateBufferMapped_Start(void **ptr, unsigned int amount, BufferUse use)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	//ctx->Map(me->mObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	*ptr = mappedResource.pData;

	return nullptr;
}
void piRendererDX11::CreateBufferMapped_End(piBuffer vme)
{
	piIBuffer *me = (piIBuffer*)vme;

	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	//	Re-enable GPU access to the vertex buffer data.
	ctx->Unmap(me->mObject, 0);
}

void piRendererDX11::DestroyBuffer(piBuffer vme)
{
	piIBuffer *me = (piIBuffer*)vme;
	me->mObject->Release();
	free(me);
}

void piRendererDX11::UpdateBuffer(piBuffer obj, const void *data, int offset, int len, bool invalidate)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	// UpdateSubresource CANNOT BE CALLED IF the resource is created with immutable or dynamic usage.

	piIBuffer *me = (piIBuffer *)obj;
	if (offset == 0)
	{
		ctx->UpdateSubresource(me->mObject, 0, NULL, data, len, 0);
	}
	else
	{
		D3D11_BOX box = { static_cast<unsigned int>(offset), 0, 0, static_cast<unsigned int>(offset + len), 1, 1 };
		ctx->UpdateSubresource(me->mObject, 0, &box, data, len, 0);
	}
}

piVertexArray piRendererDX11::CreateVertexArray2(int numStreams,
												piBuffer vb0, const ArrayLayout2 *streamLayout0,
												piBuffer vb1, const ArrayLayout2 *streamLayout1,
	                                            const void *shaderBinary, size_t shaderBinarySize,
	                                            piBuffer ib, const IndexArrayFormat ebFormat)
{
	ID3D11Device *dev = (ID3D11Device*)mDevice;

	piIVertexArray2 *me = (piIVertexArray2*)malloc(sizeof(piIVertexArray2));
	if (!me)
	return nullptr;

	me->mNumStreams = numStreams;
	if (numStreams > 0)
	{
		D3D11_INPUT_ELEMENT_DESC desc[12];
		int numElements = 0;
		for (int j = 0; j < numStreams; j++)
		{
			const ArrayLayout2 * st = (j == 0) ? streamLayout0 : streamLayout1;
			int offset = 0;
			for (int i = 0; i < st->mNumElements; i++)
			{
				desc[numElements].SemanticName = st->mEntry[i].mName;
				desc[numElements].SemanticIndex = 0;
				desc[numElements].Format = iDxFormatEnum(st->mEntry[i].mFormat);
				desc[numElements].InputSlot = j;
				desc[numElements].AlignedByteOffset = offset;
				desc[numElements].InputSlotClass = st->mEntry[i].mPerInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
				desc[numElements].InstanceDataStepRate = st->mEntry[i].mPerInstance ? 1 : 0;

				offset += iDxFormatSize(st->mEntry[i].mFormat);
				numElements++;
			}
			me->mStride[j] = offset;
			me->mBuffer[j] = (j == 0) ? vb0 : vb1;
			piAssert(((piIBuffer*)me->mBuffer[j])->mUse == BufferUse::Vertex);
		}

		HRESULT hr = dev->CreateInputLayout(desc, numElements, shaderBinary, shaderBinarySize, &me->mInputLayout);
		if (FAILED(hr)) return nullptr;
	}
	else
	{
		me->mInputLayout = nullptr;
	}

	me->mIndexBuffer = ib;
	me->mIndexArrayType = ebFormat;
	if (ib)
	{
		piAssert(((piIBuffer*)me->mIndexBuffer)->mUse == BufferUse::Index);
	}

	return (piVertexArray)me;
}

void piRendererDX11::AttachVertexArray2(piVertexArray vme)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;
	piIVertexArray2 *me = (piIVertexArray2*)vme;

	if (me->mNumStreams > 0)
	{
		ID3D11Buffer *vb[2];
		UINT stride[2];
		UINT offset[2];
		for (int i = 0; i < me->mNumStreams; i++)
		{
			stride[i] = me->mStride[i];
			offset[i] = 0;
			vb[i] = ((piIBuffer*)me->mBuffer[i])->mObject;
		}

		ctx->IASetInputLayout(me->mInputLayout); // nullptr for no VA
		ctx->IASetVertexBuffers(0, me->mNumStreams, vb, stride, offset);
	}

	if (me->mIndexBuffer != nullptr)
	{
		ID3D11Buffer *ib = ((piIBuffer*)me->mIndexBuffer)->mObject;
		ctx->IASetIndexBuffer(ib, (me->mIndexArrayType == piRenderer::IndexArrayFormat::UINT_32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
	}
}
void piRendererDX11::DestroyVertexArray2(piVertexArray vme)
{
	piIVertexArray2 *me = (piIVertexArray2*)vme;
	if (me->mInputLayout != nullptr)
	{
		me->mInputLayout->Release();
	}
	free(me);
}

piVertexArray piRendererDX11::CreateVertexArray( int numStreams,
                                                 piBuffer vb0, const piRArrayLayout *streamLayout0,
                                                 piBuffer vb1, const piRArrayLayout *streamLayout1,
                                                 piBuffer eb, const IndexArrayFormat ebFormat)
{
/*
    oglCreateVertexArrays(1, &me->mObjectID);
    if (!me->mObjectID)
        return nullptr;

    unsigned int aid = 0;

    for( int j=0; j<numStreams; j++ )
    {
        unsigned int sid = j;

        //me->mStreams[j] = (j == 0) *streamLayout0 : *streamLayout1;

        const piRArrayLayout * st = (j == 0) ? streamLayout0 : streamLayout1;
        piBuffer vb = (j==0 ) ? vb0 : vb1;


        int offset = 0;
        const int num = st->mNumElements;
        for( int i=0; i<num; i++ )
        {
            oglEnableVertexArrayAttrib(me->mObjectID, aid);
            oglVertexArrayAttribFormat(me->mObjectID, aid, st->mEntry[i].mNumComponents, glType[st->mEntry[i].mType], st->mEntry[i].mNormalize, offset);
            oglVertexArrayAttribBinding(me->mObjectID, aid, sid);

            offset += st->mEntry[i].mNumComponents*glSizeof[st->mEntry[i].mType];
            aid++;
        }

        oglVertexArrayVertexBuffer(me->mObjectID, sid, ((piIBuffer*)vb)->mObjectID, 0, st->mStride);
        //oglVertexArrayBindingDivisor(me->mObjectID, bid, (streamLayout->mDivisor>0) ? streamLayout->mDivisor : 1 );
    }

    if (eb != nullptr )
    oglVertexArrayElementBuffer(me->mObjectID, ((piIBuffer*)eb)->mObjectID);
    return (piVertexArray)me;
	*/
	return nullptr;
}

void piRendererDX11::DestroyVertexArray(piVertexArray vme)
{
    //piIVertexArray *me = (piIVertexArray*)vme;
    //oglDeleteVertexArrays(1, &me->mObjectID);
}

void piRendererDX11::AttachVertexArray(piVertexArray vme)
{
    //piIVertexArray *me = (piIVertexArray*)vme;
    //oglBindVertexArray(me->mObjectID);
}

void piRendererDX11::DettachVertexArray( void )
{
    //oglBindVertexArray( 0 );
}

void piRendererDX11::AttachPixelPackBuffer(piBuffer obj)
{
}

void piRendererDX11::DettachPixelPackBuffer(void)
{
}

static const D3D11_PRIMITIVE_TOPOLOGY pi2dx[] = {
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, //piPT_Triangle
	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, // piPT_Point
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, // piPT_TriangleStrip
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,  // piPT_LineStrip


/*
	piPT_TriPatch = 4,
	piPT_QuadPatch = 5,
	piPT_LinesAdj = 6,
	piPT_LineStripAdj = 7,
	piPT_16Patch = 8,
	piPT_32Patch = 9,
	piPT_Lines = 10
*/
};



void piRendererDX11::DrawPrimitiveIndexed(PrimitiveType pt, uint32_t num, uint32_t numInstances, uint32_t baseVertex, uint32_t baseInstance, uint32_t baseIndex)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	ctx->IASetPrimitiveTopology(pi2dx[static_cast<int>(pt)]);
	ctx->DrawIndexedInstanced(num, numInstances, baseIndex, baseVertex, baseInstance);
}

void piRendererDX11::DrawPrimitiveNotIndexed(PrimitiveType pt, int first, int num, int numInstanced)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	ctx->IASetPrimitiveTopology(pi2dx[static_cast<int>(pt)]);
	ctx->DrawInstanced(num, numInstanced, first, 0);
}

void piRendererDX11::DettachIndirectBuffer(void)
{
}

void piRendererDX11::DrawPrimitiveNotIndexedMultiple(PrimitiveType pt, const int *firsts, const int *counts, int num)
{
	/*
    GLenum glpt = GL_TRIANGLES;

         if (pt == piPT_Triangle)        glpt = GL_TRIANGLES;
    else if (pt == piPT_Point)           glpt = GL_POINTS;
    else if (pt == piPT_TriangleStrip)   glpt = GL_TRIANGLE_STRIP;
    else if (pt == piPT_LineStrip)       glpt = GL_LINE_STRIP;
    else if (pt == piPT_TriPatch)      { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 3); }
    else if (pt == piPT_QuadPatch)     { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 4); }
    else if (pt == piPT_LinesAdj)        glpt = GL_LINES_ADJACENCY;
    else if (pt == piPT_LineStripAdj)    glpt = GL_LINE_STRIP_ADJACENCY;
    else if (pt == piPT_16Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 16); }
    else if (pt == piPT_32Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 32); }
    else if (pt == piPT_Lines)           glpt = GL_LINES;

    oglMultiDrawArrays(glpt,firsts,counts,num);
	*/
}


void piRendererDX11::DrawPrimitiveNotIndexedIndirect(PrimitiveType pt, piBuffer cmds, int num)
{
	/*
    piIBuffer *buf = (piIBuffer *)cmds;

    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf->mObjectID);

    GLenum glpt = GL_TRIANGLES;

         if (pt == piPT_Triangle)        glpt = GL_TRIANGLES;
    else if (pt == piPT_Point)           glpt = GL_POINTS;
    else if (pt == piPT_TriangleStrip)   glpt = GL_TRIANGLE_STRIP;
    else if (pt == piPT_LineStrip)       glpt = GL_LINE_STRIP;
    else if (pt == piPT_TriPatch)      { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 3); }
    else if (pt == piPT_QuadPatch)     { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 4); }
    else if (pt == piPT_LinesAdj)        glpt = GL_LINES_ADJACENCY;
    else if (pt == piPT_LineStripAdj)    glpt = GL_LINE_STRIP_ADJACENCY;
    else if (pt == piPT_16Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 16); }
    else if (pt == piPT_32Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 32); }
    else if (pt == piPT_Lines)           glpt = GL_LINES;

    oglMultiDrawArraysIndirect(glpt, 0, num, sizeof(piDrawArraysIndirectCommand));
    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	*/
}



void piRendererDX11::DrawPrimitiveIndirect(PrimitiveType pt, piBuffer cmds, uint32_t offset, uint32_t num)
{
	/*
    piIBuffer *buf = (piIBuffer *)cmds;

    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf->mObjectID);

    GLenum glpt = GL_TRIANGLES;

         if (pt == piPT_Triangle)        glpt = GL_TRIANGLES;
    else if (pt == piPT_Point)           glpt = GL_POINTS;
    else if (pt == piPT_TriangleStrip)   glpt = GL_TRIANGLE_STRIP;
    else if (pt == piPT_LineStrip)       glpt = GL_LINE_STRIP;
    else if (pt == piPT_TriPatch)      { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 3); }
    else if (pt == piPT_QuadPatch)     { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 4); }
    else if (pt == piPT_LinesAdj)        glpt = GL_LINES_ADJACENCY;
    else if (pt == piPT_LineStripAdj)    glpt = GL_LINE_STRIP_ADJACENCY;
    else if (pt == piPT_16Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 16); }
    else if (pt == piPT_32Patch)       { glpt = GL_PATCHES; oglPatchParameteri(GL_PATCH_VERTICES, 32); }
    else if (pt == piPT_Lines)           glpt = GL_LINES;

    oglMultiDrawElementsIndirect(glpt, GL_UNSIGNED_INT, 0, num, sizeof(piDrawElementsIndirectCommand));
    oglBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	*/
}

void piRendererDX11::DrawUnitQuad_XY( int numInstanced )
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	piIVertexArray2 *me = (piIVertexArray2*)(mVA[0]);
	ID3D11Buffer *vb = ((piIBuffer*)me->mBuffer[0])->mObject;
	UINT stride = me->mStride[0];
	UINT offset = 0;
	ctx->IASetInputLayout(me->mInputLayout);
	ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->DrawInstanced(4, numInstanced, 0, 0);
}

void piRendererDX11::DrawUnitCube_XYZ_NOR(int numInstanced)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	piIVertexArray2 *me = (piIVertexArray2*)(mVA[1]);
	ID3D11Buffer *vb = ((piIBuffer*)me->mBuffer[0])->mObject;
	UINT stride = me->mStride[0];
	UINT offset = 0;
	ctx->IASetInputLayout(me->mInputLayout);
	ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->DrawInstanced(4, numInstanced, 0, 0);
	ctx->DrawInstanced(4, numInstanced, 4, 0);
	ctx->DrawInstanced(4, numInstanced, 8, 0);
	ctx->DrawInstanced(4, numInstanced, 12, 0);
	ctx->DrawInstanced(4, numInstanced, 16, 0);
	ctx->DrawInstanced(4, numInstanced, 20, 0);

}

void piRendererDX11::DrawUnitCube_XYZ(int numInstanced)
{
	ID3D11DeviceContext *ctx = (ID3D11DeviceContext*)mDeviceContext;

	piIVertexArray2 *me = (piIVertexArray2*)(mVA[2]);
	ID3D11Buffer *vb = ((piIBuffer*)me->mBuffer[0])->mObject;
	UINT stride = me->mStride[0];
	UINT offset = 0;
	ctx->IASetInputLayout(me->mInputLayout);
	ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	ctx->DrawInstanced(4, numInstanced, 0, 0);
	ctx->DrawInstanced(4, numInstanced, 4, 0);
	ctx->DrawInstanced(4, numInstanced, 8, 0);
	ctx->DrawInstanced(4, numInstanced,12, 0);
	ctx->DrawInstanced(4, numInstanced,16, 0);
	ctx->DrawInstanced(4, numInstanced,20, 0);
}

void piRendererDX11::ExecuteCompute(int tx, int ty, int tz, int gsx, int gsy, int gsz)
{
    //int ngx = tx / gsx; if( (ngx*gsx) < tx ) ngx++;
    //int ngy = ty / gsy; if( (ngy*gsy) < ty ) ngy++;
    //int ngz = tz / gsz; if( (ngz*gsz) < tz ) ngz++;
    //oglDispatchCompute( ngx, ngy, ngz );
}

void piRendererDX11::SetLineWidth( float size )
{
    //glLineWidth( size );
}

void piRendererDX11::SetPointSize( bool mode, float size )
{
	/*
	if( mode )
	{
		glEnable( GL_PROGRAM_POINT_SIZE );
        glPointSize( size );
	}
	else
		glDisable( GL_PROGRAM_POINT_SIZE );
	*/
}

void piRendererDX11::GetTextureRes( piTexture vme, int *res )
{
	//piITexture *me = (piITexture*)vme;
	//res[0] = me->mInfo.mXres;
	//res[1] = me->mInfo.mYres;
	//res[2] = me->mInfo.mZres;

}

void piRendererDX11::GetTextureFormat( piTexture vme, Format *format )
{
	//piITexture *me = (piITexture*)vme;
    //format[0] = me->mInfo.mFormat;
}


void piRendererDX11::GetTextureInfo( piTexture vme, TextureInfo *info )
{
	//piITexture *me = (piITexture*)vme;
    //info[0] = me->mInfo;
    //info->mDeleteMe = me->mObjectID;
}


void piRendererDX11::GetTextureSampling(piTexture vme, TextureFilter *rfilter, TextureWrap *rwrap)
{
    //piITexture *me = (piITexture*)vme;
    //rfilter[0] = me->mFilter;
    //rwrap[0] = me->mWrap;
}


void piRendererDX11::GetTextureContent( piTexture vme, void *data, const Format fmt )
{
	/*
	piITexture *me = (piITexture*)vme;
    int     	 mode, mode2, mode3, bpp;

    if( !format2gl( fmt, &bpp, &mode, &mode2, &mode3, me->mInfo.mCompressed ) )
        return;

    oglGetTextureImage(me->mObjectID, 0, mode, mode3, me->mInfo.mXres*me->mInfo.mYres*me->mInfo.mZres * bpp, data);
	*/
}

void piRendererDX11::GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres)
{
	/*
    piITexture *me = (piITexture*)vme;
    int     	 exteriorFormat, internalFormat, ftype, bpp;

    if (!format2gl(me->mInfo.mFormat, &bpp, &exteriorFormat, &internalFormat, &ftype, me->mInfo.mCompressed))
        return;

    oglGetTextureSubImage( me->mObjectID,
                           0,
                           x, y, z, xres, yres, zres,
                           exteriorFormat, ftype,
                           xres*yres*zres*bpp, data );
						   */
}

void piRendererDX11::PolygonOffset( bool mode, bool wireframe, float a, float b )
{
	/*
    if( mode )
    {
        glEnable( wireframe?GL_POLYGON_OFFSET_LINE:GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( a, b );
    }
    else
    {
        glDisable( wireframe?GL_POLYGON_OFFSET_LINE:GL_POLYGON_OFFSET_FILL );
    }
	*/
}

void piRendererDX11::RenderMemoryBarrier(BarrierType type)
{
	/*
    GLbitfield bf = 0;

    if( type & piBARRIER_SHADER_STORAGE ) bf |= GL_SHADER_STORAGE_BARRIER_BIT;
    if( type & piBARRIER_UNIFORM        ) bf |= GL_UNIFORM_BARRIER_BIT;
    if( type & piBARRIER_ATOMICS        ) bf |= GL_ATOMIC_COUNTER_BARRIER_BIT;
    if( type & piBARRIER_IMAGE          ) bf |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    if (type & piBARRIER_COMMAND        ) bf |= GL_COMMAND_BARRIER_BIT;
    if (type & piBARRIER_TEXTURE        ) bf |= GL_TEXTURE_UPDATE_BARRIER_BIT;

    if( type == piBARRIER_ALL) bf = GL_ALL_BARRIER_BITS;

    oglMemoryBarrier(bf);
	*/
}

//--- queries ---
piQuery piRendererDX11::CreateQuery(piRenderer::QueryType type)
{
    return nullptr;
}

void piRendererDX11::DestroyQuery(piQuery vme)
{
}

void piRendererDX11::BeginQuery(piQuery vme)
{
}

void piRendererDX11::EndQuery(piQuery vme)
{
}

uint64_t piRendererDX11::GetQueryResult(piQuery vme)
{
    return 0;
}

void piRendererDX11::CreateSyncObject(piBuffer &buffer)
{
}

bool piRendererDX11::CheckSyncObject(piBuffer &buffer)
{
    return false;
}

}
