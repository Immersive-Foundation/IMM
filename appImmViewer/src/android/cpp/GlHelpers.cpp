#include "GlHelpers.h"

#include <string>

#include <android/log.h>

#define ALOGV_TAG "GlHelpers"

#define ALOGF(...) { __android_log_print( ANDROID_LOG_ERROR, ALOGV_TAG, __VA_ARGS__ ); abort(); }
#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, ALOGV_TAG, __VA_ARGS__ )
#define ALOGW(...) __android_log_print( ANDROID_LOG_WARN, ALOGV_TAG, __VA_ARGS__ )
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, ALOGV_TAG, __VA_ARGS__ )

namespace ExePlayer {

#ifdef CHECK_GL_ERRORS

// GLDEBUG
typedef void(
    GL_APIENTRYP PFNGLDEBUGMESSAGECALLBACKKHRPROC)(GLDEBUGPROCKHR callback, const void* userParam);

typedef void(GL_APIENTRYP PFNGLDEBUGMESSAGECONTROLKHRPROC)(
    GLenum source,
    GLenum type,
    GLenum severity,
    GLsizei count,
    const GLuint* ids,
    GLboolean enabled);


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

bool GL_CheckErrors( const char * logTitle )
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
        ALOGW( "%s GL Error: %s", ( logTitle != nullptr ) ? logTitle : "<untitled>", GL_ErrorForEnum( err ) );
        if ( err == GL_OUT_OF_MEMORY )
        {
            ALOGF( "GL_OUT_OF_MEMORY" );
        }
    } while ( 1 );
    return hadError;
}

void CheckGLErrorsAtLine(const char * file, int line)
{
    GL_CheckErrors(StringUtils::Va("%s:%i", file, line));
}

void onGLError(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const GLchar* message, const void *userParam)
{

  ALOGV("GLDEBUG: %d %d %d %d %.*s",
      (int)source,
      (int)type,
      (int)id,
      (int)severity,
      (int)length,
      message);
}

void enableDebugMessageCallback()
{
    PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR_ =
        (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
    PFNGLDEBUGMESSAGECONTROLKHRPROC glDebugMessageControlKHR_ =
        (PFNGLDEBUGMESSAGECONTROLKHRPROC)eglGetProcAddress("glDebugMessageControlKHR");

    ALOGV("glDebugMessageCallbackKHR_ %p %p", glDebugMessageCallbackKHR_, glDebugMessageControlKHR_);
    // set debug handler and enable output
    if (glDebugMessageCallbackKHR_ != nullptr) {
        glDebugMessageCallbackKHR_(onGLError, nullptr);
        glEnable(GL_DEBUG_OUTPUT_KHR);
    }
    // attempt to enable all debug output
    if (glDebugMessageControlKHR_ != nullptr) {
        glDebugMessageControlKHR_(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
    // glDebugMessageCallbackKHR(onGLError, NULL);
}

#endif

static std::string assetDirectory = "";

void setAssetDirectory(const char * androidAssetDirectory)
{
    assetDirectory = androidAssetDirectory;
}

const char * getAssetDirectory()
{
    return assetDirectory.c_str();
}

char * readFile(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        ALOGF("Failed to open file %s\n", filename);
        exit (1);
    }
    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)calloc(length + 1, sizeof(char));
    if (!data)
    {
        ALOGF("Failed to allocate memory for file data %s\n", filename);
        exit(1);
    }
    size_t read = fread(data, sizeof(char), length, file);
    if (read != length)
    {
        ALOGF("Failed to read whole file %s\n", filename);
        exit(1);
    }
    data[length] = '\0';
    fclose(file);
    return data;
}

GLuint loadShader(GLenum shaderType, const char** pSource)
{
    GLuint shader = GL( glCreateShader(shaderType) );
    if (shader)
    {
        GL( glShaderSource(shader, 3, pSource, NULL) );
        GL( glCompileShader(shader) );
        GLint compiled = 0;
        GL( glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled) );
        if (!compiled)
        {
            GLint infoLen = 0;
            GL( glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen) );
            if (infoLen)
            {
                char *buf = (char *) malloc(infoLen);
                if (buf)
                {
                    GL( glGetShaderInfoLog(shader, infoLen, NULL, buf) );
                    ALOGW("Could not compile shader %d:\n%s\n",
                         shaderType, buf);
                    free(buf);
                }
                GL( glDeleteShader(shader) );
                shader = 0;
            }
        }
    }
    return shader;
}

