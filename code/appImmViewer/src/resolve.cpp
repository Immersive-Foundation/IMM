#include "resolve.h"

static const char* vsShaderAAResolve = ""

"layout(location = 0) in vec2 inVertex;"

"out V2FData"
"{"
	"vec2 uv;"
"}vf;"

"void main()"
"{"
	"vf.uv = inVertex;"
	"gl_Position = vec4(inVertex, 0.0, 1.0);"
"}";

static const char* fsShaderAAResolve = ""

"layout(binding = 0) uniform sampler2DMS unTex0;"

"layout(location = 0) uniform vec4 unFade;"
"layout(location = 1) uniform int  unXOffset;"
"layout(location = 0, index = 0) out vec4 outColor;"


"vec3 linear2srgb(vec3 val)"
"{"
	"if (val.x < 0.0031308) val.x *= 12.92; else val.x = 1.055*pow(val.x, 1.0 / 2.4) - 0.055;"
	"if (val.y < 0.0031308) val.y *= 12.92; else val.y = 1.055*pow(val.y, 1.0 / 2.4) - 0.055;"
	"if (val.z < 0.0031308) val.z *= 12.92; else val.z = 1.055*pow(val.z, 1.0 / 2.4) - 0.055;"
	"return val;"
"}\n"

"void main(void)"
"{"
    "ivec2 p = ivec2(gl_FragCoord.xy);"
    "p.x += unXOffset;"

	"\n#if SS>1\n"
	"vec3 col = vec3(0.0);"
	"for (int k = 0; k<SS; k++)"
	"for (int j = 0; j<SS; j++)"
	"for (int i = 0; i<8; i++)"
	    "col += texelFetch(unTex0, SS*p + ivec2(j, k), i).xyz;"
	"col /= (8.0*float(SS*SS));"
	"\n#else\n"
	"vec3 col = vec3(0.0);"
	"for (int i = 0; i<8; i++)"
	    "col += texelFetch(unTex0, p, i).xyz;"
	"col /= 8.0;"
	"\n#endif\n"

    //"col = pow(col, vec3(0.4545));"
	// this would NOT be necessary if we had glEnable(GL_FRAMEBUFFER_SRGB); , which we don't
	"col = linear2srgb(col);"

	"col *= unFade.x;"

	"outColor = vec4(col, 1.0);"
"}";

#if !defined(ANDROID)
#include "tmp/shader_resolve_vs_hlsl.inc"
#include "tmp/shader_resolve_fs_hlsl.inc"
#endif
using namespace ImmCore;

namespace ExePlayer
{
    bool Resolve::Init(piRenderer* renderer, int superSample)
    {
        mRenderStateResolve = renderer->CreateRasterState(false, true, piRenderer::CullMode::NONE, false, false);
        if (!mRenderStateResolve)
            return false;

        mBlendStateNone = renderer->CreateBlendState(false, false);
        if (!mBlendStateNone)
            return false;

        const piShaderOptions ops = { 1,{ { "SS", superSample } } };
        char error[2048];
        if (renderer->GetAPI() == piRenderer::API::GL || renderer->GetAPI() == piRenderer::API::GLES)
        {
            mAAResolveShader = renderer->CreateShader(&ops, vsShaderAAResolve, nullptr, nullptr, nullptr, fsShaderAAResolve, error);
        }
        else
        {
#if !defined(ANDROID)
            mAAResolveShader = renderer->CreateShaderBinary(nullptr, shader_resolve_vs_code[0], shader_resolve_vs_size[0],
                nullptr, 0, nullptr, 0, nullptr, 0, shader_resolve_fs_code[0], shader_resolve_fs_size[0], error);
#endif
        }
        if (!mAAResolveShader)
            return false;

        return true;
    }

    void Resolve::DeInit(piRenderer* renderer)
    {
        renderer->DestroyRasterState(mRenderStateResolve);
        renderer->DestroyBlendState(mBlendStateNone);
    }

    void Resolve::Do(piRenderer* renderer, piRTarget target, const int *vp, const int unXOffset, const float fade, piTexture colorTextureM)
    {
        renderer->SetRenderTarget(target);
        renderer->SetViewport(0, vp);

        renderer->SetShadingSamples(1);

        if (renderer->GetAPI() == piRenderer::API::DX)
        {
            renderer->SetRasterState(mRenderStateResolve);
            renderer->SetBlendState(mBlendStateNone);
        }
        else
        {
            renderer->SetWriteMask(true, false, false, false, false);
            renderer->SetState(piSTATE_BLEND, false);
            renderer->SetState(piSTATE_DEPTH_TEST, false);
            renderer->SetState(piSTATE_CULL_FACE, false);
            renderer->SetState(piSTATE_FRONT_FACE, false);
        }

        float data[4] = { fade, 0.0f, 0.0f, 0.0f };
        renderer->AttachShader(mAAResolveShader);
        renderer->SetShaderConstant4F(0, (float*)data, 1);
        renderer->SetShaderConstant1I(1, &unXOffset, 1);
        renderer->AttachTextures(1, colorTextureM);
        renderer->DrawUnitQuad_XY(1);
        renderer->DettachTextures();
        renderer->DettachShader();
    }
}
