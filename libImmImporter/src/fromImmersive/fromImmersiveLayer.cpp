#include <string.h>

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libCompression/basic/piBitInterlacer.h"
#include "libCore/src/libCompression/basic/piPredictors.h"
#include "libCore/src/libCompression/basic/piQuantize.h"
#include "libCore/src/libCompression/basic/piTransforms.h"

#include "../document/layer.h"
#include "../document/layerPaint.h"
#include "../document/layerSpawnArea.h"
#include "../document/layerSound.h"
#include "../document/layerModel3d.h"
#include "../document/layerPicture.h"
#include "../document/sequence.h"

#include "fromImmersiveLayerPaint.h"
#include "fromImmersiveLayerSound.h"
#include "fromImmersiveLayerPicture.h"
#include "fromImmersiveLayerModel.h"
#include "fromImmersiveLayerSpawnArea.h"
#include "fromImmersiveUtils.h"

using namespace ImmCore;

namespace ImmImporter
{
    namespace fiLayer
    {
        static bool iReadTimeline(piIStream *fp, Layer *la)
        {
            const uint64_t signature = fp->ReadUInt64();
            if (signature != 0x656e696c656d6954) return false; // "Timeline"
            const uint64_t size = fp->ReadUInt64();
            const uint32_t version = fp->ReadUInt32();

            if (version > 2) return false;

            // version 0 has two less properties than version 1, that has one less than version 2
            const uint32_t maxProps = static_cast<int>(Layer::AnimProperty::MAX) + (version == 0 ? -3 : (version == 1 ? -1 : 0));

            for (uint32_t i = 0; i < maxProps; i++)
            {
                Layer::AnimProperty id = static_cast<Layer::AnimProperty>(i);

                const uint32_t aid = fp->ReadUInt16();
                const uint64_t numKeys = fp->ReadUInt64();
                if (aid != i) return false;
                piAssert(numKeys < 1000000);

                if (numKeys > 0)
                {
                    for (uint64_t j = 0; j < numKeys; j++)
                    {
                        const uint64_t time = fp->ReadUInt64();
                        const Layer::InterpolationType interpolation = static_cast<Layer::InterpolationType>(fp->ReadUInt8());

                        Layer::AnimValue value;

                        switch (id)
                        {
                        case Layer::AnimProperty::Action:
                            value.mInt = fp->ReadUInt32();
                            break;
                        case Layer::AnimProperty::DrawInTime:
                            fp->ReadDoublearray(&value.mDouble, 1);
                            break;
                        case Layer::AnimProperty::Opacity:
                            value.mFloat = fp->ReadFloat();
                            break;
                        case Layer::AnimProperty::Transform: // new in version 2
                            value.mTransform = iReadTrans3d(fp);
                            break;
                        case Layer::AnimProperty::Position: // obsolete in version 2
                            if (version<2) fp->ReadDoublearray(&value.mTransform.mTranslation.x, 3);
                            break;
                        case Layer::AnimProperty::Rotation: // obsolete in version 2
                            if (version < 2) fp->ReadDoublearray(&value.mTransform.mRotation.x, 4);
                            break;
                        case Layer::AnimProperty::Scale: // obsolete in version 2
                            if (version < 2) fp->ReadDoublearray(&value.mTransform.mScale, 1);
                            break;
                        case Layer::AnimProperty::Visibility:
                            value.mBool = (fp->ReadUInt8() == 1);
                            break;
                        case Layer::AnimProperty::Loop:
                            value.mBool = (fp->ReadUInt8() == 1);
                            break;
                        case Layer::AnimProperty::Offset:
                            value.mInt = fp->ReadUInt32();
                            break;
                        default:
                            break;
                        }

                        if (!la->AddKey(piTick(int64_t(time)), id, value, interpolation))
                            return false;

                    }
                }
                else if (id == Layer::AnimProperty::Visibility)
                {
                    // make a defaut visibility key at time 0
                    Layer::AnimValue value; value.mBool = true;
                    if (!la->AddKey(piTick(0), id, value, Layer::InterpolationType::None))
                        return false;
                }
            }

            // Convert individual components to transform components
            for (unsigned int i = 0; i < la->GetNumAnimKeys(Layer::AnimProperty::Position); ++i)
            {
                Layer::AnimKey *posKey = (Layer::AnimKey*) la->GetAnimKey(Layer::AnimProperty::Position, i);
                Layer::AnimValue val;
                val.mTransform = la->GetTransform();
                val.mTransform.mTranslation = posKey->mValue.mTransform.mTranslation;
                la->AddKey(posKey->mTime, Layer::AnimProperty::Transform, val, posKey->mInterpolation);
            }
            for (unsigned int i = 0; i < la->GetNumAnimKeys(Layer::AnimProperty::Rotation); ++i)
            {
                Layer::AnimKey *rotKey = (Layer::AnimKey*) la->GetAnimKey(Layer::AnimProperty::Rotation, i);
                Layer::AnimKey *existing = (Layer::AnimKey *) la->GetAnimKeyAt(Layer::AnimProperty::Transform, rotKey->mTime);
                if (existing == nullptr)
                {
                    Layer::AnimValue val;
                    val.mTransform = la->GetTransform();
                    val.mTransform.mRotation = rotKey->mValue.mTransform.mRotation;
                    la->AddKey(rotKey->mTime, Layer::AnimProperty::Transform, val, rotKey->mInterpolation);
                }
                else
                {
                    existing->mValue.mTransform.mRotation = rotKey->mValue.mTransform.mRotation;
                    existing->mInterpolation = rotKey->mInterpolation;
                }
            }
            for (unsigned int i = 0; i < la->GetNumAnimKeys(Layer::AnimProperty::Scale); ++i)
            {
                Layer::AnimKey *scaKey = (Layer::AnimKey*) la->GetAnimKey(Layer::AnimProperty::Scale, i);
                Layer::AnimKey *existing = (Layer::AnimKey *) la->GetAnimKeyAt(Layer::AnimProperty::Transform, scaKey->mTime);
                if (existing == nullptr)
                {
                    Layer::AnimValue val;
                    val.mTransform = la->GetTransform();
                    val.mTransform.mScale = scaKey->mValue.mTransform.mScale;
                    la->AddKey(scaKey->mTime, Layer::AnimProperty::Transform, val, scaKey->mInterpolation);
                }
                else
                {
                    existing->mValue.mTransform.mScale = scaKey->mValue.mTransform.mScale;
                    existing->mInterpolation = scaKey->mInterpolation;
                }
            }

            return true;
        }

            

