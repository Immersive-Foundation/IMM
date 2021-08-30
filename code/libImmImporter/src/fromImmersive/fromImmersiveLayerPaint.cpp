#include <string.h>

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libCompression/basic/piBitInterlacer.h"
#include "libImmCore/src/libCompression/basic/piPredictors.h"
#include "libImmCore/src/libCompression/basic/piQuantize.h"
#include "libImmCore/src/libCompression/basic/piTransforms.h"

#include "../document/layerPaint/element.h"
#include "../document/layer.h"
#include "../document/layerPaint.h"
#include "../document/sequence.h"
#include "../document/layerPaint/drawingPretessellated.h"
#include "../document/layerPaint/drawingStatic.h"
#include "../document/layerPaintStatic.h"
#include "../document/layerPaintPretessellated.h"

#include "fromImmersiveLayerPaint.h"

using namespace ImmCore;


// 0=none
// 1=zlib
#define COMP 1

#if COMP==1
#include <zlib.h>
#endif


namespace ImmImporter
{

    namespace fiLayerPaint
    {
            
        static void iReadData(piTArray<uint8_t> *dst, piIStream *fp, int numElements, int numChannel, int numBits, piTArray<uint8_t> *tmp, piTArray<uint8_t> *tmp2)
        {
            const uint32_t numBytes = fp->ReadUInt32();
            if (numBytes == 0)
            {
                dst->SetLength(0);
                memset(dst->GetAddress(0), 0, numElements*numChannel * sizeof(uint8_t));
            }
            else
            {
#if COMP==0
                fp->ReadUInt8array(tmp->GetAddress(0), numBytes);
                piBitInterlacer::deinterlace8(tmp->GetAddress(0), dst->GetAddress(0), numBytes, numElements, numChannel, numBits);
#else
                fp->ReadUInt8array(tmp2->GetAddress(0), numBytes);
                unsigned long lenb = static_cast<unsigned long>(tmp->GetMaxLength());
                int r = uncompress(tmp->GetAddress(0), &lenb, tmp2->GetAddress(0), numBytes);
                piAssert(r == Z_OK);
                piBitInterlacer::deinterlace8(tmp->GetAddress(0), dst->GetAddress(0), lenb, numElements, numChannel, numBits);
#endif
            }
        }
        static void iReadData(piTArray<uint16_t> *dst, piIStream *fp, int numElements, int numChannel, int numBits, piTArray<uint8_t> *tmp, piTArray<uint8_t> *tmp2)
        {
            const uint32_t numBytes = fp->ReadUInt32();
            if (numBytes == 0)
            {
                dst->SetLength(0);
                memset(dst->GetAddress(0), 0, numElements*numChannel * sizeof(uint16_t));
            }
            else
            {
#if COMP==0
                fp->ReadUInt8array(tmp->GetAddress(0), numBytes);
                piBitInterlacer::deinterlace16(tmp->GetAddress(0), dst->GetAddress(0), numBytes, numElements, numChannel, numBits);
#else
                fp->ReadUInt8array(tmp2->GetAddress(0), numBytes);
                unsigned long lenb = static_cast<unsigned long>(tmp->GetMaxLength());
                const int r = uncompress(tmp->GetAddress(0), &lenb, tmp2->GetAddress(0), numBytes);
                piAssert(r == Z_OK);
                piBitInterlacer::deinterlace16(tmp->GetAddress(0), dst->GetAddress(0), lenb, numElements, numChannel, numBits);
#endif
            }
        }
        static void iReadDataRaw(piTArray<vec3> *dst, piIStream *fp, int numElements, piTArray<uint8_t> *tmp)
        {
            const uint32_t numBytes = fp->ReadUInt32();
            if (numBytes == 0)
            {
                dst->SetLength(0);
                memset(dst->GetAddress(0), 0, numElements * sizeof(vec3));
            }
            else
            {
#if COMP==0
                fp->ReadFloatarray2((float*)dst->GetAddress(0), numElements * 3);
#else
                fp->ReadUInt8array(tmp->GetAddress(0), numBytes);
                unsigned long lenb = static_cast<unsigned long>(tmp->GetMaxLength());
                const int r = uncompress((Bytef*)dst->GetAddress(0), &lenb, tmp->GetAddress(0), numBytes);
                piAssert(r == Z_OK);
                piAssert(numElements * sizeof(vec3) == lenb);
#endif
            }
        }


