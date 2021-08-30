//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "piRenderer.h"

#if !defined(ANDROID) // Need this because piGL4X_Ext.h includes <GL/gl.h>
#include "opengl4x/piGL4X_Renderer.h"
#include "directx11/piDX11_Renderer.h"
#else
#include "opengles/piGLES_Renderer.h"
#endif


namespace ImmCore {


piRenderer *piRenderer::Create( const API type )
{
#if !defined(ANDROID)
	if( type==API::GL ) return new piRendererGL4X();
	if( type==API::DX ) return new piRendererDX11();
#else
	if( type==API::GLES ) return new piRendererGLES();
#endif

	return NULL;
}

}
