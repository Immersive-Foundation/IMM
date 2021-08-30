#pragma once

#include "element.h"

namespace ImmImporter
{
	class Drawing
	{
    public:
        enum class ColorSpace : int
        {
            Linear = 0,
            Gamma = 1
        };

        enum PaintRenderingTechnique : int
        {
            Static = 0,
            Pretessellated = 1,
        };

	public:
		Drawing() = default;
		virtual ~Drawing() = default;

		virtual bool Init(uint32_t numElements) = 0;
		virtual void Deinit(void) = 0;

		virtual bool  StartAdding(float biggestStroke) = 0; // for the deserializer
		virtual bool  Add(const Element * ele, ColorSpace colorSpace, bool flipped) = 0;
		virtual void  StopAdding(void) = 0;

		virtual void  SetGpuId(int id) = 0;
		virtual int   GetGpuId(void) const = 0;

        virtual void  SetLoaded(bool loaded) = 0;
        virtual bool  GetLoaded(void) const = 0;

        virtual void     SetFileOffset(uint64_t offset) = 0;
        virtual uint64_t GetFileOffset(void) const = 0;

		virtual const bound3& GetBBox(void) const = 0;
		virtual const uint32_t GetNumStrokes(void) const = 0;
		virtual const uint32_t GetNumGeometryChunks(uint32_t type) const = 0;

	protected:
		virtual void iAppendChunk(const uint32_t vbase, const uint32_t ibase, const uint32_t pbase, const uint32_t numPoints, uint32_t numIndices, uint32_t numPolygons, const bound3 & bbox, const uint32_t type, float biggestStroke) = 0;
	};
}
