#include "../document/layerPaint/element.h"
#include "../document/layerPaint.h"
#include "../document/layerModel3d.h"
#include "../document/layerPicture.h"
#include "../document/layerSpawnArea.h"
#include "../document/sequence.h"

#include "toImmersiveLayer.h"
#include "toImmersiveLayerPaint.h"
#include "toImmersiveLayerPicture.h"
#include "toImmersiveLayerSound.h"
#include "toImmersiveLayerModel.h"
#include "toImmersiveLayerSpawnArea.h"
#include "toImmersiveUtils.h"

using namespace ImmCore;
namespace ImmExporter
{

    namespace tiLayer
    {
#if 1
        static void iWriteTimeline(piOStream* fp, Layer* la)
        {
            fp->WriteUInt64(0x656e696c656d6954); // "Timeline"
            const uint64_t offset = iWriteForward64(fp);

            // version 0 has two less properties than version 1 (loop & offset)
            // version 2 adds "transform"

            const int version = 2;

            fp->WriteUInt32(version); // version 1
            const uint32_t maxProps = static_cast<int>(Layer::AnimProperty::MAX) + (version == 0 ? -3 : (version == 1 ? -1 : 0));

            for (uint32_t i = 0; i < maxProps; i++)
            {
                Layer::AnimProperty id = static_cast<Layer::AnimProperty>(i);

                const uint32_t numKeys = la->GetNumAnimKeys(id);
                fp->WriteUInt16(static_cast<uint16_t>(id));
                fp->WriteUInt64(numKeys);

                for (uint32_t j = 0; j < numKeys; j++)
                {
                    const Layer::AnimKey* key = la->GetAnimKey(id, j);

                    fp->WriteUInt64( piTick::CastInt(key->mTime));
                    fp->WriteUInt8(static_cast<uint8_t>(key->mInterpolation));

                    switch (id)
                    {
                    case Layer::AnimProperty::Action:
                        fp->WriteUInt32(key->mValue.mInt);
                        break;
                    case Layer::AnimProperty::DrawInTime:
                        fp->WriteDoublearray(&key->mValue.mDouble, 1);
                        break;
                    case Layer::AnimProperty::Opacity:
                        fp->WriteFloat(key->mValue.mFloat);
                        break;
                    case Layer::AnimProperty::Position:
                        fp->WriteDoublearray(&key->mValue.mTransform.mTranslation.x,3);
                        break;
                    case Layer::AnimProperty::Rotation:
                        fp->WriteDoublearray(&key->mValue.mTransform.mRotation.x, 4);
                        break;
                    case Layer::AnimProperty::Scale:
                        fp->WriteDoublearray(&key->mValue.mTransform.mScale, 1);
                        break;
                    case Layer::AnimProperty::Visibility:
                        fp->WriteUInt8(key->mValue.mBool ? 1 : 0);
                        break;
                    case Layer::AnimProperty::Loop:
                        fp->WriteUInt8(key->mValue.mBool ? 1 : 0);
                        break;
                    case Layer::AnimProperty::Offset:
                        fp->WriteUInt32(key->mValue.mInt);
                        break;
                    case Layer::AnimProperty::Transform:
                        iWriteTrans3d(fp, key->mValue.mTransform);
                        break;
                    }
                }
            }

            iWriteBack64(fp, offset, fp->Tell() - offset - 8);
        }
#else

        struct TimeLineData
        {
            piTArray<uint64_t> mVisibilityTime;
            piTArray<uint8_t>  mVisibilityInterpolation;
            piTArray<uint8_t>  mVisibilityValue;

            piTArray<uint64_t> mOpacityTime;
            piTArray<uint8_t>  mOpacityInterpolation;
            piTArray<float>    mOpacityValue;

            piTArray<uint64_t> mTransformTime;
            piTArray<uint8_t>  mTransformInterpolation;
            piTArray<trans3d>  mTransformValue;

            piTArray<uint64_t> mActionTime;
            piTArray<uint8_t>  mActionInterpolation;
            piTArray<uint32_t> mActionValue;

            piTArray<uint64_t> mDrawInTimeTime;
            piTArray<uint8_t>  mDrawInTimeInterpolation;
            piTArray<double>   mDrawInTimeValue;
        }mData;

