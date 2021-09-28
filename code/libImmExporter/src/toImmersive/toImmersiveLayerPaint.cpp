#include <algorithm>

//#include "libDataUtils/piUOIntMap.h"
//#include "libSystem/piDebug.h"
#include "libImmCore/src/libCompression/basic/piBitInterlacer.h"
#include "libImmCore/src/libCompression/basic/piPredictors.h"
#include "libImmCore/src/libCompression/basic/piQuantize.h"
#include "libImmCore/src/libCompression/basic/piTransforms.h"


#include "../document/layerPaint/element.h"
#include "../document/layer.h"
#include "../document/layerPaint.h"
#include "../document/sequence.h"


#include "toImmersiveUtils.h"

#define EXPORT_V1


// 0=none
// 1=zlib
#define COMP 1

#if COMP==1
#include <zlib.h>
#endif
using namespace ImmCore;
namespace ImmExporter
{

    namespace tiLayerPaint
    {

        static uint64_t iSerializeCompress(piOStream *fp, uint8_t *dst, const uint8_t *src, uint64_t len, uint64_t availOut )
        {
            if (len == 0) return 0;

            #if COMP==0
            fp->WriteUInt8array(src, len);
            return len;
            #elif COMP==1
            unsigned long lenb = static_cast<unsigned long>(availOut);
            int r = compress2(dst, &lenb, src, static_cast<uLong>(len), Z_BEST_COMPRESSION);
            piAssert(r == Z_OK);
//if (lenb>len) fp->WriteUInt8array(src, len); else
            fp->WriteUInt8array(dst, lenb);
            return lenb;
            #endif
        }

        static void iWriteData(piOStream *fp, piTArray<uint8_t> *src, int numChannels, int numBits, uint8_t *dst8, uint8_t *dst8b, uint64_t availOut)
        {
            if (src->GetLength() == 0) { fp->WriteUInt32(0); return; }
            const uint64_t offNumBytes = iWriteForward32(fp,0);
            const uint64_t len = piBitInterlacer::interlace8(dst8, src->GetAddress(0), src->GetLength() / numChannels, numChannels, numBits);
            const uint64_t lenw = iSerializeCompress(fp, dst8b, dst8, len, availOut);
            iWriteBack32(fp, offNumBytes, static_cast<uint32_t>(lenw));
        }
        static void iWriteData(piOStream *fp, piTArray<uint16_t> *src, int numChannels, int numBits, uint8_t *dst8, uint8_t *dst8b, uint64_t availOut)
        {
            if (src->GetLength() == 0) { fp->WriteUInt32(0); return; }
            const uint64_t offNumBytes = iWriteForward32(fp, 0);
            const uint64_t len = piBitInterlacer::interlace16(dst8, src->GetAddress(0), src->GetLength() / numChannels, numChannels, numBits);
            const uint64_t lenw = iSerializeCompress(fp, dst8b, dst8, len, availOut);
            iWriteBack32(fp, offNumBytes, static_cast<uint32_t>(lenw));
        }

        static void iWriteDataRaw(piOStream *fp, piTArray<vec3> *src, uint8_t *dst8, uint8_t *dst8b, uint64_t availOut)
        {
            if (src->GetLength() == 0) { fp->WriteUInt32(0); return; }
            const uint64_t offNumBytes = iWriteForward32(fp, 0);
            const uint64_t len = src->GetLength() * sizeof(vec3);
            const uint64_t lenw = iSerializeCompress(fp, dst8b, (uint8_t*)src->GetAddress(0), len, availOut);
            iWriteBack32(fp, offNumBytes, static_cast<uint32_t>(lenw));
        }

        bool ExportData(uint32_t fps, piOStream *fp, LayerImplementation imp)
        {
            LayerPaint* lp = (LayerPaint*)imp;
            const uint16_t fpsNum = uint16_t(fps);
            const uint16_t fpsDen = 1;
            const uint32_t numDrawings = lp->GetNumDrawings();
            const uint32_t numFrames = lp->GetNumFrames();

            // version 1 vs 0. : pos is 16 bits vs 12
#ifdef EXPORT_V1
            fp->WriteUInt32(1); // version
#else
            fp->WriteUInt32(0); // version
#endif

            fp->WriteUInt16(fpsNum);
            fp->WriteUInt16(fpsDen);
            fp->WriteUInt32(lp->GetMaxRepeatCount());
            fp->WriteUInt32(numDrawings);
            fp->WriteUInt32(numFrames);
            uint64_t DEBUGPOS = fp->Tell();

            return true;
        }

