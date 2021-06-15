#include "keepalive.h"

using namespace ImmCore;

namespace ImmExporter
{

	const wchar_t* KeepAlive::mKeepAliveTypeNames[3] = { L"None", L"Wiggle", L"Blink" };

	KeepAlive::KeepAlive()//Sequence* sq, Layer* parent)
	{
		mType = IMMKeepAliveType::None;
	}

	KeepAlive::~KeepAlive()
	{
	}

	bool KeepAlive::Init(IMMKeepAliveType type)
	{
		mType = type;

		return true;
	}

	void KeepAlive::Deinit(void)
	{
	}

	KeepAlive::IMMKeepAliveType KeepAlive::GetType(void) const
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
