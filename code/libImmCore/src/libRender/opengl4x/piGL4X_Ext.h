//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once


#ifdef WINDOWS
#include <windows.h>
#endif
#include <GL/gl.h>
#define GLCOREARB_PROTOTYPES

#include "glcorearb.h"


#include "glext.h"

#include "../piRenderer.h"

namespace ImmCore {

#define oglActiveTexture				((PFNGLACTIVETEXTUREPROC)((NGLEXTINFO*)mExt)->myglfunc[0])
#define oglDeleteBuffers				((PFNGLDELETEBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[1])
#define oglCreateProgram	           ((PFNGLCREATEPROGRAMPROC)((NGLEXTINFO*)mExt)->myglfunc[2])
#define oglCreateShader		           ((PFNGLCREATESHADERPROC)((NGLEXTINFO*)mExt)->myglfunc[3])
#define oglShaderSource                ((PFNGLSHADERSOURCEPROC)((NGLEXTINFO*)mExt)->myglfunc[4])
#define oglCompileShader               ((PFNGLCOMPILESHADERPROC)((NGLEXTINFO*)mExt)->myglfunc[5])
#define oglAttachShader                ((PFNGLATTACHSHADERPROC)((NGLEXTINFO*)mExt)->myglfunc[6])
#define oglLinkProgram                 ((PFNGLLINKPROGRAMPROC)((NGLEXTINFO*)mExt)->myglfunc[7])
#define oglUseProgram                  ((PFNGLUSEPROGRAMPROC)((NGLEXTINFO*)mExt)->myglfunc[8])
#define oglUniform4fv                  ((PFNGLUNIFORM4FVPROC)((NGLEXTINFO*)mExt)->myglfunc[9])
#define oglUniform3fv                  ((PFNGLUNIFORM3FVPROC)((NGLEXTINFO*)mExt)->myglfunc[10])
#define oglUniform2fv                  ((PFNGLUNIFORM2FVPROC)((NGLEXTINFO*)mExt)->myglfunc[11])
#define oglUniform1fv                  ((PFNGLUNIFORM1FVPROC)((NGLEXTINFO*)mExt)->myglfunc[12])
#define oglUniform1i                   ((PFNGLUNIFORM1IPROC)((NGLEXTINFO*)mExt)->myglfunc[13])
#define oglUniform1iv                  ((PFNGLUNIFORM1IVPROC)((NGLEXTINFO*)mExt)->myglfunc[14])
#define oglUniform1ui                  ((PFNGLUNIFORM1UIPROC)((NGLEXTINFO*)mExt)->myglfunc[15])
#define oglUniform1uiv                 ((PFNGLUNIFORM1UIVPROC)((NGLEXTINFO*)mExt)->myglfunc[16])
#define oglUniformMatrix4fv            ((PFNGLUNIFORMMATRIX4FVPROC)((NGLEXTINFO*)mExt)->myglfunc[17])
#define oglGetUniformLocation          ((PFNGLGETUNIFORMLOCATIONPROC)((NGLEXTINFO*)mExt)->myglfunc[18])
#define oglGetShaderiv                 ((PFNGLGETSHADERIVPROC)((NGLEXTINFO*)mExt)->myglfunc[19])
#define oglGetShaderInfoLog            ((PFNGLGETSHADERINFOLOGPROC)((NGLEXTINFO*)mExt)->myglfunc[20])
#define oglGetProgramiv                ((PFNGLGETPROGRAMIVPROC)((NGLEXTINFO*)mExt)->myglfunc[21])
#define oglGetProgramInfoLog           ((PFNGLGETPROGRAMINFOLOGPROC)((NGLEXTINFO*)mExt)->myglfunc[22])
#define oglDeleteShader                ((PFNGLDELETESHADERPROC)((NGLEXTINFO*)mExt)->myglfunc[23])
#define oglDetachShader				   ((PFNGLDETACHSHADERPROC)((NGLEXTINFO*)mExt)->myglfunc[24])
#define oglDeleteProgram			   ((PFNGLDELETEPROGRAMPROC)((NGLEXTINFO*)mExt)->myglfunc[25])
#define oglVertexAttrib1f              ((PFNGLVERTEXATTRIB1FPROC)((NGLEXTINFO*)mExt)->myglfunc[26])
#define oglVertexAttrib2fv             ((PFNGLVERTEXATTRIB2FVPROC)((NGLEXTINFO*)mExt)->myglfunc[27])
#define oglVertexAttrib3fv             ((PFNGLVERTEXATTRIB3FVPROC)((NGLEXTINFO*)mExt)->myglfunc[28])
#define oglVertexAttrib4fv             ((PFNGLVERTEXATTRIB4FVPROC)((NGLEXTINFO*)mExt)->myglfunc[29])
#define oglDeleteRenderbuffers           ((PFNGLDELETERENDERBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[30])
#define oglBindFramebuffer               ((PFNGLBINDFRAMEBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[31])
#define oglDeleteFramebuffers            ((PFNGLDELETEFRAMEBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[32])
#define oglCheckFramebufferStatus        ((PFNGLCHECKFRAMEBUFFERSTATUSPROC)((NGLEXTINFO*)mExt)->myglfunc[33])
#define oglPointParameterfv              ((PFNGLPOINTPARAMETERFVPROC)((NGLEXTINFO*)mExt)->myglfunc[34])
#define oglBlendEquationSeparatei        ((PFNGLBLENDEQUATIONSEPARATEIPROC)((NGLEXTINFO*)mExt)->myglfunc[35])
#define oglDrawBuffers                   ((PFNGLDRAWBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[36])
#define oglGetStringi                    ((PFNGLGETSTRINGIPROC)((NGLEXTINFO*)mExt)->myglfunc[37])
#define oglColorMaski                    ((PFNGLCOLORMASKIPROC)((NGLEXTINFO*)mExt)->myglfunc[38])
#define oglDrawRangeElements             ((PFNGLDRAWRANGEELEMENTSPROC)((NGLEXTINFO*)mExt)->myglfunc[39])
#define oglBindVertexArray              ((PFNGLBINDVERTEXARRAYPROC)((NGLEXTINFO*)mExt)->myglfunc[40])
#define oglPatchParameteri              ((PFNGLPATCHPARAMETERIPROC)((NGLEXTINFO*)mExt)->myglfunc[41])
#define oglDebugMessageCallback         ((PFNGLDEBUGMESSAGECALLBACKPROC)((NGLEXTINFO*)mExt)->myglfunc[42])
#define oglVertexAttribDivisor          ((PFNGLVERTEXATTRIBDIVISORPROC)((NGLEXTINFO*)mExt)->myglfunc[43])
#define oglGetUniformBlockIndex         ((PFNGLGETUNIFORMBLOCKINDEXPROC)((NGLEXTINFO*)mExt)->myglfunc[44])
#define oglBindBufferBase               ((PFNGLBINDBUFFERBASEPROC)((NGLEXTINFO*)mExt)->myglfunc[45])
#define oglClearBufferfv                 ((PFNGLCLEARBUFFERFVPROC)((NGLEXTINFO*)mExt)->myglfunc[46])
#define oglClearTexImage                 ((PFNGLCLEARTEXIMAGEPROC)((NGLEXTINFO*)mExt)->myglfunc[47])
#define oglGenSamplers                   ((PFNGLGENSAMPLERSPROC)((NGLEXTINFO*)mExt)->myglfunc[48])
#define oglSamplerParameteri             ((PFNGLSAMPLERPARAMETERIPROC)((NGLEXTINFO*)mExt)->myglfunc[49])
#define oglDeleteSamplers                ((PFNGLDELETESAMPLERSPROC)((NGLEXTINFO*)mExt)->myglfunc[50])
#define oglBindSamplers                  ((PFNGLBINDSAMPLERSPROC)((NGLEXTINFO*)mExt)->myglfunc[51])
#define oglBindTextures                  ((PFNGLBINDTEXTURESPROC)((NGLEXTINFO*)mExt)->myglfunc[52])
#define oglMinSampleShading              ((PFNGLMINSAMPLESHADINGPROC)((NGLEXTINFO*)mExt)->myglfunc[53])
#define oglCreateFramebuffers            ((PFNGLCREATEFRAMEBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[54])
#define oglNamedFramebufferTexture       ((PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)((NGLEXTINFO*)mExt)->myglfunc[55])
#define oglCheckNamedFramebufferStatus   ((PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)((NGLEXTINFO*)mExt)->myglfunc[56])
#define oglBlitNamedFramebuffer          ((PFNGLBLITNAMEDFRAMEBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[57])
#define oglNamedFramebufferDrawBuffers   ((PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[58])
#define oglCreateBuffers                 ((PFNGLCREATEBUFFERSPROC)((NGLEXTINFO*)mExt)->myglfunc[59])
#define oglNamedBufferData               ((PFNGLNAMEDBUFFERDATAPROC)((NGLEXTINFO*)mExt)->myglfunc[60])
#define oglCreateVertexArrays            ((PFNGLCREATEVERTEXARRAYSPROC)((NGLEXTINFO*)mExt)->myglfunc[61])
#define oglVertexArrayVertexBuffer       ((PFNGLVERTEXARRAYVERTEXBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[62])
#define oglEnableVertexArrayAttrib       ((PFNGLENABLEVERTEXARRAYATTRIBPROC)((NGLEXTINFO*)mExt)->myglfunc[63])
#define oglVertexArrayAttribFormat       ((PFNGLVERTEXARRAYATTRIBFORMATPROC)((NGLEXTINFO*)mExt)->myglfunc[64])
#define oglTextureSubImage2D             ((PFNGLTEXTURESUBIMAGE2DPROC)((NGLEXTINFO*)mExt)->myglfunc[65])
#define oglGenerateTextureMipmap         ((PFNGLGENERATETEXTUREMIPMAPPROC)((NGLEXTINFO*)mExt)->myglfunc[66])
#define oglNamedBufferStorage            ((PFNGLNAMEDBUFFERSTORAGEPROC)((NGLEXTINFO*)mExt)->myglfunc[67])
#define oglVertexArrayAttribBinding      ((PFNGLVERTEXARRAYATTRIBBINDINGPROC)((NGLEXTINFO*)mExt)->myglfunc[68])
#define oglVertexArrayElementBuffer      ((PFNGLVERTEXARRAYELEMENTBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[69])
#define oglDeleteVertexArrays            ((PFNGLDELETEVERTEXARRAYSPROC)((NGLEXTINFO*)mExt)->myglfunc[70])
#define oglDrawElementsInstancedBaseVertexBaseInstance ((PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)((NGLEXTINFO*)mExt)->myglfunc[71])
#define oglNamedBufferSubData            ((PFNGLNAMEDBUFFERSUBDATAPROC)((NGLEXTINFO*)mExt)->myglfunc[72])
#define oglTextureParameteri             ((PFNGLTEXTUREPARAMETERIPROC)((NGLEXTINFO*)mExt)->myglfunc[73])
#define oglCreateTextures                ((PFNGLCREATETEXTURESPROC)((NGLEXTINFO*)mExt)->myglfunc[74])
#define oglTextureStorage2DMultisample   ((PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)((NGLEXTINFO*)mExt)->myglfunc[75])
#define oglTextureStorage2D              ((PFNGLTEXTURESTORAGE2DPROC)((NGLEXTINFO*)mExt)->myglfunc[76])
#define oglTextureStorage3D              ((PFNGLTEXTURESTORAGE3DPROC)((NGLEXTINFO*)mExt)->myglfunc[77])
#define oglTextureSubImage3D             ((PFNGLTEXTURESUBIMAGE3DPROC)((NGLEXTINFO*)mExt)->myglfunc[78])
#define oglTextureParameterf             ((PFNGLTEXTUREPARAMETERFPROC)((NGLEXTINFO*)mExt)->myglfunc[79])
#define oglSamplerParameterf             ((PFNGLSAMPLERPARAMETERFPROC)((NGLEXTINFO*)mExt)->myglfunc[80])
#define oglTextureView                   ((PFNGLTEXTUREVIEWPROC)((NGLEXTINFO*)mExt)->myglfunc[81])
#define oglDrawElementsIndirect          ((PFNGLDRAWELEMENTSINDIRECTPROC)((NGLEXTINFO*)mExt)->myglfunc[82])
#define oglMapNamedBufferRange           ((PFNGLMAPNAMEDBUFFERRANGEPROC)((NGLEXTINFO*)mExt)->myglfunc[83])
#define oglUnmapNamedBuffer              ((PFNGLUNMAPNAMEDBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[84])
#define oglBindBufferRange               ((PFNGLBINDBUFFERRANGEPROC)((NGLEXTINFO*)mExt)->myglfunc[85])
#define oglFenceSync                     ((PFNGLFENCESYNCPROC)((NGLEXTINFO*)mExt)->myglfunc[86])
#define oglClientWaitSync                ((PFNGLCLIENTWAITSYNCPROC)((NGLEXTINFO*)mExt)->myglfunc[87])
#define oglDeleteSync                    ((PFNGLDELETESYNCPROC)((NGLEXTINFO*)mExt)->myglfunc[88])
#define oglNamedFramebufferTextureLayer  ((PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)((NGLEXTINFO*)mExt)->myglfunc[89])
#define oglClearTexSubImage              ((PFNGLCLEARTEXSUBIMAGEPROC)((NGLEXTINFO*)mExt)->myglfunc[90])
#define oglDrawArraysInstanced           ((PFNGLDRAWARRAYSINSTANCEDPROC)((NGLEXTINFO*)mExt)->myglfunc[91])
#define oglGetTextureImage               ((PFNGLGETTEXTUREIMAGEPROC)((NGLEXTINFO*)mExt)->myglfunc[92])
#define oglDispatchCompute               ((PFNGLDISPATCHCOMPUTEPROC)((NGLEXTINFO*)mExt)->myglfunc[93])
#define oglMemoryBarrier                 ((PFNGLMEMORYBARRIERPROC)((NGLEXTINFO*)mExt)->myglfunc[94])
#define oglGetTextureSubImage            ((PFNGLGETTEXTURESUBIMAGEPROC)((NGLEXTINFO*)mExt)->myglfunc[95])
#define oglClipControl                   ((PFNGLCLIPCONTROLPROC)((NGLEXTINFO*)mExt)->myglfunc[96])
#define oglDebugMessageControl           ((PFNGLDEBUGMESSAGECONTROLPROC)((NGLEXTINFO*)mExt)->myglfunc[97])
#define oglMultiDrawArraysIndirect       ((PFNGLMULTIDRAWARRAYSINDIRECTPROC)((NGLEXTINFO*)mExt)->myglfunc[98])
#define oglMultiDrawArrays               ((PFNGLMULTIDRAWARRAYSPROC)((NGLEXTINFO*)mExt)->myglfunc[99])
#define oglGetTextureHandle              ((PFNGLGETTEXTUREHANDLEARBPROC)((NGLEXTINFO*)mExt)->myglfunc[100])
#define oglMakeTextureHandleResident     ((PFNGLMAKETEXTUREHANDLERESIDENTARBPROC)((NGLEXTINFO*)mExt)->myglfunc[101])  
#define oglMakeTextureHandleNonResident  ((PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC)((NGLEXTINFO*)mExt)->myglfunc[102])  
#define oglTextureBarrier                ((PFNGLTEXTUREBARRIERPROC)((NGLEXTINFO*)mExt)->myglfunc[103])  
#define oglBindBuffer                    ((PFNGLBINDBUFFERPROC)((NGLEXTINFO*)mExt)->myglfunc[104])  
#define oglBindImageTexture              ((PFNGLBINDIMAGETEXTUREPROC)((NGLEXTINFO*)mExt)->myglfunc[105])
#define oglBlendFuncSeparatei            ((PFNGLBLENDFUNCSEPARATEIPROC)((NGLEXTINFO*)mExt)->myglfunc[106])
#define oglNamedFramebufferParameteri    ((PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)((NGLEXTINFO*)mExt)->myglfunc[107])
#define oglGetMultisamplefv              ((PFNGLGETMULTISAMPLEFVPROC)((NGLEXTINFO*)mExt)->myglfunc[108])
#define oglViewportIndexedf              ((PFNGLVIEWPORTINDEXEDFPROC)((NGLEXTINFO*)mExt)->myglfunc[109])
#define oglMultiDrawElementsIndirect     ((PFNGLMULTIDRAWELEMENTSINDIRECTPROC)((NGLEXTINFO*)mExt)->myglfunc[110])
#define oglTextureStorage3DMultisample   ((PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)((NGLEXTINFO*)mExt)->myglfunc[111])
#define oglTextureParameteriv            ((PFNGLTEXTUREPARAMETERIVPROC)((NGLEXTINFO*)mExt)->myglfunc[112])
#define oglUniform2uiv                   ((PFNGLUNIFORM1UIVPROC)((NGLEXTINFO*)mExt)->myglfunc[113])
#define oglUniform3uiv                   ((PFNGLUNIFORM1UIVPROC)((NGLEXTINFO*)mExt)->myglfunc[114])
#define oglUniform4uiv                   ((PFNGLUNIFORM1UIVPROC)((NGLEXTINFO*)mExt)->myglfunc[115])
#define oglInvalidateBufferData          ((PFNGLINVALIDATEBUFFERDATAPROC)((NGLEXTINFO*)mExt)->myglfunc[116])
#define oglGenQueries                    ((PFNGLGENQUERIESPROC)((NGLEXTINFO*)mExt)->myglfunc[117])
#define oglDeleteQueries                 ((PFNGLDELETEQUERIESPROC)((NGLEXTINFO*)mExt)->myglfunc[118])
#define oglBeginQuery                    ((PFNGLBEGINQUERYPROC)((NGLEXTINFO*)mExt)->myglfunc[119])
#define oglEndQuery                      ((PFNGLENDQUERYPROC)((NGLEXTINFO*)mExt)->myglfunc[120])
#define oglGetQueryObjectui64v           ((PFNGLGETQUERYOBJECTUI64VPROC)((NGLEXTINFO*)mExt)->myglfunc[121])
#define oglGetTextureParameteriv         ((PFNGLGETTEXTUREPARAMETERIVPROC)((NGLEXTINFO*)mExt)->myglfunc[122])
#define oglGetTextureLevelParameteriv    ((PFNGLGETTEXTURELEVELPARAMETERIVPROC)((NGLEXTINFO*)mExt)->myglfunc[123])
#define oglCompressedTextureImage2D      ((PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC)((NGLEXTINFO*)mExt)->myglfunc[124])
#define oglProgramBinary                 ((PFNGLPROGRAMBINARYPROC)((NGLEXTINFO*)mExt)->myglfunc[125])
#define oglGetProgramBinary              ((PFNGLGETPROGRAMBINARYPROC)((NGLEXTINFO*)mExt)->myglfunc[126])
#define oglGetBufferParameteriv          ((PFNGLGETBUFFERPARAMETERIVPROC)((NGLEXTINFO*)mExt)->myglfunc[127])
#define oglGetVertexAttribiv             ((PFNGLGETVERTEXATTRIBIVPROC)((NGLEXTINFO*)mExt)->myglfunc[128])
#define oglVertexArrayBindingDivisor     ((PFNGLVERTEXARRAYBINDINGDIVISORPROC)((NGLEXTINFO*)mExt)->myglfunc[129])
#define oglViewportArrayv                ((PFNGLVIEWPORTARRAYVPROC)((NGLEXTINFO*)mExt)->myglfunc[130])


//#define oglNamedFramebufferSampleLocationsfv ((PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVARBPROC)((NGLEXTINFO*)mExt)->myglfunc[116])

#define NUMFUNCIONES    131

typedef struct
{
	void *myglfunc[NUMFUNCIONES];
}NGLEXTINFO;

// init
NGLEXTINFO *piGL4X_Ext_Init( piRenderer::piReporter *reporter);
void  piGL4X_Ext_Free( NGLEXTINFO *me );


} // namespace ImmCore

