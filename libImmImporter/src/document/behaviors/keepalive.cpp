#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piDebug.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"
#include "keepalive.h"

using namespace ImmCore;

namespace ImmImporter
{

	const wchar_t* KeepAlive::mKeepAliveTypeNames[3] = { L"None", L"Wiggle", L"Blink" };

	KeepAlive::KeepAlive()//Sequence* sq, Layer* parent)
	{
		mType = KeepAliveType::None;
	}

	KeepAlive::~KeepAlive()
	{
	}

	bool KeepAlive::Init(KeepAliveType type)
	{
		mType = type;

		return true;
	}

	void KeepAlive::Deinit(void)
	{
	}

	KeepAlive::KeepAliveType KeepAlive::GetType(void) const
	{
		return mType;
	}

	const KeepAlive::Wiggle *KeepAlive::GetDataWiggle(void) const
	{
		return &mData.mWiggle;
	}
	const KeepAlive::Blink *KeepAlive::GetDataBlink(void) const
	{
		return &mData.mBlink;
	}

}