        #ifdef _DEBUG
        static bool isWithinBits(ivec3 v, int bits) { return (v.x >= 0 && v.x < (1 << bits) && v.y >= 0 && v.y < (1 << bits) && v.z >= 0 && v.z < (1 << bits)); }
        #endif

        bool ExportAsset(piOStream *fp, LayerImplementation imp)
        {
            LayerPaint* lp = (LayerPaint*)imp;
            const uint32_t numDrawings = lp->GetNumDrawings();

            // write frame to drawing mapping
            fp->WriteUInt32array(lp->GetFramesData(), lp->GetNumFrames());

            struct
            {
                // per element
                piTArray<uint8_t>  mVisible;
                piTArray<uint8_t>  mBrush;
                piTArray<uint16_t> mNumPoints;
                piTArray<vec3>     mBoxCorner; // quantize the DC too? ^__^
                piTArray<uint16_t> mPosDC;
                piTArray<uint16_t> mNorDC;
                piTArray<uint16_t> mDirDC;
                piTArray<uint16_t> mColDC;
                piTArray<uint16_t> mAlpDC;
                piTArray<uint16_t> mWidDC;

                // per element per vertex
                piTArray<uint16_t> mPos;
                piTArray<uint16_t> mNor;
                piTArray<uint16_t> mDir;
                piTArray<uint16_t> mCol;
                piTArray<uint16_t> mAlp;
                piTArray<uint16_t> mWid;
            }mData;

            if (!mData.mVisible.Init(256*1024, false)) return false;
            if (!mData.mBrush.Init(256 * 1024, false)) return false;
            if (!mData.mNumPoints.Init(256 *1024, false)) return false;
            if (!mData.mBoxCorner.Init(256 * 1024, false)) return false;
            if (!mData.mPosDC.Init(256 * 1024, false)) return false;
            if (!mData.mNorDC.Init(256 * 1024, false)) return false;
            if (!mData.mDirDC.Init(256 * 1024, false)) return false;
            if (!mData.mColDC.Init(256 * 1024, false)) return false;
            if (!mData.mAlpDC.Init(256 * 1024, false)) return false;
            if (!mData.mWidDC.Init(256 * 1024, false)) return false;
            if (!mData.mPos.Init(1024*1024, false)) return false;
            if (!mData.mNor.Init(1024*1024, false)) return false;
            if (!mData.mDir.Init(1024*1024, false)) return false;
            if (!mData.mCol.Init(1024 * 1024, false)) return false;
            if (!mData.mAlp.Init(1024 * 1024, false)) return false;
            if (!mData.mWid.Init(1024 * 1024, false)) return false;

            for (uint32_t idr = 0; idr < numDrawings; idr++)
            {
                const Drawing* drawing = lp->GetDrawing(idr);
                const int numElements = drawing->GetNumElements();
                if (numElements < 1)
                {
                    fp->WriteUInt32(0);
                    continue;
                }
                const bound3 & draBBox = drawing->GetBoundingBox();

                // TODO: this would be a great place to sort the strokes spatially in Morton or
                //       Hilbert order in order to let the entropy coder exploit some of the locality
                //       and coherence in the data

                mData.mVisible.SetLength(0);
                mData.mBrush.SetLength(0);
                mData.mNumPoints.SetLength(0);
                mData.mBoxCorner.SetLength(0);
                mData.mPosDC.SetLength(0);
                mData.mNorDC.SetLength(0);
                mData.mDirDC.SetLength(0);
                mData.mColDC.SetLength(0);
                mData.mAlpDC.SetLength(0);
                mData.mWidDC.SetLength(0);
                mData.mPos.SetLength(0);
                mData.mNor.SetLength(0);
                mData.mDir.SetLength(0);
                mData.mCol.SetLength(0);
                mData.mAlp.SetLength(0);
                mData.mWid.SetLength(0);

                piPredictors::Size_3_Order_1 encoderPosDC;
                piPredictors::Size_2_Order_1 encoderNorDC;
                piPredictors::Size_2_Order_1 encoderDirDC;
                piPredictors::Size_3_Order_1 encoderColDC;
                piPredictors::Size_1_Order_1 encoderAlpDC;
                piPredictors::Size_1_Order_1 encoderWidDC;

                piPredictors::Size_3_Order_2 encoderPos;
                piPredictors::Size_2_Order_1 encoderNor;
                piPredictors::Size_2_Order_1 encoderDir;
                piPredictors::Size_3_Order_1 encoderCol;
                piPredictors::Size_1_Order_1 encoderAlp;
                piPredictors::Size_1_Order_1 encoderWid;

                int totElements = 0;
                int totPoints = 0;
                const float biggestStrokeSize = drawing->FindBiggestStroke();

                for (int iel = 0; iel < numElements; iel++)
                {
                    Element * ele = drawing->GetElement(iel);
                    const Element::VisibilityType visibleMode = ele->GetVisibleMode();
                    const Element::BrushSectionType brushType = ele->GetBrushType();
                    const Point *ptr = ele->GetPoint(0);
                    const bound3 & eleBBox = ele->GetBoundingBox();
                    const vec3 bboxCorner = getcorner(eleBBox, 0);
                    const uint32_t numPoints = ele->GetNumPoints();

                    uint16_t verticesToExport[8192];
                    uint32_t numPointsToExport = 0;
#if 1
                    // prevent consecutive identical vertices
                    // always first and last
                    {
                        ivec3 prev = ivec3(-1, -1, -1);
                        for (uint32_t i = 0; i < numPoints; i++)
                        {
                            const vec3 fpos = (ptr[i].mPos - bboxCorner) / biggestStrokeSize;
                            piAssert(fpos.x >= 0.0 && fpos.x <= 1.0f && fpos.y >= 0.0 && fpos.y <= 1.0f && fpos.z >= 0.0 && fpos.z <= 1.0f);
#ifdef EXPORT_V1
                            const ivec3 qpos = piQuantize::bits15(fpos);
#else
                            const ivec3 qpos = piQuantize::bits12(fpos);
#endif
                            if (i==0 || qpos != prev || i==(numPoints-1) )
                            {
                                verticesToExport[numPointsToExport++] = static_cast<uint16_t>(i);
                            }
                            prev = qpos;
                        }
                    }
                    if (numPointsToExport < 2)
                    {
                        continue;
                    }
#else
                    numPointsToExport = numPoints;
                    for (uint32_t i = 0; i < numPoints; i++)
                    {
                        verticesToExport[i] = static_cast<uint16_t>(i);
                    }
#endif
                    totElements += 1;
                    totPoints += numPointsToExport;

                    mData.mBrush.Append(static_cast<uint8_t>(brushType), true);
                    mData.mVisible.Append(static_cast<uint8_t>(visibleMode), true);
                    mData.mNumPoints.Append(numPointsToExport, true);
                    mData.mBoxCorner.Append(bboxCorner, true);

                    // quantize, predict, and store error
                    for (uint32_t i = 0; i < numPointsToExport; i++)
                    {
                        const int v = verticesToExport[i];
                        {
                            const vec3 fdat = (ptr[v].mPos - bboxCorner) / biggestStrokeSize;
                            piAssert(fdat.x >= 0.0 && fdat.x <= 1.0f && fdat.y >= 0.0 && fdat.y <= 1.0f && fdat.z >= 0.0 && fdat.z <= 1.0f);
#ifdef EXPORT_V1
                            // version 1: 16 bits
                            const ivec3 qdat = piQuantize::bits15(clamp(fdat,0.0,1.0));
                            piAssert(isWithinBits(qdat, 15));
#else
                            // pos, 12 bits  ---  for the longest stroke (2mm precission assuming longest stroke is 5 meters long, sqrt(3)*5m*1000/2^12)
                            const ivec3 qdat = piQuantize::bits12(clamp(fdat, 0.0, 1.0));
                            piAssert(isWithinBits(qdat, 12));
#endif
                            if (i == 0)
                            {
                                encoderPos.reset(qdat);
                                const ivec3 rdat = encoderPosDC.encode(qdat);
#ifdef EXPORT_V1
                                piAssert(isWithinBits(rdat, 16));
#else
                                piAssert(isWithinBits(rdat, 13));
#endif
                                mData.mPosDC.Append(rdat.x, true);
                                mData.mPosDC.Append(rdat.y, true);
                                mData.mPosDC.Append(rdat.z, true);
                            }
                            else
                            {
                                const ivec3 edat = encoderPos.encode(qdat);
#ifdef EXPORT_V1
                                piAssert(isWithinBits(edat, 16));
#else
                                piAssert(isWithinBits(edat, 14));
#endif
                                mData.mPos.Append(edat.x, true);
                                mData.mPos.Append(edat.y, true);
                                mData.mPos.Append(edat.z, true);
                            }
                        }
                        // nor, 10 bits
                        {
                            const vec2 fdat = piTransforms::piNormal::polar(ptr[v].mNor);
                            const ivec2 qdat = piQuantize::bits10(fdat);
                            if (i == 0)
                            {
                                encoderNor.reset(qdat);
                                const ivec2 rdat = encoderNorDC.encode(qdat);
                                mData.mNorDC.Append(rdat.x, true);
                                mData.mNorDC.Append(rdat.y, true);
                            }
                            else
                            {
                                const ivec2 edat = encoderNor.encode(qdat);
                                mData.mNor.Append(edat.x, true);
                                mData.mNor.Append(edat.y, true);
                            }
                        }
                        // dir, 10 bits
                        if( visibleMode != Element::VisibilityType::Always)
                        {
                            const vec2 fdat = piTransforms::piNormal::polar(ptr[v].mDir);
                            const ivec2 qdat = piQuantize::bits10(fdat);
                            if (i == 0)
                            {
                                encoderDir.reset(qdat);
                                const ivec2 rdat = encoderDirDC.encode(qdat);
                                mData.mDirDC.Append(rdat.x, true);
                                mData.mDirDC.Append(rdat.y, true);
                            }
                            else
                            {
                                const ivec2 edat = encoderDir.encode(qdat);
                                mData.mDir.Append(edat.x, true);
                                mData.mDir.Append(edat.y, true);
                            }
                        }
                        // col, 10bit
                        {
                            const vec3 fdat = sqrt(ptr[v].mCol); // do a quare root to bring quantize in perceptual space
                            const ivec3 qdat = piQuantize::bits10(fdat);
                            if (i == 0)
                            {
                                encoderCol.reset(qdat);
                                const ivec3 rdat = encoderColDC.encode(qdat);
                                mData.mColDC.Append(rdat.x, true);
                                mData.mColDC.Append(rdat.y, true);
                                mData.mColDC.Append(rdat.z, true);
                            }
                            else
                            {
                                const ivec3 edat = encoderCol.encode(qdat);
                                mData.mCol.Append(edat.x, true);
                                mData.mCol.Append(edat.y, true);
                                mData.mCol.Append(edat.z, true);
                            }
                        }
                        // alp, 8bits
                        {
                            const float fdat = ptr[v].mTra;
                            const int qdat = piQuantize::bits8(fdat);
                            if (i == 0)
                            {
                                encoderAlp.reset(qdat);
                                const int rdat = encoderAlpDC.encode(qdat);
                                mData.mAlpDC.Append(rdat, true);
                            }
                            else
                            {
                                const int edat = encoderAlp.encode(qdat);
                                mData.mAlp.Append(edat, true);
                            }
                        }
                        // wid, 15 bits
                        {
                            const float fdat = ptr[v].mWid / (1.7f * biggestStrokeSize);
                            const int qdat = piQuantize::bits15(fdat);
                            if (i == 0)
                            {
                                encoderWid.reset(qdat);
                                const int rdat = encoderWidDC.encode(qdat);
                                mData.mWidDC.Append(rdat, true);
                            }
                            else
                            {
                                const int edat = encoderWid.encode(qdat);
                                mData.mWid.Append(edat, true);
                            }
                        }
                    }
                }

                piAssert(mData.mBrush.GetLength() == totElements);
                piAssert(mData.mVisible.GetLength() == totElements);
                piAssert(mData.mNumPoints.GetLength() == totElements);
                piAssert(mData.mBoxCorner.GetLength() == totElements);
                piAssert(mData.mPosDC.GetLength() == totElements * 3);
                piAssert(mData.mNorDC.GetLength() == totElements * 2);
                piAssert(mData.mColDC.GetLength() == totElements * 3);
                piAssert(mData.mAlpDC.GetLength() == totElements * 1);
                piAssert(mData.mWidDC.GetLength() == totElements * 1);
                piAssert(mData.mPos.GetLength() == (totPoints - 1 * totElements) * 3);
                piAssert(mData.mNor.GetLength() == (totPoints - 1 * totElements) * 2);
                piAssert(mData.mCol.GetLength() == (totPoints - 1 * totElements) * 3);
                piAssert(mData.mAlp.GetLength() == (totPoints - 1 * totElements) * 1);
                piAssert(mData.mWid.GetLength() == (totPoints - 1 * totElements) * 1);

                // write to disk
                fp->WriteUInt32(totElements);
                if (totElements == 0)
                    return true;

                fp->WriteUInt32(totPoints);
                fp->WriteFloat(biggestStrokeSize);

                // compression parameters
                {
#ifdef EXPORT_V1
                    // version 1
                    fp->WriteUInt32(1); // 0=none, 1=zip
                    fp->WriteUInt8(8); // brush
                    fp->WriteUInt8(4); // visib
                    fp->WriteUInt8(16); // numpoints
                    fp->WriteUInt8(8); // corner
                    fp->WriteUInt8(15); // posDC
                    fp->WriteUInt8(10); // norDC
                    fp->WriteUInt8(10); // dirDC
                    fp->WriteUInt8(10); // colDC
                    fp->WriteUInt8(8); // alpDC
                    fp->WriteUInt8(15); // widDC
                    fp->WriteUInt8(15); // pos
                    fp->WriteUInt8(10); // nor
                    fp->WriteUInt8(10); // dir
                    fp->WriteUInt8(10); // col
                    fp->WriteUInt8(8); // alp
                    fp->WriteUInt8(15); // wid
#else
                    // version 0
                    fp->WriteUInt32(1); // 0=none, 1=zip
                    fp->WriteUInt8(8); // brush
                    fp->WriteUInt8(4); // visib
                    fp->WriteUInt8(16); // numpoints
                    fp->WriteUInt8(8); // corner
                    fp->WriteUInt8(12); // posDC
                    fp->WriteUInt8(10); // norDC
                    fp->WriteUInt8(10); // dirDC
                    fp->WriteUInt8(10); // colDC
                    fp->WriteUInt8(8); // alpDC
                    fp->WriteUInt8(15); // widDC
                    fp->WriteUInt8(12); // pos
                    fp->WriteUInt8(10); // nor
                    fp->WriteUInt8(10); // dir
                    fp->WriteUInt8(10); // col
                    fp->WriteUInt8(8); // alp
                    fp->WriteUInt8(15); // wid
#endif
                }

                {
                    const uint64_t size = mData.mPos.GetLength() * 3 * sizeof(uint16_t);
                    uint8_t *dst8 = (uint8_t*)malloc(size);
                    if (!dst8) return false;
                    #if COMP==0
                    const uint32_t availOut = 0;
                    uint8_t *dst8b = nullptr;
                    #else
                    const uint64_t availOut = size * 2;
                    uint8_t *dst8b = (uint8_t*)malloc(availOut);
                    if (!dst8b) return false;
                    #endif

#ifdef EXPORT_V1
                    iWriteData(fp, &mData.mBrush, 1, 8, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mVisible, 1, 4, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNumPoints, 1, 16, dst8, dst8b, availOut);
                    iWriteDataRaw(fp, &mData.mBoxCorner, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mPosDC, 3, 16, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNorDC, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mDirDC, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mColDC, 3, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mAlpDC, 1, 9, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mWidDC, 1, 16, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mPos, 3, 16, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNor, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mDir, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mCol, 3, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mAlp, 1, 9, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mWid, 1, 16, dst8, dst8b, availOut);

#else
                    iWriteData(fp, &mData.mBrush, 1, 8, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mVisible, 1, 4, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNumPoints, 1, 16, dst8, dst8b, availOut);
                    iWriteDataRaw(fp, &mData.mBoxCorner, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mPosDC, 3, 13, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNorDC, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mDirDC, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mColDC, 3, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mAlpDC, 1, 9, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mWidDC, 1, 16, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mPos, 3, 14, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mNor, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mDir, 2, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mCol, 3, 11, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mAlp, 1, 9, dst8, dst8b, availOut);
                    iWriteData(fp, &mData.mWid, 1, 16, dst8, dst8b, availOut);
#endif

                    free(dst8);
                    #if COMP>0
                    free(dst8b);
                    #endif
                }
            }

            mData.mBrush.End();
            mData.mNumPoints.End();
            mData.mVisible.End();
            mData.mBoxCorner.End();
            mData.mPosDC.End();
            mData.mNorDC.End();
            mData.mDirDC.End();
            mData.mColDC.End();
            mData.mAlpDC.End();
            mData.mWidDC.End();
            mData.mPos.End();
            mData.mNor.End();
            mData.mDir.End();
            mData.mCol.End();
            mData.mAlp.End();
            mData.mWid.End();

            return true;
        }
    }

}
