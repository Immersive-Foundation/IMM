//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif

#ifdef LINUX
#include <GL/glx.h>
#endif

#ifdef IRIX
#include <GL/glx.h>
#endif

#include "piGL4X_Ext.h"

#include <malloc.h>
#include <string.h>

//--- d a t a ---------------------------------------------------------------

namespace ImmCore {


static char *funciones = {
    "glActiveTexture\x0"
    "glDeleteBuffers\x0"
	"glCreateProgram\x0"
	"glCreateShader\x0"
	"glShaderSource\x0"
	"glCompileShader\x0"
	"glAttachShader\x0"
	"glLinkProgram\x0"
	"glUseProgram\x0"
    "glUniform4fv\x0"
    "glUniform3fv\x0"
    "glUniform2fv\x0"
    "glUniform1fv\x0"
    "glUniform1i\x0"
    "glUniform1iv\x0"
    "glUniform1ui\x0"
    "glUniform1uiv\x0"
	"glUniformMatrix4fv\x0"
    "glGetUniformLocation\x0"
	"glGetShaderiv\x0"
	"glGetShaderInfoLog\x0"
    "glGetProgramiv\x0"
	"glGetProgramInfoLog\x0"
    "glDeleteShader\x0"
    "glDetachShader\x0"
    "glDeleteProgram\x0"
    "glVertexAttrib1f\x0"
    "glVertexAttrib2fv\x0"
    "glVertexAttrib3fv\x0"
    "glVertexAttrib4fv\x0"
    "glDeleteRenderbuffers\x0"
    "glBindFramebuffer\x0"
    "glDeleteFramebuffers\x0"
    "glCheckFramebufferStatus\x0"
    "glPointParameterfv\x0"
    "glBlendEquationSeparatei\x0"
	"glDrawBuffers\x0"
    "glGetStringi\x0"
    "glColorMaski\x0"
    "glDrawRangeElements\x0"
    "glBindVertexArray\x0"
    "glPatchParameteri\x0"
	"glDebugMessageCallbackARB\x0"
	"glVertexAttribDivisor\x0"
    "glGetUniformBlockIndex\x0"
    "glBindBufferBase\x0"
    "glClearBufferfv\x0"
    "glClearTexImage\x0"
    "glGenSamplers\x0"
    "glSamplerParameteri\x0"
    "glDeleteSamplers\x0"
    "glBindSamplers\x0"
    "glBindTextures\x0"
    "glMinSampleShading\x0"
	"glCreateFramebuffers\x0"
	"glNamedFramebufferTexture\x0"
	"glCheckNamedFramebufferStatus\x0"
	"glBlitNamedFramebuffer\x0"
	"glNamedFramebufferDrawBuffers\x0"
	"glCreateBuffers\x0"
	"glNamedBufferData\x0"
	"glCreateVertexArrays\x0"
	"glVertexArrayVertexBuffer\x0"
	"glEnableVertexArrayAttrib\x0"
	"glVertexArrayAttribFormat\x0"
	"glTextureSubImage2D\x0"
	"glGenerateTextureMipmap\x0"
    "glNamedBufferStorage\x0"
    "glVertexArrayAttribBinding\x0"
    "glVertexArrayElementBuffer\x0"
    "glDeleteVertexArrays\x0"
    "glDrawElementsInstancedBaseVertexBaseInstance\x0"
    "glNamedBufferSubData\x0"
    "glTextureParameteri\x0"
    "glCreateTextures\x0"
    "glTextureStorage2DMultisample\x0"
    "glTextureStorage2D\x0"
    "glTextureStorage3D\x0"
    "glTextureSubImage3D\x0"
    "glTextureParameterf\x0"
    "glSamplerParameterf\x0"
    "glTextureView\x0"
    "glDrawElementsIndirect\x0"
    "glMapNamedBufferRange\x0"
    "glUnmapNamedBuffer\x0"
    "glBindBufferRange\x0"
    "glFenceSync\x0"
    "glClientWaitSync\x0"
    "glDeleteSync\x0"
    "glNamedFramebufferTextureLayer\x0"
    "glClearTexSubImage\x0"
    "glDrawArraysInstanced\x0"
    "glGetTextureImage\x0"
    "glDispatchCompute\x0"
    "glMemoryBarrier\x0"
    "glGetTextureSubImage\x0"
    "glClipControl\x0"
    "glDebugMessageControl\x0"
    "glMultiDrawArraysIndirect\x0"
    "glMultiDrawArrays\x0"
    "glGetTextureHandleARB\x0"
    "glMakeTextureHandleResidentARB\x0"
    "glMakeTextureHandleNonResidentARB\x0"
    "glTextureBarrier\x0"
    "glBindBuffer\x0"
    "glBindImageTexture\x0"
    "glBlendFuncSeparatei\x0"
    "glNamedFramebufferParameteri\x0"
    "glGetMultisamplefv\x0"
    "glViewportIndexedf\x0"
    "glMultiDrawElementsIndirect\x0"
    "glTextureStorage3DMultisample\x0"
    "glTextureParameteriv\x0"
    "glUniform2uiv\x0"
    "glUniform3uiv\x0"
    "glUniform4uiv\x0"
    "glInvalidateBufferData\x0"
    "glGenQueries\x0"
    "glDeleteQueries\x0"
    "glBeginQuery\x0"
    "glEndQuery\x0"
    "glGetQueryObjectui64v\x0"
    "glGetTextureParameteriv\x0"
    "glGetTextureLevelParameteriv\x0"
    "glCompressedTextureImage2DEXT\x0"
    "glProgramBinary\x0"
    "glGetProgramBinary\x0"
    "glGetBufferParameteriv\x0" 
    "glGetVertexAttribiv\x0"     
    "glVertexArrayBindingDivisor\x0"  
    "glViewportArrayv\x0"
};


//--- c o d e ---------------------------------------------------------------

NGLEXTINFO *piGL4X_Ext_Init(piRenderer::piReporter *reporter)
{
    NGLEXTINFO *info = (NGLEXTINFO *)malloc( sizeof(NGLEXTINFO) );
    if( !info ) 
        return 0;

    char *str = funciones;
    for( int i=0; i<NUMFUNCIONES; i++ )
    {
        #ifdef WINDOWS
        info->myglfunc[i] = (void *)wglGetProcAddress( str );
        #endif
        #ifdef LINUX
        info->myglfunc[i] = (void *)glXGetProcAddress( (const unsigned char *)str );
        #endif
        #ifdef IRIX
        info->myglfunc[i] = (void *)glXGetProcAddress( (const unsigned char *)str );
        #endif

        if( !info->myglfunc[i] )
        {
            reporter->Error( str, 0 );
            return nullptr;
        }

        str += 1 + strlen(str);
    }
    
    return( info );
}

void piGL4X_Ext_Free( NGLEXTINFO *me )
{
    free( me );
}
/*
bool piExt_IsPresent( NGLEXTINFO *me, int ext )
{
    return ( me->ispressent[ext] !=0 );
}
*/

}
