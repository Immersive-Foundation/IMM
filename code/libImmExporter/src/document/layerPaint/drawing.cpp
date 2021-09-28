#include "drawing.h"

using namespace ImmCore;

namespace ImmExporter{
    Drawing::Drawing() {}

    Drawing::~Drawing() {}

    bool Drawing::Init(uint32_t numElements, bool flipped)
    {

        mNumElements = numElements;
        if (numElements >= 1)
            mElements = new Element[numElements];

        mFlipped = flipped;
        return true;
    }

    void Drawing::Destroy(void)
    {
        for (uint32_t i = 0; i < mNumElements; i++)
        {
            mElements[i].Destroy();
        }
        //if(mElements != nullptr)
        //    delete mElements;
    }

    Element* Drawing::GetElement(uint32_t eleIndex) const
    {
        if (eleIndex >= mNumElements)
            return nullptr;
        return mElements + eleIndex;
    }

    void Drawing::ComputeBoundingBox(void)
    {
        if (mNumElements < 1)
            return;
        for (uint32_t i = 0; i < mNumElements; i++)
        {
            if (mElements[i].GetBoundingBox().mMinX == 1e20f) //actual value not set
                continue;
            if (i == 0)
                mBBox = mElements[i].GetBoundingBox();
            else
                mBBox = include(mBBox, mElements[i].GetBoundingBox());
        }
    }

    const float Drawing::FindBiggestStroke(void) const
    {
        // find biggest stroke in this layer
        float biggestStroke = 0.0;
        for (uint32_t i = 0; i < mNumElements; i++)
        {
            const bound3 eleBBox = mElements[i].GetBoundingBox();
            biggestStroke = std::fmaxf(biggestStroke, maxcomp(getsize(eleBBox)));
        }
        return biggestStroke;
    }

    const bool Drawing::GetFlipped(void) const
    {
        return mFlipped;
    }

}
