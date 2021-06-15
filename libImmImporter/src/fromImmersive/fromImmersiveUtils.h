#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piTArray.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piStreamArrayI.h"

namespace ImmImporter
{
    bool iReadString(ImmCore::piIStream* fp, ImmCore::piString & wstr);
    ImmCore::vec3 iReadVec3f(ImmCore::piIStream *fp);
    ImmCore::vec4 iReadVec4f(ImmCore::piIStream *fp);
    ImmCore::bound3 iReadBound3f(ImmCore::piIStream *fp);
    ImmCore::trans3d iReadTrans3d(ImmCore::piIStream *fp);		
    bool iReadBlob(ImmCore::piIStream *fp, ImmCore::piTArray<uint8_t> *data);
        


    bool GenerateNewTempFileName( ImmCore::piString *dst, const wchar_t *tmpFolder);


}