        static void iWriteTimeline(piOStream *fp, Layer *la)
        {
            const uint32_t maxProps = static_cast<int>(Layer::AnimProperty::MAX);
            for (uint32_t i = 0; i < maxProps; i++)
            {
                Layer::AnimProperty id = static_cast<Layer::AnimProperty>(i);
                const uint64_t numKeys = la->GetNumAnimKeys(id);
                if (numKeys == 0) continue;

                for (uint64_t j = 0; j < numKeys; j++)
                {
                    const Layer::AnimKey* key = la->GetAnimKey(id, j);

                    if (id == Layer::AnimProperty::Visibility)
                    {
                        mData.mVisibilityTime.Append(key->mTime);
                        mData.mVisibilityInterpolation.Append(static_cast<uint8_t>(key->mInterpolation));
                        mData.mVisibilityValue.Append(key->mValue.mBool, true);
                    }
                    else if (id == Layer::AnimProperty::Opacity)
                    {
                        mData.mOpacityTime.Append(key->mTime);
                        mData.mOpacityInterpolation.Append(static_cast<uint8_t>(key->mInterpolation));
                        mData.mOpacityValue.Append(key->mValue.mFloat, true);
                    }
                    else if (id == Layer::AnimProperty::Transform)
                    {
                        mData.mTransformTime.Append(key->mTime);
                        mData.mTransformInterpolation.Append(static_cast<uint8_t>(key->mInterpolation));
                        mData.mTransformValue.Append(key->mValue.mTransform, true);
                    }
                    else if (id == Layer::AnimProperty::DrawInTime)
                    {
                        mData.mDrawInTimeTime.Append(key->mTime);
                        mData.mDrawInTimeInterpolation.Append(static_cast<uint8_t>(key->mInterpolation));
                        mData.mDrawInTimeValue.Append(key->mValue.mDouble, true);
                    }
                    else if (id == Layer::AnimProperty::Action)
                    {
                        mData.mActionTime.Append(key->mTime);
                        mData.mActionInterpolation.Append(static_cast<uint8_t>(key->mInterpolation));
                        mData.mActionValue.Append(key->mValue.mInt, true);
                    }
                }
            }
        }
#endif
        static const char *kLayerTypeStrs[] = {
            "Immersiv.Group",
            "Quill.Paint",
            "Quill.Effect",
            "Quill.Model",
            "Quill.Picture",
            "Quill.Sound",
            "Immersiv.Reference",
            "Immersiv.Instance",
            "Quill.SpawnArea"
        };


        static bool iExportLayer(piOStream *fp, Layer* la, piArray *delayed, bool *outWasExported, uint32_t fps)
        {
            const Layer::Type lt = la->GetType();

            *outWasExported = true;

            fp->WriteUInt64(0x736564726579614c); // "Layerdes"
            const uint64_t layerOffset = iWriteForward64(fp);

            // version 1 adds pivot from version 0
            const int version = 1;

            fp->WriteUInt32(version); // version

            //const bound3d laBbox = la->GetBBox();

            LayerImplementation imp = la->GetImplementation();

            trans3d layerTransform = la->GetTransform();

            iWriteTrans3d(fp, layerTransform);

            if (version>=1)
                iWriteTrans3d(fp, la->GetPivot()); // new in version 1

            fp->WriteFloat(la->GetOpacity());
            fp->WriteUInt8(la->GetIsTimeline() ? 1 : 0);
            fp->WriteUInt64(piTick::CastInt(la->GetDuration()));
            fp->WriteUInt32(la->GetMaxRepeatCount());
            iWriteString(fp, &la->GetName());
            iWriteString(fp, kLayerTypeStrs[static_cast<int>(lt)]);
            const uint64_t assetIDOffset = iWriteForward32(fp, 0xffffffff);

            // --- timeline -------------
            iWriteTimeline(fp, la);
            //---------------------------

            if (lt == Layer::Type::Group)
            {
                const unsigned int num = la->GetNumChildren();
                const uint64_t off = iWriteForward32(fp, num);
                uint32_t numChildrenExported = 0;
                for (unsigned int i = 0; i < num; i++)
                {
                    Layer* lc = la->GetChild(i);
                    bool we = false;
                    if (!iExportLayer(fp, lc, delayed, &we, fps))
                        return false;
                    if (we)
                        numChildrenExported++;
                }
                iWriteBack32(fp, off, numChildrenExported);
            }
            else if(lt == Layer::Type::Model || lt == Layer::Type::Paint || lt == Layer::Type::Picture || lt == Layer::Type::Sound || lt == Layer::Type::SpawnArea)
            {
                bool res = false;
                if (lt == Layer::Type::Model)     res = tiLayerModel::ExportData(fp, imp);
                else if (lt == Layer::Type::Paint)     res = tiLayerPaint::ExportData(fps, fp, imp);
                else if (lt == Layer::Type::Picture)   res = tiLayerPicture::ExportData(fp, imp);
                else if (lt == Layer::Type::Sound)     res = tiLayerSound::ExportData(fp, imp);
                else if (lt == Layer::Type::SpawnArea) res = tiLayerSpawnArea::ExportData(fp, imp);

                if (!res)
                    return false;

                const uint32_t assetID = static_cast<uint32_t>(delayed->GetLength());
                iWriteBack32(fp, assetIDOffset, assetID);

                DelayedSerialization ds = { la, 0 };
                delayed->Append(&ds, true);
            }

            iWriteBack64(fp, layerOffset, fp->Tell() - layerOffset- 8);

            return true;
        }

        bool ExportLayer(piOStream *fp, Layer* la, piArray *delayed, uint32_t fps)
        {
            bool wasExported = true;
            const bool res = iExportLayer(fp, la, delayed, &wasExported, fps);
            if (!wasExported) return false;
            return res;
        }
    }

}
