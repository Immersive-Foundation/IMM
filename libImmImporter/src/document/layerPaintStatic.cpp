#include <new>
#include "libCore/src/libBasics/piDebug.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"

#include "layerPaintStatic.h"

namespace ImmImporter
{

	bool LayerPaintStatic::Init(int numDrawings, int numFrames, int maxRepeatCount, uint32_t frameRate, unsigned int version)
	{
		// When adding properties here, make sure to add them to Copy and De/Serialize
		mIsPlaying = false;		
		mTime = 0;
		mFrameRate = frameRate;
		mCurrentFrame = 0;
        mOffset = 0;
		mMaxRepeatCount = maxRepeatCount;
        mVersion = version;

		mDrawings = std::vector<DrawingStatic>(static_cast<size_t>(numDrawings));

		if (!mFrames.Init(numFrames, sizeof(uint32_t), true))
			return false;
		mFrames.SetLength(numFrames);
		return true;
	}

	void LayerPaintStatic::Deinit(void)
	{
		for (uint64_t i = 0, max = mDrawings.size(); i < max; i++)
		{
			Drawing* drawing = &mDrawings[i];
			drawing->Deinit();
		}
		mDrawings.clear();
		mFrames.End();
	}

	const bound3& LayerPaintStatic::GetBBox(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		const Drawing* dr = &mDrawings[drawing];
		return dr->GetBBox();
	}

	const bool LayerPaintStatic::HasBBox(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		const Drawing* dr = &mDrawings[drawing];
		return dr->GetNumStrokes() > 0;
	}

	unsigned int LayerPaintStatic::GetNumDrawings(void) const
	{
		return static_cast<unsigned int>(mDrawings.size());
	}

	unsigned int LayerPaintStatic::GetNumFrames(void) const
	{
		return static_cast<unsigned int>(mFrames.GetLength());
	}

    bool LayerPaintStatic::GetPlaying(void) const
    {
        return mIsPlaying;
    }

	void LayerPaintStatic::SetTime(piTick time)
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

    void LayerPaintStatic::SetOffset(uint32_t offsetFrames)
    {
        mOffset = offsetFrames;
    }

	void LayerPaintStatic::SetPlaying(bool playing)
	{
		if(mFrames.GetLength() <= 1) return;

		mIsPlaying = playing;
	}

    void LayerPaintStatic::SetMaxRepeatCount(uint32_t maxCount)
    {
        mMaxRepeatCount = maxCount;
    }


	const Drawing * LayerPaintStatic::GetCurrentDrawing(void) const
	{
		const uint32_t drawing = mFrames.GetSInt32(mCurrentFrame);
		return &mDrawings[drawing];
	}

    Drawing * LayerPaintStatic::NewDrawing(void)
    {
        return nullptr; // unused
    }

	Drawing * LayerPaintStatic::GetDrawing(int drawing) const
	{
		return const_cast<DrawingStatic*>(&mDrawings[drawing]);
	}

	uint32_t * LayerPaintStatic::GetFrameBuffer(void)
	{
		return (uint32_t*)mFrames.GetAddress(0);
	}

    unsigned int LayerPaintStatic::GetVersion(void) const
    {
        return mVersion;
    }

    unsigned int LayerPaintStatic::GetMaxRepeatCount(void) const
    {
        return mMaxRepeatCount;
    }

    unsigned int LayerPaintStatic::GetFrameRate(void) const
    {
        return mFrameRate;
    }

}