        bool Read(Layer *me, piIStream *fp, Sequence *sq, piLog* log, Drawing::PaintRenderingTechnique renderingTechnique, volatile bool *doLoad)
        {
            if (!*doLoad) return false;

            piString laName;
            piString laType;

            bool visible = true;

            const uint64_t signature = fp->ReadUInt64();
            if (signature != 0x736564726579614c)
            {
                log->Printf(LT_ERROR, L"Could not init layer, layerdesc missing");
                return false; // "Layerdes"
            }
            const uint64_t size = fp->ReadUInt64();
            const uint32_t version = fp->ReadUInt32();

            if (version > 1)
            {
                log->Printf(LT_ERROR, L"Could not init layer version is %d", version);
                return false;
            }

            const trans3d laTran = iReadTrans3d(fp);
            trans3d laPivot = trans3d::identity();

            if (version >= 1) // added pivot in version 1
            {
                laPivot = iReadTrans3d(fp);
            }


            const float laOpacity = fp->ReadFloat();
            const bool isTimeLine = (fp->ReadUInt8() == 1);
            const uint64_t duration = fp->ReadUInt64();
            const uint32_t maxRepeatCount = fp->ReadUInt32();

            if (!iReadString(fp, laName))
            {
                laName.End();
                laType.End();
                log->Printf(LT_ERROR, L"Could not read layer name");
                return false;
            }
            if (!iReadString(fp, laType))
            {
                laName.End();
                laType.End();
                log->Printf(LT_ERROR, L"Could not read layer \"%s\" type", laName.GetS());
                return false;
            }

            const uint32_t assetID = fp->ReadUInt32();

            Layer::Type lt = Layer::Type::Group;
            if (laType.EqualW(L"Immersiv.Group"))  lt = Layer::Type::Group;
            else if (laType.EqualW(L"Quill.Paint"))     lt = Layer::Type::Paint;
            else if (laType.EqualW(L"Quill.Model"))     lt = Layer::Type::Model;
            else if (laType.EqualW(L"Quill.Picture"))   lt = Layer::Type::Picture;
            else if (laType.EqualW(L"Quill.Sound"))     lt = Layer::Type::Sound;
            else if (laType.EqualW(L"Quill.Viewpoint") || laType.EqualW(L"Quill.SpawnArea")) lt = Layer::Type::SpawnArea;
            else
            {
                laName.End();
                laType.End();
                log->Printf(LT_ERROR, L"Could not read layer \"%s\" unknown type", laName.GetS());
                return false;
            }
            laType.End();

            if (!me->Init(lt, laName.GetS(), visible, laTran, laPivot, laOpacity, isTimeLine, piTick(int64_t(duration)), maxRepeatCount, assetID, log))
            {
                log->Printf(LT_ERROR, L"Could not init layer \"%s\"", laName.GetS());
                laName.End();
                return false;
            }
            laName.End();

            if (!iReadTimeline(fp, me))
            {
                log->Printf(LT_ERROR, L"Could not read timeline \"%s\"", laName.GetS());
                return false;
            }

            if (!*doLoad) return false;

            if (lt == Layer::Type::Group)
            {
                const uint32_t num = fp->ReadUInt32();
                for (uint32_t i = 0; i < num; i++)
                {
                    Layer* chla = sq->CreateLayer(me);
                    if (chla == nullptr)
                    {
                        log->Printf(LT_ERROR, L"Could not create new layer");
                        return false;
                    }

                    if (!Read(chla, fp, sq, log, renderingTechnique, doLoad))
                    {
                        log->Printf(LT_ERROR, L"Could not read child");
                        return false;
                    }
                }
            }
            else if (lt == Layer::Type::Model ||
                lt == Layer::Type::Paint || lt == Layer::Type::Picture ||
                lt == Layer::Type::Sound || lt == Layer::Type::SpawnArea)
            {
                LayerImplementation lp = nullptr;


                if (lt == Layer::Type::Model)          lp = fiLayerModel::ReadData(fp, log);
                else if (lt == Layer::Type::Paint)     lp = fiLayerPaint::ReadData(fp, log, renderingTechnique);
                else if (lt == Layer::Type::Picture)   lp = fiLayerPicture::ReadData(fp, log);
                else if (lt == Layer::Type::Sound)     lp = fiLayerSound::ReadData(fp, log);
                else if (lt == Layer::Type::SpawnArea) lp = fiLayerSpawnArea::ReadData(fp, log);

                if (!lp)
                {
                    log->Printf(LT_ERROR, L"Could not init layer implementation \"%s\"", laName.GetS());
                    return false;
                }

                me->SetImplementation(lp);


            }
            return true;

        }           

