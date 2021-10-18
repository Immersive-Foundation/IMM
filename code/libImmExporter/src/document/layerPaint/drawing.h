#pragma once


#include "libImmCore/src/libBasics/piVecTypes.h"

#include "element.h"
#include <vector>

namespace ImmExporter
{
	class Drawing
	{
	public:
		Drawing();
		~Drawing();

        /** Initialize drawing.
        * Only Allocate memory for elements of the drawings
        * Need to call Element::Init() to initialize individual elements.
        * \param numElements Number of elements. Can not be zero. */
        bool Init(uint32_t numElements, bool flipped);

        /** Get an element of the drawing.
        * \param eleIndex Index of the element.
        * \return Pointer to the element object. If the element index is out of bounds, return nullptr. */
        Element* GetElement(uint32_t eleIndex) const;

        /** Compute bounding box of the drawing. */
        void ComputeBoundingBox(void);

        /** Get bounding box of the drawing.
        * Need to call ComputeBoundingBox() at least once before using this function.
        * \return Bounding box.  */
        inline const ImmCore::bound3 GetBoundingBox(void) const { return mBBox; }
        inline uint32_t GetNumElements(void) const { return mNumElements; }
        void Destroy(void);
        const float FindBiggestStroke(void) const;
        const bool GetFlipped(void) const;

    private:
        Element* mElements = nullptr;
        uint32_t mNumElements = 0;
        ImmCore::bound3 mBBox;
        bool mFlipped;
	};
}
