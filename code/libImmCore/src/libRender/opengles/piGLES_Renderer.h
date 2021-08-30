#pragma once

#include "../piRenderer.h"

namespace ImmCore {

class piRendererGLES : public piRenderer
{
public:
    piRendererGLES();
    ~piRendererGLES() = default;

    bool      Initialize(int id, const void **hwnd, int num, bool disableVSync, bool disableErrors, piReporter *reporter, bool createDevice, void *device);
    void      Deinitialize( void );
    bool      SupportsFeature(RendererFeature feature);
    API       GetAPI(void);
    void      Report( void );
    void      SetActiveWindow( int id );
    void      Enable(void);
    void      Disable(void);
    void      SwapBuffers( void );
    void *    GetContext(void);

    //--- performance measurement --- (we measure performance in a ping-ping manner to prevent stalling the GPU on the queries)
    void      StartPerformanceMeasure(void);
    void      EndPerformanceMeasure(void);
    uint64_t  GetPerformanceMeasure(void); // this will get the performance of the previous frame


    //--- render targets ---
    piRTarget CreateRenderTarget(piTexture vtex0, piTexture vtex1, piTexture vtex2, piTexture vtex3, piTexture zbuf);
    void      DestroyRenderTarget( piRTarget obj );
    bool      SetRenderTarget( piRTarget obj );
    void      RenderTargetSampleLocations(piRTarget vdst, const float *locations);
    void      BlitRenderTarget( piRTarget dst, piRTarget src, bool color, bool depth );
    void      SetWriteMask( bool c0, bool c1, bool c2, bool c3, bool z );
    void      SetShadingSamples( int shadingSamples );
    void      RenderTargetGetDefaultSampleLocation(piRTarget vdst, const int id, float *location);

    //--- general ---
    void      Clear( const float *color0, const float *color1, const float *color2, const float *color3, const bool depth0 );
    void      SetState( piState state, bool value );
    void      SetBlending(int buf, BlendEquation equRGB, BlendOperations srcRGB, BlendOperations dstRGB,
                                   BlendEquation equALP, BlendOperations srcALP, BlendOperations dstALP);
    void      SetViewport(int id, const int *vp);
    void      SetViewports(int num, const float *viewports); // x,y,w,h,zmin,zmax // 0.0, 0.0, 800.0, 600.0, 0.0, 1.0
    void      GetViewports(int *num, float *viewports); // x,y,w,h,zmin,zmax

    piRasterState CreateRasterState(bool wireframe, bool frontIsCounterClockWise, CullMode cullMode, bool depthClamp, bool multiSample);
    void      SetRasterState(const piRasterState vme);
    void      DestroyRasterState( piRasterState vme);

    piBlendState CreateBlendState(bool alphaToCoverage, bool enabled0);
    void      SetBlendState(const piBlendState vme);
    void      DestroyBlendState(piBlendState vme);

    piDepthState CreateDepthState(bool alphaToCoverage, bool lessEqual);
    void SetDepthState(const piDepthState vme);
    void DestroyDepthState(piDepthState vme);

    //--- textures ---
    piTexture CreateTexture( const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap, float aniso, const void *buffer );
    piTexture CreateTexture2(const wchar_t *key, const TextureInfo *info, bool compress, TextureFilter filter, TextureWrap wrap1, float aniso, const void *buffer, int bindUsage);
    void      DestroyTexture( piTexture obj );
    void      AttachTextures( int num, piTexture vt0, piTexture vt1, piTexture vt2, piTexture vt3, piTexture vt4, piTexture vt5, piTexture vt6, piTexture vt7, piTexture vt8, piTexture vt9, piTexture vt10, piTexture vt11, piTexture vt12, piTexture vt13, piTexture vt14, piTexture vt15 );
    void      AttachTextures( int num, piTexture *vt, int offset );
    void      DettachTextures( void );
    void      ClearTexture( piTexture vme, int level, const void *data );
    void      UpdateTexture( piTexture me, int x0, int y0, int z0, int xres, int yres, int zres, const void *buffer );
    void      GetTextureRes( piTexture me, int *res );
    void      GetTextureFormat( piTexture me, Format *format );
    void      GetTextureContent( piTexture me, void *data, const Format fmt );
    void      GetTextureContent(piTexture vme, void *data, int x, int y, int z, int xres, int yres, int zres);
    void      GetTextureInfo( piTexture me, TextureInfo *info );
    void      GetTextureSampling(piTexture vme, TextureFilter *rfilter, TextureWrap *rwrap);
    piTexture CreateTextureFromID(unsigned int id, TextureFilter filter);
    void      ComputeMipmaps( piTexture me );
    void      MakeResident( piTexture vme );
    void      MakeNonResident( piTexture vme );
    uint64_t  GetTextureHandle( piTexture vme );


