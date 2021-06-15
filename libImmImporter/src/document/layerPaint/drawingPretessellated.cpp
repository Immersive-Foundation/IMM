#include <new>

#include "libCore/src/libBasics/piDebug.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piColor.h"

#include "drawingPretessellated.h"

#if defined(ANDROID)
#define __forceinline inline
#endif
using namespace ImmCore;


namespace ImmImporter
{
	static constexpr int kNumBrushTypes = static_cast<int>(Element::BrushSectionType::Count);
	static constexpr int kMaxVertsPerBatch = 65536;
	
	static __forceinline uint32_t buildGeometry_GetVertices(int numPoints, const Element::BrushSectionType brushID)
	{
		return numPoints * Element::kSectionsLUT[static_cast<int>(brushID)];
	}

	static __forceinline uint32_t buildGeometry_GetIndices(int numPoints, const Element::BrushSectionType brushID)
	{
		if (brushID == Element::BrushSectionType::Point)
		{
			return numPoints * 2 + 2;
		}
		else if (brushID == Element::BrushSectionType::Segment)
		{
			return numPoints * 2 + 2;
		}
		else
		{
			return Element::kSectionsLUT[static_cast<int>(brushID)] * (numPoints * 2 + 2);
		}
	}

    static vec3 iConvert( const  vec3 & col )
    {
       // iq-TODO: create a 3rd color space, ColorSpace::SRGB, and use that in the Android host
       // so we don't have to harcode OS-dependent stuff here
       #if defined(ANDROID)
       return linear2srgb(col);
       #else
       return pow(col, 0.4545f);
       #endif
    }

	static void iBuildGeometry(int vertexBase, DrawingPretessellated::MyVertexFormat *verts, uint16_t *index,  const Element *ele, int numIndices, const int strokeID, Drawing::ColorSpace colorSpace, bool flipped, float biggestStroke)
	{
        piAssert(verts!=nullptr && index!=nullptr);

		const int numPoints = ele->GetNumPoints();
		const Point *points = ((Element*)ele)->GetPoints();
		const Element::BrushSectionType brushID = ele->GetBrush();
		const int brushIDint = static_cast<int>(brushID);
		const Element::VisibilityType visibleMode = ele->GetVisibleMode();

		const int sectionNumPoints = Element::kSectionsLUT[brushIDint];

		const vec2 rect[4] = { vec2(-1.0f,-1.0f), vec2(1.0f,-1.0f), vec2(1.0f,1.0f), vec2(-1.0f,1.0f) };

		// Generate vertices
        int nv = 0;
		for (int j = 0; j < numPoints; j++)
		{
			const Point *p = points + j;
            const float wid = ele->GetWidth(j,biggestStroke);
			const vec3 col = (colorSpace==Drawing::ColorSpace::Gamma) ? iConvert(p->mCol) : sqrt(p->mCol);
			const vec3 dir = (visibleMode == Element::VisibilityType::Always) ? vec3(0.0f, 0.0f, 0.0f) : p->mDir;
            vec3 tan, bU, bV; ele->ComputeBasis(j, &tan, &bU, &bV);
            
			if (brushID == Element::BrushSectionType::Segment || brushID == Element::BrushSectionType::Point)
			{
				for (int i = 0; i <2; i++)
				{
					const float u = (i == 0) ? 1.0f : -1.0f;
					verts[nv].mPos = p->mPos - wid * bU * u;
					verts[nv].mCol[0] = int(255.0f*col.x);
					verts[nv].mCol[1] = int(255.0f*col.y);
					verts[nv].mCol[2] = int(255.0f*col.z);
					verts[nv].mAlp    = p->mTra;
					verts[nv].mDir[0] = int(127.5f + 127.5f*dir.x);
					verts[nv].mDir[1] = int(127.5f + 127.5f*dir.y);
					verts[nv].mDir[2] = int(127.5f + 127.5f*dir.z);
					verts[nv].mInfo = (static_cast<uint32_t>(visibleMode) << 7) + (strokeID & 127);
#if PT_VERTEX_FORMAT==0
					verts[nv].mTim = p->mTim;
#endif
					nv++;
				}
			}
			else
			{
				for (int i = 0; i <sectionNumPoints; i++)
				{
					const float u = float(i) / float(sectionNumPoints);
					const float a = 6.2831f*u;
					vec2 sc;


					if (brushID == Element::BrushSectionType::Circle)
					{
						sc = vec2(cosf(a), sinf(a));
					}
					else if (brushID == Element::BrushSectionType::Ellipse)
					{
						const float ecc = 0.3f;
						sc = vec2(cosf(a), sinf(a) * ecc);
					}
					else if (brushID == Element::BrushSectionType::Square)
					{
						sc = rect[i];
					}

					verts[nv].mPos = p->mPos + wid * (bU * sc.x + bV * sc.y );
					verts[nv].mCol[0] = int(255.0f*col.x);
					verts[nv].mCol[1] = int(255.0f*col.y);
					verts[nv].mCol[2] = int(255.0f*col.z);
					verts[nv].mAlp    = p->mTra;
					verts[nv].mDir[0] = int(127.5f + 127.5f*dir.x);
					verts[nv].mDir[1] = int(127.5f + 127.5f*dir.y);
					verts[nv].mDir[2] = int(127.5f + 127.5f*dir.z);
					verts[nv].mInfo = (static_cast<uint32_t>(visibleMode) << 7) + (strokeID & 127);
#if PT_VERTEX_FORMAT==0
					verts[nv].mTim = p->mTim;
#endif
					nv++;
				}
			}
		}



		// Generate indices
		if (brushID == Element::BrushSectionType::Segment || brushID == Element::BrushSectionType::Point)
		{
			if(numIndices>0) index[-1] = vertexBase + ((flipped)?1:0);

			int off = vertexBase;
            if (flipped)
            {
                for (int j = 0; j < numPoints; j++)
                {
                    index[2 * j + 1] = off + 0;
                    index[2 * j + 0] = off + 1;
                    off += sectionNumPoints;
                }
                index[2 * numPoints + 0] = index[2 * numPoints - 1];
                index[2 * numPoints + 1] = index[2 * numPoints + 0];
            }
            else
            {
                for (int j = 0; j < numPoints; j++)
                {
                    index[2 * j + 0] = off + 0;
                    index[2 * j + 1] = off + 1;
                    off += sectionNumPoints;
                }
                index[2 * numPoints + 0] = index[2 * numPoints - 1];
                index[2 * numPoints + 1] = index[2 * numPoints + 0];
            }
        }
		else
		{
            int nf = 0;
            for (int k = 0; k < sectionNumPoints; k++)
			{
				const int kp1 = (k + 1) % sectionNumPoints;

				if (numIndices>0 || k>0) index[nf-1] = vertexBase + ((flipped) ? kp1 : k);

				int off = vertexBase;
                if (flipped)
                {
                    for (int j = 0; j < numPoints; j++)
                    {
                        index[nf + 1] = off + k;
                        index[nf + 0] = off + kp1;
                        nf += 2;
                        off += sectionNumPoints;
                    }
                    index[nf] = index[nf - 1]; nf++;
                    index[nf] = index[nf - 1]; nf++;
                }
                else
                {
                    for (int j = 0; j < numPoints; j++)
                    {
                        index[nf + 0] = off + k;
                        index[nf + 1] = off + kp1;
                        nf += 2;
                        off += sectionNumPoints;
                    }
                    index[nf] = index[nf - 1]; nf++;
                    index[nf] = index[nf - 1]; nf++;
                }
			}
		}
	}