static const char * versionStr = "#version 310 es\n";

GLuint createProgram(
        const char* pVertexSource, const char* pFragmentSource,
        GLuint & vertexShader, GLuint & fragmentShader, std::map<std::string, int> & compileDefines)
{
    const GLchar * vertexSource[3] = { versionStr, "\n", pVertexSource };
    const GLchar * fragmentSource[3] = { versionStr, "\n", pFragmentSource };
    char defineStr[80 * compileDefines.size()];

    if (!compileDefines.empty())
    {
        int offset = 0;
        for (auto const& define : compileDefines)
        {
            offset = snprintf(defineStr, sizeof(defineStr), "#define %s %d\n", define.first.c_str(), define.second);
        }
        defineStr[offset] = 0;

        vertexSource[1] = defineStr;
        fragmentSource[1] = defineStr;
    }

    vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader)
    {
        return 0;
    }

    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader)
    {
        return 0;
    }

    GLuint program = GL( glCreateProgram() );
    if (program)
    {
        GL( glAttachShader(program, vertexShader) );
        GL( glAttachShader(program, fragmentShader) );
        GL( glLinkProgram(program) );
        GLint linkStatus = GL_FALSE;
        GL( glGetProgramiv(program, GL_LINK_STATUS, &linkStatus) );
        if (linkStatus != GL_TRUE)
        {
            GLint bufLength = 0;
            GL( glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength) );
            if (bufLength)
            {
                char* buf = (char*) malloc(bufLength);
                if (buf)
                {
                    GL( glGetProgramInfoLog(program, bufLength, NULL, buf) );
                    ALOGW("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            GL( glDeleteProgram(program) );
            program = 0;
        }
    }
    return program;
}

GLuint GetGLAttributeLocation(GLuint programHandle, const char * label)
{
    GLint location = GL( glGetAttribLocation(programHandle, label) );

    if (location < 0)
    {
        ALOGW("Could not locate %s in program", label);
        return -1;
    }

    return location;
}

void SetShaderConstantMatrix4f(
        GLuint program, const char * name, float * matrix, bool transpose)
{
    const GLint pos = GL( glGetUniformLocation(program, name) );
    if (pos < 0)
    {
        ALOGW("Could not find uniform %s", name);
        return;
    }

    GL( glUniformMatrix4fv(pos, 1, transpose, matrix) );
}

void SetShaderConstant1f(
        GLuint program, const char * name, float value)
{
    const GLint pos = GL( glGetUniformLocation(program, name) );
    if (pos < 0)
    {
        ALOGW("Could not find uniform %s", name);
        return;
    }

    GL( glUniform1f(pos, value) );
}

void SetShaderConstant1i(
        GLuint program, const char * name, float value)
{
    const GLint pos = GL( glGetUniformLocation(program, name) );
    if (pos < 0)
    {
        ALOGW("Could not find uniform %s", name);
        return;
    }

    GL( glUniform1i(pos, value) );
}

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

#if defined EGL_SYNC
// EGL_KHR_reusable_sync
PFNEGLCREATESYNCKHRPROC         eglCreateSyncKHR;
PFNEGLDESTROYSYNCKHRPROC        eglDestroySyncKHR;
PFNEGLCLIENTWAITSYNCKHRPROC     eglClientWaitSyncKHR;
PFNEGLSIGNALSYNCKHRPROC         eglSignalSyncKHR;
PFNEGLGETSYNCATTRIBKHRPROC      eglGetSyncAttribKHR;
#endif

OpenGLExtensions_t glExtensions;

OpenGLExtensions_t getGLExtensions()
{
    return glExtensions;
}

void EglInitExtensions()
{
#if defined EGL_SYNC
    eglCreateSyncKHR        = (PFNEGLCREATESYNCKHRPROC)         eglGetProcAddress( "eglCreateSyncKHR" );
    eglDestroySyncKHR       = (PFNEGLDESTROYSYNCKHRPROC)        eglGetProcAddress( "eglDestroySyncKHR" );
    eglClientWaitSyncKHR    = (PFNEGLCLIENTWAITSYNCKHRPROC)     eglGetProcAddress( "eglClientWaitSyncKHR" );
    eglSignalSyncKHR        = (PFNEGLSIGNALSYNCKHRPROC)         eglGetProcAddress( "eglSignalSyncKHR" );
    eglGetSyncAttribKHR     = (PFNEGLGETSYNCATTRIBKHRPROC)      eglGetProcAddress( "eglGetSyncAttribKHR" );
#endif

    const char * allExtensions = (const char *)glGetString( GL_EXTENSIONS );
    if ( allExtensions != NULL )
    {
        glExtensions.multi_view = strstr( allExtensions, "GL_OVR_multiview2" ) &&
                                  strstr( allExtensions, "GL_OVR_multiview_multisampled_render_to_texture" );

        glExtensions.EXT_texture_border_clamp = strstr( allExtensions, "GL_EXT_texture_border_clamp" ) ||
                                                strstr( allExtensions, "GL_OES_texture_border_clamp" );
    }
}

const char * EglErrorString( const EGLint error )
{
    switch ( error )
    {
        case EGL_SUCCESS:               return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:       return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:             return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:         return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:           return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:   return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:           return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:           return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:             return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:         return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:     return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:     return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:          return "EGL_CONTEXT_LOST";
        default:                        return "unknown";
    }
}

const char * GlFrameBufferStatusString( GLenum status )
{
    switch ( status )
    {
        case GL_FRAMEBUFFER_UNDEFINED:                      return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:                    return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:                                            return "unknown";
    }
}

/*
================================================================================

ovrEgl

================================================================================
*/

void ovrEgl_Clear( ovrEgl * egl )
{
    egl->MajorVersion = 0;
    egl->MinorVersion = 0;
    egl->Display = 0;
    egl->Config = 0;
    egl->TinySurface = EGL_NO_SURFACE;
    egl->MainSurface = EGL_NO_SURFACE;
    egl->Context = EGL_NO_CONTEXT;
}

void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
{
    if ( egl->Display != 0 )
    {
        return;
    }

    egl->Display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    ALOGV( "        eglInitialize( Display, &MajorVersion, &MinorVersion )" );
    eglInitialize( egl->Display, &egl->MajorVersion, &egl->MinorVersion );
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    const EGLint configAttribs[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8, // need alpha for the multi-pass timewarp compositor
        EGL_DEPTH_SIZE,     0, // TODO: verify how many bits we need here
        EGL_STENCIL_SIZE,   0, // Couldn't find eglConfig with 24bit depth and no stencil
        EGL_SAMPLES,        0,
        EGL_NONE
    };

    if ( eglGetConfigs( egl->Display, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE )
    {
        ALOGW( "        eglGetConfigs() failed: %s", EglErrorString( eglGetError() ) );
    }

    ALOGV("Found %i chosen configs", numConfigs);

    egl->Config = 0;
    for ( int i = 0; i < numConfigs; i++ )
    {
        EGLint value = 0;

        eglGetConfigAttrib( egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value );
        if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
        {
            continue;
        }

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib( egl->Display, configs[i], EGL_SURFACE_TYPE, &value );
        if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
        {
            continue;
        }

        int j = 0;
        for ( ; configAttribs[j] != EGL_NONE; j += 2 )
        {
            eglGetConfigAttrib( egl->Display, configs[i], configAttribs[j], &value );
            if ( value != configAttribs[j + 1] )
            {
                ALOGV("for config %i attribute at %i does not match requirement... expected %i got %i", i, j, configAttribs[j + 1], value);
                break;
            }
        }
        if ( configAttribs[j] == EGL_NONE )
        {
            egl->Config = configs[i];
            break;
        }
    }
    if ( egl->Config == 0 )
    {
        ALOGF( "        eglChooseConfig() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
        EGL_NONE
    };
    ALOGV( "        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )" );
    egl->Context = eglCreateContext( egl->Display, egl->Config, ( shareEgl != NULL ) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs );
    if ( egl->Context == EGL_NO_CONTEXT )
    {
        ALOGF( "        eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    const EGLint surfaceAttribs[] =
    {
        EGL_WIDTH, 16,
        EGL_HEIGHT, 16,
        EGL_NONE
    };
    ALOGV( "        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )" );
    egl->TinySurface = eglCreatePbufferSurface( egl->Display, egl->Config, surfaceAttribs );
    if ( egl->TinySurface == EGL_NO_SURFACE )
    {
        ALOGV( "        eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
    ALOGV( "        eglMakeCurrent( Display, TinySurface, TinySurface, Context )" );
    if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
    {
        ALOGF( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroySurface( egl->Display, egl->TinySurface );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

void ovrEgl_DestroyContext( ovrEgl * egl )
{
    if ( egl->Display != 0 )
    {
        // TODO: remove these and fix clean up
        // FAIL( "        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )" );
        if ( eglMakeCurrent( egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
        {
            ALOGV( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        }

        ALOGV( "        eglMakeCurrent() succeeded");
    }
    if ( egl->Context != EGL_NO_CONTEXT )
    {
        // FAIL( "        eglDestroyContext( Display, Context )" );
        if ( eglDestroyContext( egl->Display, egl->Context ) == EGL_FALSE )
        {
            ALOGF( "        eglDestroyContext() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->Context = EGL_NO_CONTEXT;
        ALOGV( "        eglDestroyContext() succeeded");
    }
    if ( egl->TinySurface != EGL_NO_SURFACE )
    {
        // FAIL( "        eglDestroySurface( Display, TinySurface )" );
        if ( eglDestroySurface( egl->Display, egl->TinySurface ) == EGL_FALSE )
        {
            ALOGF( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->TinySurface = EGL_NO_SURFACE;
        ALOGV( "        eglDestroySurface() succeeded");
    }
    if ( egl->Display != 0 )
    {
        // FAIL( "        eglTerminate( Display )" );
        if ( eglTerminate( egl->Display ) == EGL_FALSE )
        {
            ALOGF( "        eglTerminate() failed: %s", EglErrorString( eglGetError() ) );
        }
        ALOGV( "        eglTerminate() succeeded");
        egl->Display = 0;
    }
}


/*
================================================================================

ovrFramebuffer

================================================================================
*/

void ovrFramebuffer_Clear( ovrFramebuffer * frameBuffer )
{
    frameBuffer->Width = 0;
    frameBuffer->Height = 0;
    frameBuffer->Multisamples = 0;
    frameBuffer->TextureSwapChainLength = 0;
    frameBuffer->TextureSwapChainIndex = 0;
    frameBuffer->UseMultiview = false;
    frameBuffer->ColorTextureSwapChain = NULL;
    frameBuffer->DepthBuffers = NULL;
    frameBuffer->FrameBuffers = NULL;
}

bool ovrFramebuffer_Create( ovrFramebuffer * frameBuffer, const bool useMultiview, const ovrTextureFormat colorFormat, const int width, const int height, const int multisamples )
{
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
        (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
        (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );

    PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC) eglGetProcAddress( "glFramebufferTextureMultiviewOVR" );
    PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR =
        (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC) eglGetProcAddress( "glFramebufferTextureMultisampleMultiviewOVR" );

    frameBuffer->Width = width;
    frameBuffer->Height = height;
    frameBuffer->Multisamples = multisamples;
    frameBuffer->UseMultiview = ( useMultiview && ( glFramebufferTextureMultiviewOVR != NULL ) ) ? true : false;

    frameBuffer->ColorTextureSwapChain = vrapi_CreateTextureSwapChain( frameBuffer->UseMultiview ? VRAPI_TEXTURE_TYPE_2D_ARRAY : VRAPI_TEXTURE_TYPE_2D, colorFormat, width, height, 1, true );
    frameBuffer->TextureSwapChainLength = vrapi_GetTextureSwapChainLength( frameBuffer->ColorTextureSwapChain );
    frameBuffer->DepthBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) );
    frameBuffer->FrameBuffers = (GLuint *)malloc( frameBuffer->TextureSwapChainLength * sizeof( GLuint ) );

    ALOGV( "        frameBuffer->UseMultiview = %d", frameBuffer->UseMultiview );

    for ( int i = 0; i < frameBuffer->TextureSwapChainLength; i++ )
    {
        // Create the color buffer texture.
        const GLuint colorTexture = vrapi_GetTextureSwapChainHandle( frameBuffer->ColorTextureSwapChain, i );
        GLenum colorTextureTarget = frameBuffer->UseMultiview ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        GL( glBindTexture( colorTextureTarget, colorTexture ) );
        if ( glExtensions.EXT_texture_border_clamp )
        {
            GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER ) );
            GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER ) );
            GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            GL( glTexParameterfv( colorTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor ) );
        }
        else
        {
            // Just clamp to edge. However, this requires manually clearing the border
            // around the layer to clear the edge texels.
            GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
            GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
        }
        GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
        GL( glTexParameteri( colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
        GL( glBindTexture( colorTextureTarget, 0 ) );

        if ( frameBuffer->UseMultiview )
        {
            // Create the depth buffer texture.
            GL( glGenTextures( 1, &frameBuffer->DepthBuffers[i] ) );
            GL( glBindTexture( GL_TEXTURE_2D_ARRAY, frameBuffer->DepthBuffers[i] ) );
            GL( glTexStorage3D( GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, 2 ) );
            GL( glBindTexture( GL_TEXTURE_2D_ARRAY, 0 ) );

            // Create the frame buffer.
            GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
            GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
            if ( multisamples > 1 && ( glFramebufferTextureMultisampleMultiviewOVR != NULL ) )
            {
                GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frameBuffer->DepthBuffers[i], 0 /* level */, multisamples /* samples */, 0 /* baseViewIndex */, 2 /* numViews */ ) );
                GL( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0 /* level */, multisamples /* samples */, 0 /* baseViewIndex */, 2 /* numViews */ ) );
            }
            else
            {
                GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frameBuffer->DepthBuffers[i], 0 /* level */, 0 /* baseViewIndex */, 2 /* numViews */ ) );
                GL( glFramebufferTextureMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0 /* level */, 0 /* baseViewIndex */, 2 /* numViews */ ) );
            }

            GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
            GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
            if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
            {
                ALOGF( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
                return false;
            }
        }
        else
        {
            if ( multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL && glFramebufferTexture2DMultisampleEXT != NULL )
            {
                // Create multisampled depth buffer.
                GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
                GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
                GL( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height ) );
                GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

                // Create the frame buffer.
                // NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
                GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
                GL( glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
                GL( glFramebufferTexture2DMultisampleEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples ) );
                GL( glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
                GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_FRAMEBUFFER ) );
                GL( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
                if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
                {
                    ALOGF( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
                    return false;
                }
            }
            else
            {
                // Create depth buffer.
                GL( glGenRenderbuffers( 1, &frameBuffer->DepthBuffers[i] ) );
                GL( glBindRenderbuffer( GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
                GL( glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height ) );
                GL( glBindRenderbuffer( GL_RENDERBUFFER, 0 ) );

                // Create the frame buffer.
                GL( glGenFramebuffers( 1, &frameBuffer->FrameBuffers[i] ) );
                GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i] ) );
                GL( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i] ) );
                GL( glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0 ) );

                // Force direct rendering
                //glEnable(GL_BINNING_CONTROL_HINT_QCOM);
                //glHint(GL_BINNING_CONTROL_HINT_QCOM, GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM);

                GL( GLenum renderFramebufferStatus = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
                GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
                if ( renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE )
                {
                    ALOGF( "Incomplete frame buffer object: %s", GlFrameBufferStatusString( renderFramebufferStatus ) );
                    return false;
                }
            }
        }
    }

    return true;
}

void ovrFramebuffer_Destroy( ovrFramebuffer * frameBuffer )
{
    GL( glDeleteFramebuffers( frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers ) );
    if ( frameBuffer->UseMultiview )
    {
        GL( glDeleteTextures( frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers ) );
    }
    else
    {
        GL( glDeleteRenderbuffers( frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers ) );
    }
    vrapi_DestroyTextureSwapChain( frameBuffer->ColorTextureSwapChain );

    free( frameBuffer->DepthBuffers );
    free( frameBuffer->FrameBuffers );

    ovrFramebuffer_Clear( frameBuffer );
}

void ovrFramebuffer_SetCurrent( ovrFramebuffer * frameBuffer )
{
    GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex] ) );
}

void ovrFramebuffer_SetNone()
{
    GL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
}

void ovrFramebuffer_Resolve( ovrFramebuffer * frameBuffer )
{
    // Discard the depth buffer, so the tiler won't need to write it back out to memory.
    const GLenum depthAttachment[1] = { GL_DEPTH_ATTACHMENT };
    glInvalidateFramebuffer( GL_DRAW_FRAMEBUFFER, 1, depthAttachment );

    // Flush this frame worth of commands.
    glFlush();
}

void ovrFramebuffer_Advance( ovrFramebuffer * frameBuffer )
{
    // Advance to the next texture from the set.
    frameBuffer->TextureSwapChainIndex = ( frameBuffer->TextureSwapChainIndex + 1 ) % frameBuffer->TextureSwapChainLength;
}

/*
================================================================================================================================

ovrFence

================================================================================================================================
*/

void ovrFence_Create( ovrFence * fence )
{
#if defined( EGL_SYNC )
    fence->Display = 0;
    fence->Sync = EGL_NO_SYNC_KHR;
#else
    fence->Sync = 0;
#endif
}

void ovrFence_Destroy( ovrFence * fence )
{
#if defined( EGL_SYNC )
    if ( fence->Sync != EGL_NO_SYNC_KHR )
    {
        if ( eglDestroySyncKHR( fence->Display, fence->Sync ) ==  EGL_FALSE )
        {
            ALOGF( "eglDestroySyncKHR() : EGL_FALSE" );
            return;
        }
        fence->Display = 0;
        fence->Sync = EGL_NO_SYNC_KHR;
    }
#else
    if ( fence->Sync != 0 )
    {
        glDeleteSync( fence->Sync );
        fence->Sync = 0;
    }
#endif
}

void ovrFence_Insert( ovrFence * fence )
{
    ovrFence_Destroy( fence );

#if defined( EGL_SYNC )
    fence->Display = eglGetCurrentDisplay();
    fence->Sync = eglCreateSyncKHR( fence->Display, EGL_SYNC_FENCE_KHR, NULL );
    if ( fence->Sync == EGL_NO_SYNC_KHR )
    {
        ALOGF( "eglCreateSyncKHR() : EGL_NO_SYNC_KHR" );
        return;
    }
    // Force flushing the commands.
    // Note that some drivers will already flush when calling eglCreateSyncKHR.
    if ( eglClientWaitSyncKHR( fence->Display, fence->Sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) == EGL_FALSE )
    {
        ALOGF( "eglClientWaitSyncKHR() : EGL_FALSE" );
        return;
    }
#else
    // Create and insert a new sync object.
    fence->Sync = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
    // Force flushing the commands.
    // Note that some drivers will already flush when calling glFenceSync.
    glClientWaitSync( fence->Sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0 );
#endif
}

} // namespace Quill
