#include <new>
#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piStr.h"

#include "layerPaintPretessellated.h"

namespace ImmImporter
{

	bool LayerPaintPretessellated::Init(int numDrawings, int numFrames, int maxRepeatCount, uint32_t frameRate, unsigned int version)
	{
		// When adding properties here, make sure to add them to Copy and De/Serialize
		mIsPlaying = false;		
		mTime = 0;
		mFrameRate = frameRate;
		mCurrentFrame = 0;
        mOffset = 0;
		mMaxRepeatCount = maxRepeatCount;
        mVersion = version;

		mDrawings = std::vector<DrawingPretessellated>(static_cast<size_t >(numDrawings));

		if (!mFrames.Init(numFrames, sizeof(uint32_t), true))
			return false;
		mFrames.SetLength(numFrames);
		return true;
	}

	void LayerPaintPretessellated::Deinit(void)
	{
		for (uint64_t i = 0, max = mDrawings.size(); i < max; i++)
		{
			Drawing* drawing = &mDrawings[i];
			drawing->Deinit();
		}
		mDrawings.clear();
		mFrames.End();
	}

	const bound3& LayerPaintPretessellated::GetBBox(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		const Drawing* dr = &mDrawings[drawing];
		return dr->GetBBox();
	}

	const bool LayerPaintPretessellated::HasBBox(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		const Drawing* dr = &mDrawings[drawing];
		return dr->GetNumStrokes() > 0;
	}

	unsigned int LayerPaintPretessellated::GetNumDrawings(void) const
	{
		return static_cast<unsigned int>(mDrawings.size());
	}

	unsigned int LayerPaintPretessellated::GetNumFrames(void) const
	{
		return static_cast<unsigned int>(mFrames.GetLength());
	}

    bool LayerPaintPretessellated::GetPlaying(void) const
    {
        return mIsPlaying;
    }

	void LayerPaintPretessellated::SetTime(piTick time)
	{
		if (!mIsPlaying || mFrames.GetLength() <= 1) return;

		mTime = time;

        const uint64_t numFrames = mFrames.GetLength();
        const uint64_t framesSinceStart = piTick::ToFramesFloor(mTime, mFrameRate) + mOffset;
        const uint64_t framesSinceStartWrapped = framesSinceStart % numFrames;
        const uint64_t numRepeatsSinceStart = framesSinceStart / numFrames;

        if (mMaxRepeatCount != 0 && numRepeatsSinceStart >= mMaxRepeatCount)
        {
            mCurrentFrame = static_cast<unsigned int>(numFrames - 1);
        }
        else
        {
            mCurrentFrame = static_cast<uint32_t>(framesSinceStartWrapped);
        }
	}

    void LayerPaintPretessellated::SetOffset(uint32_t offsetFrames)
    {
        mOffset = offsetFrames;
    }

	void LayerPaintPretessellated::SetPlaying(bool playing)
	{
		if(mFrames.GetLength() <= 1) return;

		mIsPlaying = playing;
	}

    void LayerPaintPretessellated::SetMaxRepeatCount(uint32_t maxCount)
    {
        mMaxRepeatCount = maxCount;
    }


	const Drawing * LayerPaintPretessellated::GetCurrentDrawing(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		return &mDrawings[drawing];
	}

    Drawing * LayerPaintPretessellated::NewDrawing(void)
    {
        return nullptr; // unused
    }

	Drawing * LayerPaintPretessellated::GetDrawing(int drawing) const
	{
		return const_cast<DrawingPretessellated*>(&mDrawings[drawing]);
	}

	uint32_t * LayerPaintPretessellated::GetFrameBuffer(void)
	{
		return (uint32_t*)mFrames.GetAddress(0);
	}

    unsigned int LayerPaintPretessellated::GetVersion() const
    {
        return mVersion;
    }

    unsigned int LayerPaintPretessellated::GetMaxRepeatCount(void) const
    {
        return mMaxRepeatCount;
    }

    unsigned int LayerPaintPretessellated::GetFrameRate(void) const
    {
        return mFrameRate;
    }

}
