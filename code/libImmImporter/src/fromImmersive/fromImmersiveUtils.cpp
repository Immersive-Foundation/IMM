#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#include "fromImmersiveUtils.h"

using namespace ImmCore;

namespace ImmImporter
{
        
    bool iReadString(piIStream* fp, piString & wstr)
    {
        const uint32_t num = fp->ReadUInt32();
        char tmp[1024];
        piAssert(num < 1023);
        fp->Read(tmp, num);
        tmp[num] = 0;
        return wstr.InitCopyS(tmp) != 0;;
    }

        
    vec3 iReadVec3f(piIStream* fp)
    {
        vec3 tmp;
        fp->ReadFloatarray((float*)&tmp, 3);
        return tmp;
    }

        
    vec3d iReadVec3d(piIStream *fp)
    {
        vec3d tmp;
        fp->ReadDoublearray((double*)&tmp, 3);
        return tmp;
    }
        
    vec4 iReadVec4f(piIStream *fp)
    {
        vec4 tmp;
        fp->ReadFloatarray((float*)&tmp, 4);
        return tmp;
    }

        

    quatd iReadQuatd(piIStream *fp)
    {
        quatd tmp;
        fp->ReadDoublearray((double*)&tmp, 4);
        return tmp;
    }

      

    bound3 iReadBound3f(piIStream *fp)
    {
        bound3 box;
        box.mMinX = fp->ReadFloat();
        box.mMaxX = fp->ReadFloat();
        box.mMinY = fp->ReadFloat();
        box.mMaxY = fp->ReadFloat();
        box.mMinZ = fp->ReadFloat();
        box.mMaxZ = fp->ReadFloat();
        return box;
    }

      

    trans3d iReadTrans3d(piIStream *fp)
    {
        const quatd rot = iReadQuatd(fp);
        const double sca = fp->ReadDouble();
        const vec3d  tra = iReadVec3d(fp);
        const flip3  fli = flip3(fp->ReadUInt8());
        return trans3d(rot, sca, fli, tra);
    }

    

    bool iReadBlob(piIStream *fp, piTArray<uint8_t> *data)
    {
        const unsigned int len = static_cast<unsigned int>(fp->ReadUInt64());
        if (!data->Init(len, false))
            return false;
        data->SetLength(len);
        fp->Read(data->GetAddress(0), len);
        return true;
    }

        

    static int id = 0;
    bool GenerateNewTempFileName(piString *dst, const wchar_t *tmpFolder)
    {
        wchar_t tmp[1024];
        piwsprintf(tmp, 1023, L"%s/t%d", tmpFolder, id);
        if (!dst->InitCopyW(tmp))
            return false;
        id++;
        return true;
    }

}
