#pragma once

#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piLog.h"
#include "element.h"
#include "drawing.h"


// 0: 24 bytes (PC/Windows)
// 1: 20 bytes (Quest/Android)
#if defined(ANDROID)
#define PT_VERTEX_FORMAT 1
#else
#define PT_VERTEX_FORMAT 0 // set 1 to debug Android issues on Windows
#endif

using namespace ImmCore;

namespace ImmImporter
{

	class DrawingPretessellated : public Drawing
	{
	public:
		DrawingPretessellated() = default;
		~DrawingPretessellated() = default;

		bool Init(uint32_t numElements);
		void Deinit(void);

		bool  StartAdding(float biggestStroke); // for the deserailzies
		bool  Add(const Element * ele, ColorSpace colorSpace, bool flipped);
		void  StopAdding(void);

		void     SetGpuId(int id);
		int      GetGpuId(void) const;

        void  SetLoaded(bool loaded);
        bool  GetLoaded(void) const;

        void  SetFileOffset(uint64_t offset);
        uint64_t GetFileOffset(void) const;

		// GPU
		// 24 / 20
		typedef struct
		{
            vec3    mPos;    // 12  ----> can be 6 bytes is we use "half[3]". chunk data then should have bbox corner
			uint8_t mCol[3]; //  3
			uint8_t mAlp;    //  1
			uint8_t mDir[3]; //  3   ----> can do without in most strokes, split in 2 vertex formats? if not, also, it can be 2 bytes
			uint8_t mInfo;   //  1
#if PT_VERTEX_FORMAT==0
			float   mTim;    //  4   ----> not needed, we can recover time from the chunk data and vertex id!
#endif
		}MyVertexFormat;

		struct Geometry
		{
			struct Chunk
			{
				uint32_t      mNumVertices;
				uint32_t      mNumIndices;
				uint32_t      mNumPolygons;
                ImmCore::bound3        mBBox;
				uint32_t      mType;
				uint32_t      mVertexOffset;
				uint32_t      mIndexOffset;
			};

			struct Data
			{
				ImmCore::piTArray<MyVertexFormat> mVertices;
				ImmCore::piTArray<uint16_t>       mIndices;
				ImmCore::piTArray<Chunk>          mChunks;
			}mBuffers[5];

			uint32_t mNumStrokes;
            float    mBiggesttroke;
		};


		const ImmCore::bound3& GetBBox(void) const;
		const uint32_t GetNumStrokes(void) const;
		const uint32_t GetNumGeometryChunks(uint32_t type) const;
		const Geometry::Chunk * GetGeometryChunk(uint32_t id, uint32_t type) const;
		const Geometry *GetGeometry(void) const;

	protected:
		void iAppendChunk(const uint32_t vbase, const uint32_t ibase, const uint32_t pbase, const uint32_t numVertices, uint32_t numIndices, uint32_t numPolygons, const ImmCore::bound3 & bbox, const uint32_t type, float biggestStroke);

	private:
        ImmCore::bound3   mBBox;
		int      mGpuId;
		Geometry mGeometry;

		struct Builder
		{
            uint32_t mNumVertices;
            uint32_t mNumIndices;
			uint32_t mNumPolygons;
            uint32_t mVertexBase;
            uint32_t mIndexBase;
			ImmCore::bound3 lbox;
		}mBuilders[5];

        uint64_t mFileOffset;
        bool mLoaded;
	};
}
