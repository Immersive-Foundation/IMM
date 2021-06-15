#include <malloc.h>
#include <string.h>

#include "libCore/src/libBasics/piStr.h"
#include "toImmersiveUtils.h"

using namespace ImmCore;

namespace ImmExporter
{

	void iWriteString(piOStream *fp, const piString * wstr)
	{
		char *cstr = piws2str(wstr->GetS());
		const uint32_t num = static_cast<uint32_t>(strlen(cstr));
		fp->WriteUInt32(num);
		fp->Write(cstr, num);
		free(cstr);
	}
	void iWriteString(piOStream *fp, const char * str)
	{
		const uint32_t num = static_cast<uint32_t>(strlen(str));
		fp->WriteUInt32(num);
		fp->Write(str, num);
	}
	void iWriteVec3f(piOStream *fp, const vec3 & col)
	{
		fp->WriteFloatarray((float*)&col, 3);
	}

	void iWriteTrans3d(piOStream *fp, const trans3d & t)
	{
		fp->WriteDoublearray((const double*)&t.mRotation, 4);
		fp->WriteDoublearray(&t.mScale, 1);
		fp->WriteDoublearray((const double*)&t.mTranslation, 3);
		fp->WriteUInt8((unsigned char)t.mFlip);
	}

	void iWriteBlob(piOStream *fp, void *data, uint64_t len)
	{
		fp->WriteUInt64(len);
		fp->Write(data, len);
	}

	bool iWriteBlob(piOStream *fp, const piString & fileName)
	{
		piFile src;
		if (!src.Open(fileName.GetS(), L"rb"))
			return false;
		const uint64_t srcLen = src.GetLength();
		void *srcData = malloc(srcLen);
		if (!srcData)
			return false;
		src.Read(srcData, srcLen);
		src.Close();

		iWriteBlob(fp, srcData, srcLen);
		free(srcData);
		return true;
	}



	//==============================
	uint64_t iWriteForward16(piOStream *fp)
	{
		const uint64_t offNumBytes = fp->Tell();
		fp->WriteUInt16(0);
		return offNumBytes;
	}
	uint64_t iWriteForward32(piOStream *fp, uint32_t defaultV)
	{
		const uint64_t offNumBytes = fp->Tell(); 
		fp->WriteUInt32(defaultV);
		return offNumBytes;
	}
	uint64_t iWriteForward64(piOStream *fp)
	{
		const uint64_t offNumBytes = fp->Tell();
		fp->WriteUInt64(0);
		return offNumBytes;
	}

	void iWriteBack16(piOStream *fp, uint64_t offset, uint16_t value)
	{
		const uint64_t offCurrent = fp->Tell();
		fp->Seek(offset, piOStream::SeekMode::SET);
		fp->WriteUInt16(value);
		fp->Seek(offCurrent, piOStream::SeekMode::SET);
	}
	void iWriteBack32(piOStream *fp, uint64_t offset, uint32_t value)
	{
		const uint64_t offCurrent = fp->Tell();
		fp->Seek(offset, piOStream::SeekMode::SET);
		fp->WriteUInt32(value);
		fp->Seek(offCurrent, piOStream::SeekMode::SET);
	}
	void iWriteBack64(piOStream *fp, uint64_t offset, uint64_t value)
	{
		const uint64_t offCurrent = fp->Tell();
		fp->Seek(offset, piOStream::SeekMode::SET);
		fp->WriteUInt64(value);
		fp->Seek(offCurrent, piOStream::SeekMode::SET);
	}
	void iWriteBack64(piOStream *fp, uint64_t offset, uint64_t value1, uint64_t value2)
	{
		const uint64_t offCurrent = fp->Tell();
		fp->Seek(offset, piOStream::SeekMode::SET);
		fp->WriteUInt64(value1);
		fp->WriteUInt64(value2);
		fp->Seek(offCurrent, piOStream::SeekMode::SET);
	}


}