    piSampler CreateSampler(TextureFilter filter, TextureWrap wrap, float anisotropy);
    void      DestroySampler( piSampler obj );
    void      AttachSamplers(int num, piSampler vt0, piSampler vt1, piSampler vt2, piSampler vt3, piSampler vt4, piSampler vt5, piSampler vt6, piSampler vt7);
    void      DettachSamplers( void );

    void      AttachImage(int unit, piTexture texture, int level, bool layered, int layer, Format format);

    //--- shaders ---
    piShader  CreateShader( const piShaderOptions *options, const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error);
    piShader  CreateShaderBinary(const piShaderOptions *options, const uint8_t *vs, const int vs_len, const uint8_t *cs, const int cs_len, const uint8_t *es, const int es_len, const uint8_t *gs, const int gs_len, const uint8_t *fs, const int fs_len, char *error);
    //piShader  CreateShaderRaw(const char *vs, const char *cs, const char *es, const char *gs, const char *fs, char *error);
    void      DestroyShader( piShader obj );
    void      AttachShader( piShader obj );
    void      DettachShader( void );

    piShader  CreateCompute(const piShaderOptions *options, const char *cs, char *error);

    void      AttachShaderConstants(piBuffer obj, int unit);
    void      SetShaderConstant4F(const unsigned int pos, const float *value, int num);
    void      SetShaderConstant3F(const unsigned int pos, const float *value, int num);
    void      SetShaderConstant2F(const unsigned int pos, const float *value, int num);
    void      SetShaderConstant1F(const unsigned int pos, const float *value, int num);
    void      SetShaderConstant1I(const unsigned int pos, const int *value, int num);
    void      SetShaderConstant1UI(const unsigned int pos, const unsigned int *value, int num);
    void      SetShaderConstant2UI(const unsigned int pos, const unsigned int *value, int num);
    void      SetShaderConstant3UI(const unsigned int pos, const unsigned int *value, int num);
    void      SetShaderConstant4UI(const unsigned int pos, const unsigned int *value, int num);
    void      SetShaderConstantMat4F(const unsigned int pos, const float *value, int num, bool transpose);
    void      SetShaderConstantSampler(const unsigned int pos, int unit);
    void      AttachShaderBuffer(piBuffer obj, int unit);
    void      DettachShaderBuffer(int unit);
    void      AttachAtomicsBuffer(piBuffer obj, int unit);
    void      DettachAtomicsBuffer(int unit);

    //--- buffers (vertex, index, shader constants) ---
    piBuffer  CreateBuffer(const void *data, unsigned int amount, BufferType mode, BufferUse use);
    piBuffer  CreateStructuredBuffer(const void *data, unsigned int numElements, unsigned int elementSize, BufferType mode, BufferUse use);
    piBuffer  CreateBufferMapped_Start(void **ptr, unsigned int amount, BufferUse use);
    void      CreateBufferMapped_End(piBuffer vme);
    void      DestroyBuffer(piBuffer);
    void      UpdateBuffer(piBuffer obj, const void *data, int offset, int len, bool invalidate);

    void      AttachPixelPackBuffer(piBuffer obj);
    void      DettachPixelPackBuffer(void);