        LayerImplementation ReadData(piIStream *fp, piLog* log, Drawing::PaintRenderingTechnique renderingTechnique)
        {
            LayerPaint *me = nullptr;

            switch (renderingTechnique)
            {
            case Drawing::Static:
                me = new LayerPaintStatic();
                break;
            case Drawing::Pretessellated:
                me = new LayerPaintPretessellated();
                break;
            default:
                me = new LayerPaintStatic();
                break;
            }

            if (!me) return nullptr;

            const uint32_t version = fp->ReadUInt32(); // version
            if (version >1) return nullptr;

            int fpsNum = fp->ReadUInt16();
            const int unused = fp->ReadUInt16();
            const int maxRepeatCount = fp->ReadUInt32();
            const uint32_t numDrawings = fp->ReadUInt32();
            const uint32_t numFrames = fp->ReadUInt32();

            //piAssert(numFrames > 0 && numFrames < 1024);
            //piAssert(numDrawings > 0 && numDrawings < 1024);

            // Only needed for imm files published before next release to Facebook Spaces (Sep 2018).
            while (!piTick::IsFramerateAllowed(fpsNum))
            {
                fpsNum++;
            }

            if (!me->Init(numDrawings, numFrames, maxRepeatCount, fpsNum, version))
                return nullptr;

            return me;
        }


