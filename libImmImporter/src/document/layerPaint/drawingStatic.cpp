#include <new>

#include "libCore/src/libBasics/piDebug.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piColor.h"

#include "drawingStatic.h"

using namespace ImmCore;

namespace ImmImporter
{
    static constexpr int kNumBrushTypes = static_cast<int>(Element::BrushSectionType::Count);
    static constexpr int kMaxVertsPerBatch = 65536;
    static const int sectionsLUT[kNumBrushTypes] = { 2, 2, 7, 7, 4 };

    static inline uint32_t buildGeometry_GetIndices(int numPoints, const Element::BrushSectionType brushID)
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
            return sectionsLUT[static_cast<int>(brushID)] * (numPoints * 2 + 2);
        }
    }

    static uint32_t encodeUnitVector(const vec3 & n)
    {
        vec2 xy = n.xy() / (fabsf(n.x) + fabsf(n.y) + fabsf(n.z));
        if( n.z<0.0f ) xy =(1.0f - abs(xy.yx()))*sign(xy.xy());
        xy = round( (1.0f+xy.xy())*32767.5f );
        uint32_t x = clamp(uint32_t(xy.x),0,65535);
        uint32_t y = clamp(uint32_t(xy.y),0,65535);
        return (x) | (y<<16);
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

    static void iBuildGeometry(int vertexBase, DrawingStatic::MyVertexFormat *verts, uint16_t *index, const Element *ele, int numIndices, const int strokeID, Drawing::ColorSpace colorSpace, bool flipped)
    {
        piAssert( verts != nullptr );
        piAssert( index != nullptr );

        const int numPoints = ele->GetNumPoints();
        const Point *points = ((Element*)ele)->GetPoints();
        const Element::BrushSectionType brushID = ele->GetBrush();
        const int brushIDint = static_cast<int>(brushID);
        const Element::VisibilityType visibleMode = ele->GetVisibleMode();

        const int sectionNumPoints = sectionsLUT[brushIDint];

        // Generate vertices
        for (int j = 0; j < numPoints; j++)
        {
            const Point *p = points + j;

            vec3 col = (colorSpace == Drawing::ColorSpace::Gamma) ? iConvert(p->mCol) : sqrt(p->mCol);
            vec3 dir = (visibleMode == Element::VisibilityType::Always) ? vec3(0.0f, 0.0f, 0.0f) : p->mDir;
            vec3 tan, bU, bV; ele->ComputeBasis(j, &tan, &bU, &bV);
            vec3 pos = p->mPos;

            if (j == 0 && equal(points[0].mPos, points[1].mPos))
            {
                pos -= 0.0001f*tan;
            }
            else if (j == numPoints-1 && equal(points[numPoints-1].mPos, points[numPoints-2].mPos))
            {
                pos += 0.0001f*tan;
            }

            // add the vertex data
            verts[j].mPos = pos;
            verts[j].mCol[0] = uint8_t(255.0f * col.x);
            verts[j].mCol[1] = uint8_t(255.0f * col.y);
            verts[j].mCol[2] = uint8_t(255.0f * col.z);
            verts[j].mAxU = encodeUnitVector(bU);
            verts[j].mAxV = encodeUnitVector(bV);
            piAssert(verts[j].mAxU  != verts[j].mAxV);
#if ST_VERTEX_FORMAT == 1
            verts[j].mWidInfo = (p->mWid << 8u) | ((uint8_t) ((static_cast<uint32_t>(visibleMode) << 7u) + (strokeID & 127u)));
            verts[j].mAlp = p->mTra;
#else
            verts[j].mWid = p->mWid;
            verts[j].mAlp = p->mTra;
            verts[j].mDir[0] = int(127.5f + 127.5f*dir.x);
            verts[j].mDir[1] = int(127.5f + 127.5f*dir.y);
            verts[j].mDir[2] = int(127.5f + 127.5f*dir.z);
            verts[j].mInfo = (static_cast<uint32_t>(visibleMode) << 7) + (strokeID & 127);
            verts[j].mTim = p->mTim;
#endif
        }


        // Generate indices   
        if (brushID == Element::BrushSectionType::Segment || brushID == Element::BrushSectionType::Point)
        {
            if(numIndices>0) index[-1] = vertexBase + ((flipped) ? 1 : 0);

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
                
                if (numIndices>0 || k>0 ) index[nf-1] = vertexBase + ((flipped) ? kp1 : k);

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

    bool DrawingStatic::Init(uint32_t numElements)
    {
        mGpuId = -1;
        mGeometry.mNumStrokes = 0;
        mLoaded = false;

        for (int i = 0; i < kNumBrushTypes; i++)
        {
            if (!mGeometry.mBuffers[i].mPoints.Init(32, false))
                return false;

            if (!mGeometry.mBuffers[i].mIndices.Init(32, false))
                return false;

            if (!mGeometry.mBuffers[i].mChunks.Init(4, false))
                return false;
        }

        return true;
    }

    void DrawingStatic::Deinit(void)
    {
        for (int i = 0; i < kNumBrushTypes; i++)
        {
            mGeometry.mBuffers[i].mChunks.End();
            mGeometry.mBuffers[i].mPoints.End();
            mGeometry.mBuffers[i].mIndices.End();
        }
    }

    const bound3& DrawingStatic::GetBBox(void) const
    {
        return mBBox;
    }

    void DrawingStatic::iAppendChunk(const uint32_t vbase, const uint32_t ibase, const uint32_t pbase, const uint32_t numPointsv, uint32_t numIndices, uint32_t numPolygons, const bound3 & bbox, const uint32_t type, const float biggestStroke)
    {
        Geometry::Chunk *chunk = mGeometry.mBuffers[type].mChunks.Alloc(1, true);
        piAssert(chunk != nullptr);
        chunk->mNumVertices = numPointsv;
        chunk->mNumIndices = numIndices;
        chunk->mNumPolygons = numPolygons;
        chunk->mBBox = bbox;
        chunk->mType = type;
        chunk->mVertexOffset = vbase;
        chunk->mIndexOffset = ibase;
        chunk->mPointsOffset = pbase;
        chunk->mChunkIndex = mGeometry.mBuffers[type].mChunks.GetLength() - 1;
        chunk->mBiggestStroke = biggestStroke;
    }

    bool DrawingStatic::StartAdding(float biggestStroke)
    {
        mGeometry.mBiggestStroke = biggestStroke;
        for (int i = 0; i < kNumBrushTypes; i++)
        {
            Builder *builder = mBuilders + i;
            builder->mPointsBase = 0;
            builder->mVerticesBase = 0;
            builder->mIndexBase = 0;
            builder->mNumPoints = 0;
            builder->mNumVertices = 0;
            builder->mNumIndices = 0;
            builder->mNumPolygons = 0;
            builder->lbox = bound3(1e20f);
        }

        mGeometry.mNumStrokes = 0;

        return true;
    }

    
    bool DrawingStatic::Add(const Element * ele, ColorSpace colorSpace, bool flipped)
    {
        const Element::BrushSectionType brushID = ele->GetBrush();
        const int brushIDint = static_cast<int>(brushID);
        const uint32_t numPoints = ele->GetNumPoints();
        const int sectionNumPoints = sectionsLUT[brushIDint];
        const uint32_t numIndices = buildGeometry_GetIndices(numPoints, brushID);
        const uint32_t numVertices = numPoints * sectionNumPoints;

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
            iAppendChunk(builder->mVerticesBase, builder->mIndexBase, builder->mPointsBase, builder->mNumPoints, builder->mNumIndices, builder->mNumPolygons, builder->lbox, brushIDint, mGeometry.mBiggestStroke);

            // reset builder
            builder->mPointsBase   += builder->mNumPoints;
            builder->mVerticesBase += builder->mNumVertices;
            builder->mIndexBase    += builder->mNumIndices;
            builder->mNumPoints   = 0;
            builder->mNumVertices = 0;
            builder->mNumIndices  = 0;
            builder->mNumPolygons = 0;
            builder->lbox = bound3(1e20f);
        }

        iBuildGeometry(builder->mNumVertices, buf->mPoints.Alloc(numPoints, true), buf->mIndices.Alloc(numIndices, true), ele, builder->mNumIndices, mGeometry.mNumStrokes, colorSpace, flipped);
        builder->mNumPoints += numPoints;
        builder->mNumVertices += numVertices;
        builder->mNumIndices += numIndices;
        builder->mNumPolygons += ele->GetNumPolygons();
        builder->lbox = include(builder->lbox, ele->GetBBox());
        mGeometry.mNumStrokes++;

        return true;
    }

    void DrawingStatic::StopAdding(/*piLog *log*/)
    {
        for (int i = 0; i < kNumBrushTypes; i++)
        {
            // place last remaning geometry
            Builder *builder = mBuilders + i;
            if (builder->mNumVertices > 0)
            {
                iAppendChunk(builder->mVerticesBase, builder->mIndexBase, builder->mPointsBase, builder->mNumPoints, builder->mNumIndices, builder->mNumPolygons, builder->lbox, i, mGeometry.mBiggestStroke);
                builder->mPointsBase   += builder->mNumPoints;
                builder->mVerticesBase += builder->mNumVertices;
                builder->mIndexBase    += builder->mNumIndices;
            }

            piAssert(mBuilders[i].mPointsBase == mGeometry.mBuffers[i].mPoints.GetLength());
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

    const uint32_t DrawingStatic::GetNumStrokes(void) const
    {
        return mGeometry.mNumStrokes;
    }

    const uint32_t DrawingStatic::GetNumGeometryChunks(uint32_t type) const
    {
        if( mGeometry.mNumStrokes == 0) return 0;
        return static_cast<uint32_t>(mGeometry.mBuffers[type].mChunks.GetLength());
    }
    const DrawingStatic::Geometry::Chunk * DrawingStatic::GetGeometryChunk(uint32_t id, uint32_t type) const
    {
        return mGeometry.mBuffers[type].mChunks.GetAddress(id);
    }
    const DrawingStatic::Geometry *DrawingStatic::GetGeometry(void) const
    {
        return &mGeometry;
    }
    void DrawingStatic::SetGpuId(int id)
    {
        mGpuId = id;
    }
    int DrawingStatic::GetGpuId(void) const
    {
        return mGpuId;
    }

    void DrawingStatic::SetLoaded(bool loaded)
    {
        mLoaded = loaded;
    }

    bool DrawingStatic::GetLoaded(void) const
    {
        return mLoaded;
    }

    void DrawingStatic::SetFileOffset(uint64_t offset)
    {
        mFileOffset = offset;
    }

    uint64_t DrawingStatic::GetFileOffset(void) const
    {
        return mFileOffset;
    }
}
