#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piStreamArrayI.h"
#include "libCore/src/libBasics/piStreamFileI.h"

#include "../document/layerPaint/element.h"
#include "../document/layer.h"
#include "../document/layerPaint.h"
#include "../document/layerModel3d.h"
#include "../document/layerPicture.h"
#include "../document/layerSpawnArea.h"
#include "../document/sequence.h"
#include "fromImmersiveLayer.h"
#include "fromImmersiveLayerPaint.h"
#include "fromImmersiveUtils.h"

#include <algorithm>
#include <thread>
#include "fromImmersive.h"

using namespace ImmCore;



namespace ImmImporter
{

    static volatile bool doLoad = true;
    static volatile bool doneLoading = true;


    static bool iReadSceneGraph(piIStream* fp, Sequence* sq, piLog* log, Sequence::Type type, uint16_t caps, Drawing::PaintRenderingTechnique  renderingTechnique)
    {

        piString initialSpawnArea;
        if (!iReadString(fp, initialSpawnArea))
        {
            initialSpawnArea.End();
            return false;
        }


        const vec3 col = iReadVec3f(fp);

        if (!sq->Init(type, caps, col))
        {
            initialSpawnArea.End();
            return false;
        }

        Layer *root = sq->GetRoot();

        if (!fiLayer::Read(root, fp, sq, log, renderingTechnique, &doLoad))
        {
            log->Printf(LT_ERROR, L"Could not deserialize root.");
            initialSpawnArea.End();
            return false;
        }

        Layer* vpla = sq->FindLayerByFullName(initialSpawnArea.GetS());
        if (nullptr != vpla)
        {
            LayerSpawnArea* vp = (LayerSpawnArea*)vpla->GetImplementation();
            if (0 < vp->GetVersion())
            {
                sq->SetInitialSpawnArea(vpla);
            }
        }
        initialSpawnArea.End();
        return true;
    }




    static bool iReadAssetTable(piIStream *fp, Sequence* sq, piLog* log)
    {
        const uint32_t num = fp->ReadUInt32();
        for (uint32_t i = 0; i < num; i++)
        {
            const uint32_t layerID = fp->ReadUInt32();
            const uint64_t offset = fp->ReadUInt64();
            const uint64_t size = fp->ReadUInt64();
            if (!sq->AddAsset(offset, size))
                return false;
        }
        return true;
    }

    //---------------------

    constexpr uint64_t kSig_Immersiv = 0x76697372656d6d49;
    constexpr uint64_t kSig_Category = 0x79726f6765746143;
    constexpr uint64_t kSig_CoordSys = 0x73795364726f6f43;
    constexpr uint64_t kSig_Sequence = 0x65636e6575716553;
    constexpr uint64_t kSig_ResTable = 0x656c626174736552;


    bool IsLoadingAsync()
    {
        return !doneLoading;
    }

    bool IsStoppedLoading()
    {
        return doLoad==false;
    }


    void StopLoadingAsync()
    {
        doLoad = false;
    }

