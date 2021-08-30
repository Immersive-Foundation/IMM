#pragma once

#include "VrApi.h"

#include <map>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>


#if !defined( EGL_OPENGL_ES3_BIT_KHR )
#define EGL_OPENGL_ES3_BIT_KHR      0x0040
#endif

// QCOM_binning_control2
#define GL_BINNING_CONTROL_HINT_QCOM           0x8FB0

#define GL_BINNING_QCOM                        0x8FB1
#define GL_VISIBILITY_OPTIMIZED_BINNING_QCOM   0x8FB2
#define GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM   0x8FB3
#define GL_DONT_CARE                           0x1100

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER          0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR     0x1004
#endif

#if !defined( GL_EXT_multisampled_render_to_texture )
typedef void (GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif

#if !defined( GL_OVR_multiview )
static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR       = 0x9630;
static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR = 0x9632;
static const int GL_MAX_VIEWS_OVR                                      = 0x9631;
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);
#endif

#if !defined( GL_OVR_multiview_multisampled_render_to_texture )
typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);
#endif

// Must use EGLSyncKHR because the VrApi still supports OpenGL ES 2.0
#define EGL_SYNC

namespace ExePlayer {

//#define CHECK_GL_ERRORS 1

#ifdef CHECK_GL_ERRORS

void CheckGLErrorsAtLine(const char * file, int line);

void onGLError(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const char* message, void *userParam);

void enableDebugMessageCallback();

#define GL( func )      func; CheckGLErrorsAtLine(__FILE__, __LINE__);

#else

#define GL( func )      func;

#endif

bool enabledMultiView();
void setAssetDirectory(const char * androidAssetDirectory);
const char * getAssetDirectory();
char * readFile(const char *filename);
GLuint loadShader(GLenum shaderType, const char** pSource);
GLuint createProgram(
        const char* pVertexSource, const char* pFragmentSource,
        GLuint & vertexShader, GLuint & fragmentShader, std::map<std::string, int> & compileDefines);
GLuint GetGLAttributeLocation(GLuint programHandle, const char * label);
void SetShaderConstantMatrix4f(
        GLuint program, const char * name, float * matrix, bool transpose = false);
void SetShaderConstant1f(
        GLuint program, const char * name, float value);
void SetShaderConstant1i(
        GLuint program, const char * name, float value);

void EglInitExtensions();
const char * EglErrorString( const EGLint error );
const char * GlFrameBufferStatusString( GLenum status );

typedef struct
{
    bool multi_view;                    // GL_OVR_multiview, GL_OVR_multiview2
    bool EXT_texture_border_clamp;      // GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
} OpenGLExtensions_t;

OpenGLExtensions_t getGLExtensions();

/*
================================================================================

ovrEgl

================================================================================
*/

typedef struct
{
    EGLint      MajorVersion;
    EGLint      MinorVersion;
    EGLDisplay  Display;
    EGLConfig   Config;
    EGLSurface  TinySurface;
    EGLSurface  MainSurface;
    EGLContext  Context;
} ovrEgl;

void ovrEgl_Clear( ovrEgl * egl );
void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl );
void ovrEgl_DestroyContext( ovrEgl * egl );

typedef struct
{
    int                     Width;
    int                     Height;
    int                     Multisamples;
    int                     TextureSwapChainLength;
    int                     TextureSwapChainIndex;
    bool                    UseMultiview;
    ovrTextureSwapChain *   ColorTextureSwapChain;
    GLuint *                DepthBuffers;
    GLuint *                FrameBuffers;
} ovrFramebuffer;

void ovrFramebuffer_Clear( ovrFramebuffer * frameBuffer );
bool ovrFramebuffer_Create( ovrFramebuffer * frameBuffer, const bool useMultiview, const ovrTextureFormat colorFormat, const int width, const int height, const int multisamples );
void ovrFramebuffer_Destroy( ovrFramebuffer * frameBuffer );
void ovrFramebuffer_SetCurrent( ovrFramebuffer * frameBuffer );
void ovrFramebuffer_SetNone();
void ovrFramebuffer_Resolve( ovrFramebuffer * frameBuffer );
void ovrFramebuffer_Advance( ovrFramebuffer * frameBuffer );

typedef struct
{
#if defined( EGL_SYNC )
    EGLDisplay  Display;
    EGLSyncKHR  Sync;
#else
    GLsync      Sync;
#endif
} ovrFence;

void ovrFence_Create( ovrFence * fence );
void ovrFence_Destroy( ovrFence * fence );
void ovrFence_Insert( ovrFence * fence );

} // namespace Quill
