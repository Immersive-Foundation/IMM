#pragma once

#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piStreamO.h"

namespace ImmExporter
{


	void iWriteString(ImmCore::piOStream *fp, const ImmCore::piString * wstr);
	void iWriteString(ImmCore::piOStream *fp, const char * str);
	void iWriteVec3f(ImmCore::piOStream *fp, const ImmCore::vec3 & col);
	void iWriteTrans3d(ImmCore::piOStream *fp, const ImmCore::trans3d & t);

	void iWriteBlob(ImmCore::piOStream *fp, void *data, uint64_t len);
	bool iWriteBlob(ImmCore::piOStream *fp, const ImmCore::piString & fileName);

	uint64_t iWriteForward16(ImmCore::piOStream *fp);
	uint64_t iWriteForward32(ImmCore::piOStream *fp, uint32_t defaultV);
	uint64_t iWriteForward64(ImmCore::piOStream *fp);
	void iWriteBack16(ImmCore::piOStream *fp, uint64_t offset, uint16_t value);
	void iWriteBack32(ImmCore::piOStream *fp, uint64_t offset, uint32_t value);
	void iWriteBack64(ImmCore::piOStream *fp, uint64_t offset, uint64_t value);
	void iWriteBack64(ImmCore::piOStream *fp, uint64_t offset, uint64_t value1, uint64_t value2);


}