	//----------------------------------------------------------

	//static uint64_t totMemory = 0;
	//static uint64_t totStrokes = 0;

	bool DrawingPretessellated::Init(uint32_t numElements)
	{
		mGpuId = -1;
		mGeometry.mNumStrokes = 0;
        mLoaded = false;

		for (int i = 0; i < kNumBrushTypes; i++)
		{
            new(&mGeometry.mBuffers[i].mVertices)piTArray<MyVertexFormat>();
            if (!mGeometry.mBuffers[i].mVertices.Init(32, false))
                return false;

            new(&mGeometry.mBuffers[i].mIndices)piTArray<uint16_t>();
            if (!mGeometry.mBuffers[i].mIndices.Init(32, false))
                return false;

            new(&mGeometry.mBuffers[i].mChunks) piTArray<Geometry::Chunk>();
            if (!mGeometry.mBuffers[i].mChunks.Init(4, false))
                return false;
		}

		return true;
	}

	void DrawingPretessellated::Deinit(void)
	{
		for (int i = 0; i < kNumBrushTypes; i++)
		{
			mGeometry.mBuffers[i].mVertices.End();
			mGeometry.mBuffers[i].mIndices.End();
			mGeometry.mBuffers[i].mChunks.End();
		}
	}

	const bound3& DrawingPretessellated::GetBBox(void) const
	{
		return mBBox;
	}


	void DrawingPretessellated::iAppendChunk(const uint32_t vbase, const uint32_t ibase, const uint32_t pbase, const uint32_t numVertices, uint32_t numIndices, uint32_t numPolygons, const bound3 & bbox, const uint32_t type, float biggestStroke)
	{
		Geometry::Chunk *chunk = mGeometry.mBuffers[type].mChunks.Alloc(1, true);
		piAssert(chunk != nullptr);
		chunk->mNumVertices = numVertices;
		chunk->mNumIndices = numIndices;
		chunk->mNumPolygons = numPolygons;
		chunk->mBBox = bbox;
		chunk->mType = type;
		chunk->mVertexOffset = vbase;
		chunk->mIndexOffset = ibase;
	}

	bool DrawingPretessellated::StartAdding(float biggestStroke)
	{
        mGeometry.mBiggesttroke = biggestStroke;
		for (int i = 0; i < kNumBrushTypes; i++)
		{
			Builder *builder = mBuilders + i;
			builder->mVertexBase = 0;
			builder->mIndexBase = 0;
			builder->mNumVertices = 0;
			builder->mNumIndices = 0;
            builder->lbox = bound3(1e20f);
        }

		mGeometry.mNumStrokes = 0;

		return true;
	}