        bool LoadAsset(Layer *me, piIStream *fp, Sequence *sq, piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique)
        {
            const uint32_t assetID = me->GetAssetID();
            if (assetID == 0xffffffff) return true;

            const Sequence::Asset* asset = sq->GetAsset(assetID);

            const uint64_t offsetCurrent = fp->Tell();

            fp->Seek(asset->mFileOffset, piIStreamArray::SeekMode::SET);

            const Layer::Type lt = me->GetType();
            LayerImplementation lp = me->GetImplementation();

            bool res = false;

            if (lt == Layer::Type::Model)          res = fiLayerModel::ReadAsset(lp, fp, log);
            else if (lt == Layer::Type::Paint)     res = fiLayerPaint::ReadAsset(lp, fp, log, colorSpace, renderingTechnique, (me->GetTransformToWorld().mFlip != flip3::N));
            else if (lt == Layer::Type::Picture)   res = fiLayerPicture::ReadAsset(lp, fp, log);
            else if (lt == Layer::Type::Sound)     res = fiLayerSound::ReadAsset(lp, fp, log);
            else if (lt == Layer::Type::SpawnArea) res = fiLayerSpawnArea::ReadAsset(lp, fp, log);

            if (!res)
                return false;

            fp->Seek(offsetCurrent, piIStreamArray::SeekMode::SET);

            return true;
        }

    }

}