    //--- arrays ---
    piVertexArray CreateVertexArray(int numStreams, piBuffer vb0, const piRArrayLayout *streamLayout0, piBuffer vb1, const piRArrayLayout *streamLayout1, piBuffer eb, const IndexArrayFormat ebFormat);
    void      DestroyVertexArray(piVertexArray obj);
    void      AttachVertexArray(piVertexArray obj);
    void      DettachVertexArray(void);

    piVertexArray CreateVertexArray2(int numStreams, piBuffer vb0, const ArrayLayout2 *streamLayout0, piBuffer vb1, const ArrayLayout2 *streamLayout1, const void *shaderBinary, size_t shaderBinarySize, piBuffer ib, const IndexArrayFormat ebFormat);
    void AttachVertexArray2(piVertexArray vme);
    void DestroyVertexArray2(piVertexArray vme);

    //--- queries ---
    piQuery CreateQuery(piRenderer::QueryType type);
    void DestroyQuery(piQuery vme);
    void BeginQuery(piQuery vme);
    void EndQuery(piQuery vme);
    uint64_t GetQueryResult(piQuery vme);

    //--- misc ---

	void      DrawPrimitiveIndexed(PrimitiveType pt, uint32_t num, uint32_t numInstances, uint32_t baseVertex, uint32_t baseInstance, uint32_t baseIndex);
    void      DrawPrimitiveIndirect(PrimitiveType pt, piBuffer cmds, uint32_t offset, uint32_t num);
    void      DrawPrimitiveNotIndexed(PrimitiveType pt, int first, int num, int numInstanced);
    void      DrawPrimitiveNotIndexedMultiple(PrimitiveType pt, const int *firsts, const int *counts, int num);
    void      DrawPrimitiveNotIndexedIndirect(PrimitiveType pt, piBuffer cmds, int num);
    void      DettachIndirectBuffer(void);

    void      DrawUnitCube_XYZ_NOR(int numInstanced);
    void      DrawUnitCube_XYZ(int numInstanced);
    void      DrawUnitQuad_XY(int numInstanced);
    void      DrawTestSquare(); // TODO: remove

    void      ExecuteCompute(int ngx, int ngy, int ngz, int gsx, int gsy, int gsz);

    void      CreateSyncObject(piBuffer &buffer);
    bool      CheckSyncObject(piBuffer &buffer);


    //--- misc ---

    void      SetPointSize( bool mode, float size ); // if false, the v/g shader decides
    void      SetLineWidth( float size );
    void      PolygonOffset( bool mode, bool wireframe, float a, float b );

    void      RenderMemoryBarrier(BarrierType type);

private:
    void      PrintInfo( int showExtensions );

protected:
    piReporter *mReporter;
    int               mID = -1;
    piRTarget         mBindedTarget;
    piVertexArray     mBoundVertexArray;


    uint64_t mMemCurrent = 0;
    uint64_t mMemPeak = 0;
    int      mNumCurrent = 0;
    int      mNumPeak = 0;
    int     mNumShaders = 0;
    int     mPeakShaders =0;

    // aux geometry... (cubes, quads, etc)
    piVertexArray mVA[3];
    piBuffer      mVBO[3];

    bool mFeatureVertexViewport;
    bool mFeatureViewportArray;

    static const int QUERY_BUFFER_SIZE = 2;
    piQuery mPerfQueries[QUERY_BUFFER_SIZE];
    int     mNextPerformanceQuery = 0;
    int     mNumPerformanceQueries = 0;


    struct SavedRenderState
    {
        piRenderer::CullMode mCullMode = CullMode::NONE;
        bool mFrontSideIsCounterClockWise = true;
        bool mDepthClamp = false;
        bool mDepthTest = false;
        bool mBlend = false;

        SavedRenderState & operator=(const SavedRenderState & other)
        {
            mCullMode = other.mCullMode;
            mFrontSideIsCounterClockWise = other.mFrontSideIsCounterClockWise;
            mDepthClamp = other.mDepthClamp;
            mDepthTest = other.mDepthTest;
            mBlend = other.mBlend;

            return *this;
        }
    };

    SavedRenderState mCurrentRenderState = SavedRenderState();
};

} // namespace ImmCore
