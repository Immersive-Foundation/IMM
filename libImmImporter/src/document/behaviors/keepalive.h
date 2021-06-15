#pragma once

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piTArray.h"
#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piFile.h"

namespace ImmImporter
{

	typedef void* KeepAliveImplementation;

	//class Layer;

	class KeepAlive
	{
	public:
		KeepAlive();//Sequence* sq, Layer* parent);
		~KeepAlive();

		enum class KeepAliveType       // subtle/minimal procedural effects on top of animated or static content
		{
			None,
			Wiggle,    // uniform undulation (positions, for water, tree canopies, grasses, etc)
			Blink      // uniform undulate (alpha)
		};

		struct None
		{
			int mDummy;
		};
		struct Wiggle
		{
			float mFrequency;
			float mSpeed;
			float mAmplitude;
		};
		struct Blink
		{
			int   mWaveForm; // 0=sin, 1=sqr, 2=saw, 3=tri
			float mSpeed;    // oscilations per second
			float mMinOut;   // min alpha (to be modulated with layer alpha and stroke alpha)
			float mMaxOut;   // max alpha (to be modulated with layer alpha and stroke alpha)
			float mMinIn;    // min signal to map to mMinOut
			float mMaxIn;    // max signal to map to mMaxOut
		};

		static const wchar_t* mKeepAliveTypeNames[3];

		bool Init(KeepAliveType type);
		void Deinit(void);

		KeepAliveType GetType(void) const;
		const Wiggle *GetDataWiggle(void) const;
		const Blink  *GetDataBlink(void) const;

	private:
		KeepAliveType mType;

		union
		{
			None   mNone;
			Wiggle mWiggle;
			Blink  mBlink;
		}mData;

	};

}