	bool DrawingPretessellated::Add(const Element * ele, ColorSpace colorSpace, bool flipped)
	{
		const Element::BrushSectionType brushID = ele->GetBrush();
		const int brushIDint = static_cast<int>(brushID);
		const uint32_t numPoints = ele->GetNumPoints();
		const uint32_t numVertices = buildGeometry_GetVertices(numPoints, brushID);
		const uint32_t numIndices  = buildGeometry_GetIndices(numPoints, brushID);

		Builder *builder = mBuilders + brushIDint;
		Geometry::Data *buf = mGeometry.mBuffers + brushIDint;

		if (mGeometry.mNumStrokes == 0)
		{
			mBBox = ele->GetBBox();
		}
		else
		{
			mBBox = include(mBBox, ele->GetBBox());
		}

		// check if next stroke still fits in the current chunk. If not, create a new one, copy the data, and flush builder
		if (builder->mNumVertices + numVertices > kMaxVertsPerBatch)
		{
			iAppendChunk(builder->mVertexBase, builder->mIndexBase, 0, builder->mNumVertices, builder->mNumIndices, builder->mNumPolygons, builder->lbox, brushIDint, mGeometry.mBiggesttroke);

			// reset builder
			builder->mVertexBase += builder->mNumVertices;
			builder->mIndexBase  += builder->mNumIndices;
            builder->mNumVertices = 0;
			builder->mNumPolygons = 0;
            builder->mNumIndices  = 0;
			builder->lbox = bound3(1e20f);
		}

		iBuildGeometry(builder->mNumVertices, buf->mVertices.Alloc(numVertices,true), buf->mIndices.Alloc(numIndices,true), ele, builder->mNumIndices, mGeometry.mNumStrokes, colorSpace, flipped, mGeometry.mBiggesttroke);
        builder->mNumVertices += numVertices;
		builder->mNumPolygons += ele->GetNumPolygons();
        builder->mNumIndices  += numIndices;
        builder->lbox = include(builder->lbox, ele->GetBBox());
		mGeometry.mNumStrokes++;


		return true;
	}

	void DrawingPretessellated::StopAdding(/*piLog *log*/)
	{
		for (int i = 0; i < kNumBrushTypes; i++)
		{
			// place last remaning geometry
			Builder *builder = mBuilders + i;
			if (builder->mNumVertices > 0)
			{
				iAppendChunk(builder->mVertexBase, builder->mIndexBase, 0, builder->mNumVertices, builder->mNumIndices, builder->mNumPolygons, builder->lbox, i, mGeometry.mBiggesttroke);
				builder->mVertexBase += builder->mNumVertices;
				builder->mIndexBase += builder->mNumIndices;
			}

            piAssert(mBuilders[i].mVertexBase == mGeometry.mBuffers[i].mVertices.GetLength());
            piAssert(mBuilders[i].mIndexBase == mGeometry.mBuffers[i].mIndices.GetLength());
        }

		/*
		for (int i = 0; i < kNumBrushTypes; i++)
		{
			totMemory += mGeometry.mBuffers[i].mIndices.GetLength() * sizeof(uint16_t);
			totMemory += mGeometry.mBuffers[i].mVertices.GetLength() * sizeof(MyVertexFormat);
			totMemory += mGeometry.mBuffers[i].mChunks.GetLength() * sizeof(Geometry::Chunk);
		}
		totStrokes += mGeometry.mNumStrokes;
		log->Printf(LT_MESSAGE, L"%d MB, %d strokes", totMemory >> 20L, totStrokes);
		*/
	}

	const uint32_t DrawingPretessellated::GetNumStrokes(void) const
	{
		return mGeometry.mNumStrokes;
	}

	const uint32_t DrawingPretessellated::GetNumGeometryChunks(uint32_t type) const
	{
		if( mGeometry.mNumStrokes == 0) return 0;
		return static_cast<uint32_t>(mGeometry.mBuffers[type].mChunks.GetLength());
	}
	const DrawingPretessellated::Geometry::Chunk * DrawingPretessellated::GetGeometryChunk(uint32_t id, uint32_t type) const
	{
		return mGeometry.mBuffers[type].mChunks.GetAddress(id);
	}
	const DrawingPretessellated::Geometry *DrawingPretessellated::GetGeometry(void) const
	{
		return &mGeometry;
	}
	void DrawingPretessellated::SetGpuId(int id)
	{
		mGpuId = id;
	}
	int DrawingPretessellated::GetGpuId(void) const
	{
		return mGpuId;
	}

    void DrawingPretessellated::SetLoaded(bool loaded)
    {
        mLoaded = loaded;
    }

    bool DrawingPretessellated::GetLoaded(void) const
    {
        return mLoaded;
    }

    void DrawingPretessellated::SetFileOffset(uint64_t offset)
    {
        mFileOffset = offset;
    }

    uint64_t DrawingPretessellated::GetFileOffset(void) const
    {
        return mFileOffset;
    }

}