    static bool iImportStream(piIStream* fp, const wchar_t* filename, Sequence* sq, piLog* log, const Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique)
    {
        int numChunks = 0;

        doLoad = true;
        doneLoading = false;

        Sequence::Type sqType = Sequence::Type::Still;
        uint16_t sqCaps = 0;

        bool done = false;
        while (!done)
        {
            if (doLoad == false)
            {
                doneLoading = true;
                return false;
            }

            const uint64_t chunkSignature = fp->ReadUInt64();
            const uint64_t chunkSize = fp->ReadUInt64();
            const uint64_t currentOffset = fp->Tell();
            numChunks++;

            if (numChunks == 1 && chunkSignature != kSig_Immersiv)
            {
                return false;
            } // test validity of the file - first chunk must be of type kSig_Immersiv

            if (chunkSignature == kSig_Immersiv)
            {
                const uint32_t version = fp->ReadUInt32();
                if (version != 0x00010001)
                {
                    return false;
                }
            }
            else if (chunkSignature == kSig_CoordSys)
            {
                const uint8_t units = fp->ReadUInt8();
                const uint8_t axes = fp->ReadUInt8();
            }
            else if (chunkSignature == kSig_Category)
            {
                const uint8_t type = fp->ReadUInt8();
                const uint8_t caps = fp->ReadUInt8();
                const uint16_t size = fp->ReadUInt16();

                if (type >= static_cast<int>(Sequence::Type::COUNT))
                    return false;

                sqType = static_cast<Sequence::Type>(type);
                sqCaps = static_cast<uint16_t>(caps);
                const uint8_t axes = fp->ReadUInt8();
            }
            else if (chunkSignature == kSig_Sequence)
            {
                if (!iReadSceneGraph(fp, sq, log, sqType, sqCaps, renderingTechnique))
                {
                    return false;
                }
            }
            else if (chunkSignature == kSig_ResTable)
            {
                if (!iReadAssetTable(fp, sq, log))
                {
                    return false;
                }
                done = true;
            }

            fp->Seek(currentOffset + chunkSize, piIStreamArray::SeekMode::SET);
        }


        // Order the assets by the time they are needed
        struct LayerTime
        {
            Layer* mLayer;
            uint32_t mDrawingId;
            piTick mTime;
        };

        std::vector<LayerTime> sortedLayers;

        auto populateLayerNeededRootTime = [sq, &sortedLayers, fp, log, colorSpace,renderingTechnique](Layer* layer, int level, int child, bool instance) -> bool
        {
            Layer* root = sq->GetRoot();
            Layer* rootParent = layer;
            while (rootParent->GetTimeline() != root)
            {
                rootParent = rootParent->GetParent();
            }
            const piTick firstLayerAppearTime = rootParent->GetAnimKey(Layer::AnimProperty::Visibility, 0)->mTime;

            if (layer->GetType() == Layer::Type::Paint)
            {
                LayerPaint* lp = (LayerPaint*)layer->GetImplementation();

                if (!fiLayer::LoadAsset(layer, fp, sq, log, colorSpace, renderingTechnique))
                    log->Printf(LT_ERROR, L"Could not load asset for layer %s", layer->GetName().GetS());
                else
                    layer->SetLoaded(true);

                uint32_t* frames = lp->GetFrameBuffer();
                const uint32_t numDrawings = lp->GetNumDrawings();
                const uint32_t numFrames = lp->GetNumFrames();
                // Store individual drawings and offset each drawing by first frame appearance
                for (unsigned int i = 0; i < numDrawings; i++)
                {
                    for (unsigned int j = 0; j < numFrames; j++)
                    {
                        if (frames[j] == i)
                        {
                            sortedLayers.push_back({ layer, i, firstLayerAppearTime + piTick::FromFrames(j,lp->GetFrameRate()) });
                            break;
                        }
                    }
                }
            }
            else
            {
                sortedLayers.push_back({ layer, 0, firstLayerAppearTime });
            }
            return true;
        };
        sq->Recurse(populateLayerNeededRootTime, false, false, false, false);

        std::sort(sortedLayers.begin(), sortedLayers.end(), [](const LayerTime& a, const LayerTime& b)->bool { return a.mTime < b.mTime; });

        const piTick bufferingTime = piTick::FromSeconds(5.0);

        // Blocking load all assets needed for first 5 seconds
        for (auto i : sortedLayers)
        {
            if (doLoad == false)
            {
                doneLoading = true;
                return false;
            }

            if (i.mLayer->GetType() == Layer::Type::SpawnArea)
            {
                if (!fiLayer::LoadAsset(i.mLayer, fp, sq, log, colorSpace, renderingTechnique))
                {
                    log->Printf(LT_ERROR, L"Could not load asset for layer %s", i.mLayer->GetName().GetS());
                    return false;
                }
                i.mLayer->SetLoaded(true);
                continue;
            }

            if (i.mTime > bufferingTime) continue;

            if (i.mLayer->GetType() == Layer::Type::Paint)
            {
                const bool flipped = (i.mLayer->GetTransformToWorld().mFlip != flip3::N);
                if (!fiLayerPaint::ReadDrawing(i.mLayer->GetImplementation(), i.mDrawingId, fp, log, colorSpace, renderingTechnique, flipped))
                    log->Printf(LT_ERROR, L"Could not load drawing %d for layer %s",i.mDrawingId, i.mLayer->GetName().GetS());
                //else
                //    log->Printf(LT_MESSAGE, L"Loaded drawing %d for layer %s", i.mDrawingId, i.mLayer->GetName().GetS());
            }
            else
            {
                if (!fiLayer::LoadAsset(i.mLayer, fp, sq, log, colorSpace, renderingTechnique))
                {
                    log->Printf(LT_ERROR, L"Could not load asset for layer %s", i.mLayer->GetName().GetS());
                    return false;
                }
                i.mLayer->SetLoaded(true);
            }

        }

        // Async load assets in order they are needed
        std::thread assetLoadingThread([sortedLayers, fp, filename, sq, log, colorSpace, renderingTechnique, bufferingTime](void)->bool
        {

            piFile file;
            piIStream* stream;
            if (filename != nullptr)
            {
                if (!file.Open(filename, L"rb"))
                    return false;
                stream = new piIStreamFile(&file, piIStreamFile::kDefaultFileSize);
            }
            else stream = fp;


            for (auto i : sortedLayers)
            {
                if (doLoad == false)
                {
                    doneLoading = true;
                    if (filename != nullptr)
                    {
                        file.Close();
                    }
                    delete stream;
                    return false;
                }
                if (i.mTime <= bufferingTime) continue;

                if (i.mLayer->GetType() == Layer::Type::Paint)
                {
                    const bool flipped = (i.mLayer->GetTransformToWorld().mFlip != flip3::N);
                    if (!fiLayerPaint::ReadDrawing(i.mLayer->GetImplementation(), i.mDrawingId, stream, log, colorSpace, renderingTechnique, flipped))
                        log->Printf(LT_ERROR, L"Could not load drawing %d for layer %s", i.mDrawingId, i.mLayer->GetName().GetS());
#ifdef _DEBUG
                    else
                        log->Printf(LT_MESSAGE, L"Loaded drawing %d for layer %s", i.mDrawingId, i.mLayer->GetName().GetS());
#endif
                }
                else
                {
                    if (!fiLayer::LoadAsset(i.mLayer, stream, sq, log, colorSpace, renderingTechnique))
                    {
                        log->Printf(LT_ERROR, L"Could not load asset for layer %s", i.mLayer->GetName().GetS());
                        return false;
                    }
                    i.mLayer->SetLoaded(true);
                }
            }

            if (filename != nullptr)
            {
                file.Close();
            }

            delete stream;

            doneLoading = true;
            return true;
        }
        );

        assetLoadingThread.detach();


        return true;

    }

    bool ImportFromDisk(Sequence* sq, piLog* log, const wchar_t* filename, const Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique)
    {
        piFile fp;
        if (!fp.Open(filename, L"rb"))
            return false;

        piIStreamFile fstr(&fp, piIStreamFile::kDefaultFileSize);

        bool error = iImportStream(&fstr, filename, sq, log, colorSpace, renderingTechnique);

        fp.Close();

        return error;
	}

    bool ImportFromMemory(piTArray<uint8_t>* data, Sequence* sq, piLog* log, const Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique)
    {
        piIStreamArray* fstr = new piIStreamArray(data);
        return iImportStream(fstr, nullptr, sq, log, colorSpace, renderingTechnique);
    }

}