        #ifdef _DEBUG
        static bool isWithinBits(ivec3 v, int bits) { return (v.x >= 0 && v.x < (1 << bits) && v.y >= 0 && v.y < (1 << bits) && v.z >= 0 && v.z < (1 << bits)); }
        #endif

           
        bool ReadDrawing(LayerImplementation vme, uint32_t drawingId, piIStream *fp, piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique, bool flipped)
        {
            LayerPaint* lp = (LayerPaint*)vme;
            Drawing* dr = lp->GetDrawing(drawingId);
            const uint32_t version = lp->GetVersion();
                
            fp->Seek(dr->GetFileOffset(), piIStream::SeekMode::SET);
                
            struct
            {
                // global
                vec2               mMinMaxWid;

                // per element
                piTArray<uint8_t>  mVisible;
                piTArray<uint8_t>  mBrush;
                piTArray<uint16_t> mNumPoints;
                piTArray<vec3>     mBoxCorner;
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

            struct
            {
                piTArray<uint8_t> mBufA;
                piTArray<uint8_t> mBufB;
            }mTmp;

            mData.mMinMaxWid = vec2( 1e20f, -1e20f );
            if (!mData.mVisible.Init(256 * 1024, false)) return false;
            if (!mData.mBrush.Init(256 * 1024, false)) return false;
            if (!mData.mNumPoints.Init(256 * 1024, false)) return false;
            if (!mData.mBoxCorner.Init(256 * 1024, false)) return false;
            if (!mData.mPosDC.Init(256 * 1024, false)) return false;
            if (!mData.mNorDC.Init(256 * 1024, false)) return false;
            if (!mData.mDirDC.Init(256 * 1024, false)) return false;
            if (!mData.mColDC.Init(256 * 1024, false)) return false;
            if (!mData.mAlpDC.Init(256 * 1024, false)) return false;
            if (!mData.mWidDC.Init(256 * 1024, false)) return false;
            if (!mData.mPos.Init(1024 * 1024, false)) return false;
            if (!mData.mNor.Init(1024 * 1024, false)) return false;
            if (!mData.mDir.Init(1024 * 1024, false)) return false;
            if (!mData.mCol.Init(1024 * 1024, false)) return false;
            if (!mData.mAlp.Init(1024 * 1024, false)) return false;
            if (!mData.mWid.Init(1024 * 1024, false)) return false;

            if (!mTmp.mBufA.Init(512 * 1024, false)) return false;
            if (!mTmp.mBufB.Init(512 * 1024, false)) return false;

            const uint32_t numElements = fp->ReadUInt32();

            if (numElements > 0)
            {
                const uint32_t numTotalPoints = fp->ReadUInt32();

                mData.mBrush.SetMaxLengthNoShrink(numElements);
                mData.mVisible.SetMaxLengthNoShrink(numElements);
                mData.mNumPoints.SetMaxLengthNoShrink(numElements);
                mData.mBoxCorner.SetMaxLengthNoShrink(numElements);
                mData.mPosDC.SetMaxLengthNoShrink(numElements * 3);
                mData.mNorDC.SetMaxLengthNoShrink(numElements * 2);
                mData.mDirDC.SetMaxLengthNoShrink(numElements * 2);
                mData.mColDC.SetMaxLengthNoShrink(numElements * 3);
                mData.mAlpDC.SetMaxLengthNoShrink(numElements);
                mData.mWidDC.SetMaxLengthNoShrink(numElements);
                mData.mPos.SetMaxLengthNoShrink(numTotalPoints * 3);
                mData.mNor.SetMaxLengthNoShrink(numTotalPoints * 2);
                mData.mDir.SetMaxLengthNoShrink(numTotalPoints * 2);
                mData.mCol.SetMaxLengthNoShrink(numTotalPoints * 3);
                mData.mAlp.SetMaxLengthNoShrink(numTotalPoints);
                mData.mWid.SetMaxLengthNoShrink(numTotalPoints);

                mTmp.mBufA.SetMaxLengthNoShrink(numTotalPoints * 3 * 2 * 2);
                mTmp.mBufB.SetMaxLengthNoShrink(numTotalPoints * 3 * 2 * 2);

                const float biggestStroke = fp->ReadFloat();
                
                if (version == 1)
                {
                    // compression params                
                    if (fp->ReadUInt32() != 1) return false; // 0=none, 1=zip
                    // bit settings
                    if (fp->ReadUInt8() != 8) return false; // brush
                    if (fp->ReadUInt8() != 4) return false; // visib
                    if (fp->ReadUInt8() != 16) return false; // numpoints
                    if (fp->ReadUInt8() != 8) return false; // corner
                    if (fp->ReadUInt8() != 15) return false; // posDC
                    if (fp->ReadUInt8() != 10) return false; // norDC
                    if (fp->ReadUInt8() != 10) return false; // dirDC
                    if (fp->ReadUInt8() != 10) return false; // colDC
                    if (fp->ReadUInt8() != 8) return false; // alpDC
                    if (fp->ReadUInt8() != 15) return false; // widDC
                    if (fp->ReadUInt8() != 15) return false; // pos
                    if (fp->ReadUInt8() != 10) return false; // nor
                    if (fp->ReadUInt8() != 10) return false; // dir
                    if (fp->ReadUInt8() != 10) return false; // col
                    if (fp->ReadUInt8() != 8) return false; // alp
                    if (fp->ReadUInt8() != 15) return false; // wid

                    // read all channels
                    iReadData(&mData.mBrush, fp, numElements, 1, 8, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mVisible, fp, numElements, 1, 4, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNumPoints, fp, numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadDataRaw(&mData.mBoxCorner, fp, numElements, &mTmp.mBufB);
                    iReadData(&mData.mPosDC, fp, numElements, 3, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNorDC, fp, numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mDirDC, fp, numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mColDC, fp, numElements, 3, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mAlpDC, fp, numElements, 1, 9, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mWidDC, fp, numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mPos, fp, numTotalPoints - numElements, 3, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNor, fp, numTotalPoints - numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mDir, fp, numTotalPoints - numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mCol, fp, numTotalPoints - numElements, 3, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mAlp, fp, numTotalPoints - numElements, 1, 9, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mWid, fp, numTotalPoints - numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                }
                else if (version == 0)
                {
                    // compression params                
                    if (fp->ReadUInt32() != 1) return false; // 0=none, 1=zip
                    // bit settings
                    if (fp->ReadUInt8() != 8) return false; // brush
                    if (fp->ReadUInt8() != 4) return false; // visib
                    if (fp->ReadUInt8() != 16) return false; // numpoints
                    if (fp->ReadUInt8() != 8) return false; // corner
                    if (fp->ReadUInt8() != 12) return false; // posDC
                    if (fp->ReadUInt8() != 10) return false; // norDC
                    if (fp->ReadUInt8() != 10) return false; // dirDC
                    if (fp->ReadUInt8() != 10) return false; // colDC
                    if (fp->ReadUInt8() != 8) return false; // alpDC
                    if (fp->ReadUInt8() != 15) return false; // widDC
                    if (fp->ReadUInt8() != 12) return false; // pos
                    if (fp->ReadUInt8() != 10) return false; // nor
                    if (fp->ReadUInt8() != 10) return false; // dir
                    if (fp->ReadUInt8() != 10) return false; // col
                    if (fp->ReadUInt8() != 8) return false; // alp
                    if (fp->ReadUInt8() != 15) return false; // wid

                    // read all channels
                    iReadData(&mData.mBrush, fp, numElements, 1, 8, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mVisible, fp, numElements, 1, 4, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNumPoints, fp, numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadDataRaw(&mData.mBoxCorner, fp, numElements, &mTmp.mBufB);
                    iReadData(&mData.mPosDC, fp, numElements, 3, 13, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNorDC, fp, numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mDirDC, fp, numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mColDC, fp, numElements, 3, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mAlpDC, fp, numElements, 1, 9, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mWidDC, fp, numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mPos, fp, numTotalPoints - numElements, 3, 14, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mNor, fp, numTotalPoints - numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mDir, fp, numTotalPoints - numElements, 2, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mCol, fp, numTotalPoints - numElements, 3, 11, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mAlp, fp, numTotalPoints - numElements, 1, 9, &mTmp.mBufA, &mTmp.mBufB);
                    iReadData(&mData.mWid, fp, numTotalPoints - numElements, 1, 16, &mTmp.mBufA, &mTmp.mBufB);
                }
                
                // de-quantize
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

                const uint16_t *dataPosDC = mData.mPosDC.GetAddress(0);
                const uint16_t *dataNorDC = mData.mNorDC.GetAddress(0);
                const uint16_t *dataDirDC = mData.mDirDC.GetAddress(0);
                const uint16_t *dataColDC = mData.mColDC.GetAddress(0);
                const uint16_t *dataAlpDC = mData.mAlpDC.GetAddress(0);
                const uint16_t *dataWidDC = mData.mWidDC.GetAddress(0);
                const uint16_t *dataPos = mData.mPos.GetAddress(0);
                const uint16_t *dataNor = mData.mNor.GetAddress(0);
                const uint16_t *dataDir = mData.mDir.GetAddress(0);
                const uint16_t *dataCol = mData.mCol.GetAddress(0);
                const uint16_t *dataAlp = mData.mAlp.GetAddress(0);
                const uint16_t *dataWid = mData.mWid.GetAddress(0);

                if (!dr->StartAdding(biggestStroke))
                    return false;

                Element ele;
                for (uint32_t j = 0; j < numElements; j++)
                {

                    const uint16_t numPoints = mData.mNumPoints.Get(j);
                    const uint8_t  brush = mData.mBrush.Get(j);
                    const uint8_t  visible = mData.mVisible.Get(j);
                    const vec3     bboxCorner = mData.mBoxCorner.Get(j);

                    ele.Make(numPoints, static_cast<Element::BrushSectionType>(brush), static_cast<Element::VisibilityType>(visible));

                    Point *points = ele.GetPoints();
                    for (int k = 0; k < numPoints; k++)
                    {
                        // Position
                        {
                            ivec3 qdat;
                            if (k == 0)
                            {
                                ivec3 sdat = ivec3(dataPosDC); dataPosDC += 3;
                                //piAssert(isWithinBits(sdat, version >= 1 ? 16 : 13));
                                qdat = encoderPosDC.decode(sdat);
                                encoderPos.reset(qdat);
                            }
                            else
                            {
                                ivec3 sdat = ivec3(dataPos); dataPos += 3;
                                //piAssert(isWithinBits(sdat, version >= 1 ? 16 : 14));
                                qdat = encoderPos.decode(sdat);
                            }
                            
                            //piAssert(isWithinBits(qdat, version >= 1 ? 15 : 12));
                            if (version==1)
                                points[k].mPos = bboxCorner + biggestStroke * piQuantize::ibits15(qdat);
                            else
                                points[k].mPos = bboxCorner + biggestStroke * piQuantize::ibits12(qdat);
                            
                        }
                        // Normal
                        {
                            ivec2 qdat;
                            if (k == 0)
                            {
                                ivec2 sdat = ivec2(dataNorDC); dataNorDC += 2;
                                qdat = encoderNorDC.decode(sdat);
                                encoderNor.reset(qdat);
                            }
                            else
                            {
                                qdat = encoderNor.decode(ivec2(dataNor)); dataNor += 2;
                            }
                            points[k].mNor = piTransforms::piNormal::ipolar(piQuantize::ibits10(qdat));
                        }
                        // View direction
                        if (static_cast<Element::VisibilityType>(visible) != Element::VisibilityType::Always)
                        {
                            ivec2 qdat;
                            if (k == 0)
                            {
                                ivec2 sdat = ivec2(dataDirDC); dataDirDC += 2;
                                qdat = encoderDirDC.decode(sdat);
                                encoderDir.reset(qdat);
                            }
                            else
                            {
                                qdat = encoderDir.decode(ivec2(dataDir)); dataDir += 2;
                            }
                            points[k].mDir = piTransforms::piNormal::ipolar(piQuantize::ibits10(qdat));
                        }
                        // Color
                        {
                            ivec3 qdat;
                            if (k == 0)
                            {
                                ivec3 sdat = ivec3(dataColDC); dataColDC += 3;
                                qdat = encoderColDC.decode(sdat);
                                encoderCol.reset(qdat);
                            }
                            else
                            {
                                ivec3 sdat = ivec3(dataCol); dataCol += 3;
                                qdat = encoderCol.decode(sdat);
                            }
                            const vec3 fcol = piQuantize::ibits10(qdat);
                            points[k].mCol = fcol * fcol;
                        }
                        // Opacity
                        {
                            int qdat;
                            if (k == 0)
                            {
                                int sdat = *dataAlpDC; dataAlpDC += 1;
                                qdat = encoderAlpDC.decode(sdat);
                                encoderAlp.reset(qdat);
                            }
                            else
                            {
                                int sdat = *dataAlp; dataAlp += 1;
                                qdat = encoderAlp.decode(sdat);
                            }
                            points[k].mTra = qdat;
                        }
                        // Width
                        {
                            int qdat;
                            if (k == 0)
                            {
                                int sdat = *dataWidDC; dataWidDC += 1;
                                qdat = encoderWidDC.decode(sdat);
                                encoderWid.reset(qdat);
                            }
                            else
                            {
                                int sdat = *dataWid; dataWid += 1;
                                qdat = encoderWid.decode(sdat);
                            }
                            points[k].mWid = qdat; // piQuantize::ibits15(qdat) * (1.7f*biggestStroke);
                        }
                    }

                    // We never export two consequtive points that quantize to the same value. However, we do need
                    // to allow that for the first and last vertex, for they often are part of the caps of the stroke.
                    // Therefore, in those cases, we do reach this point with two consecutive vertices that have exact
                    // same coordinates. That messes up with the tangents and other computations down the line. So, we
                    // introduce a imperceptible bias here to workaround that.
                    //
                    // Better ways to solve this, of course. But for now:
                    //points[          0].mPos = points[          0].mPos - 0.0001f*normalize(points[          2].mPos - points[          1].mPos);
                    //points[numPoints-1].mPos = points[numPoints-1].mPos + 0.0001f*normalize(points[numPoints-2].mPos - points[numPoints-3].mPos);

                    ele.Compute(biggestStroke);

                    if (!dr->Add(&ele, static_cast<Drawing::ColorSpace>(colorSpace), flipped))
                        return false;
                }

                dr->StopAdding();
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

            mTmp.mBufA.End();
            mTmp.mBufB.End();

            dr->SetLoaded(true);
            return true;
        }

        bool ReadAsset(LayerImplementation vme, piIStream *fp, piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique, bool flipped)
        {
            // only loads the main layerpaint asset, drawings are loaded separately
            LayerPaint *me = (LayerPaint *)vme;

            const uint32_t numDrawings = me->GetNumDrawings();
            const uint32_t numFrames = me->GetNumFrames();
            const uint32_t version = me->GetVersion();

            // read the frames
            fp->ReadUInt32array(me->GetFrameBuffer(), numFrames);
                            
            // drawings
            for (uint32_t i = 0; i < numDrawings; i++)
            {
                Drawing * dr = nullptr;
                switch (renderingTechnique)
                {
                case Drawing::Pretessellated:
                {
                    dr = static_cast<DrawingPretessellated *>(me->GetDrawing(i));
                    piAssert(dr != nullptr);
                    new(dr) DrawingPretessellated();
                    break;
                }
                case Drawing::Static:
                {
                    dr = static_cast<DrawingStatic *>(me->GetDrawing(i));
                    piAssert(dr != nullptr);
                    new(dr) DrawingStatic();
                    break;
                }
                default:
                {
                    dr = static_cast<DrawingStatic *>(me->GetDrawing(i));
                    piAssert(dr != nullptr);
                    new(dr) DrawingStatic();
                    break;
                }
                }
                    
                // Only initialize empty drawings, mark unloaded,
                // & store file offsets

                dr->SetLoaded(false);
                dr->SetFileOffset(fp->Tell());

                // read global info
                const uint32_t numElements = fp->ReadUInt32();

                if (!dr->Init(numElements))
                    return false;

                if (numElements > 0)
                {
                    // skip numTotalPoints(4), biggestStroke(4), compressed(4) + 16 bytes bit specs
                    fp->Seek(4 + 4 + 4 + 16, piIStream::SeekMode::CURRENT);
                    // skip each 16 streams
                    for (int j = 0; j < 16; ++j)
                        fp->Seek(fp->ReadUInt32(), piIStream::SeekMode::CURRENT);
                }
            }

            return true;
        }


    }

}
