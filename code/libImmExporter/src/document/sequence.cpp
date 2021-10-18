#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piStr.h"

#include "sequence.h"
using namespace ImmCore;

namespace ImmExporter
{


Sequence::Sequence() {}

Sequence::~Sequence() {}

bool Sequence::Init(const Type & type, uint8_t caps, const Requirements & reqs, const vec3 & backgroundColor, uint32_t frameRate)
{
    mBackgroundColor = backgroundColor;
	mType = type;
	mCaps = caps;
    mSpawnAreaNeedsUpdate = false;
    mFrameRate = frameRate;
    mRequirements = reqs;

	if (!mLayers.Init(256, true)) // this array grows as needed
        return false;

	mRoot = this->CreateLayer(nullptr);
	if (!mRoot)
		return false;

    return true;
}

void Sequence::Deinit()
{
	// recursively destroy all resources
	mRoot->Deinit();
	// delete layer manager
    mLayers.End();
}

const Layer* Sequence::GetInitialSpawnArea(void) const
{
	return mInitialSpawnArea;
}

void Sequence::SetInitialSpawnArea(const Layer* la)
{
	mInitialSpawnArea = la;
}

Layer* Sequence::GetRoot(void) const { return mRoot; }

vec3 Sequence::GetBackgroundColor(void) const { return mBackgroundColor; }

Layer* Sequence::GetLayer(uint64_t layerID)
{
    return mLayers.Get(layerID);
}

const uint64_t Sequence::GetNumLayers() const
{
    return mLayers.GetLength();
}

void Sequence::SetFrameRate(uint32_t fps)
{
    mFrameRate = fps;
}

Layer* Sequence::CreateLayer(Layer* parent)
{
	const int id = static_cast<int>(mLayers.GetLength());

	Layer* la = new Layer(this, parent, id);

    if (!mLayers.Append(la, true))
        return nullptr;


    return la;
}

bool Sequence::Recurse(Layer::IMMVisitorF v, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec)
{
    return mRoot->Recurse(v, 0, 0, false, doNotRecurseCollapsedGroups, doNotRecurseHiddenGroups, doNotRecurseLockedGroups, doPostExec);
}

Layer* Sequence::FindLayerByFullName(const wchar_t* name)
{
    const uint64_t num = mLayers.GetLength();
    for (uint64_t i = 0; i < num; i++)
    {
        Layer* la = mLayers.Get(i);
        const piString* fullName = la->GetFullName();
        if (piwstrcmp(fullName->GetS(), name) == 0)
            return la;
    }
    return nullptr;
}

Sequence::Type Sequence::GetType(void) const
{
	return mType;
}

uint8_t Sequence::GetCaps(void) const
{
	return mCaps;
}

const Sequence::Requirements * Sequence::GetRequirements(void) const
{
    return &mRequirements;
}

void Sequence::GetInfo(Type *resType, bool *resHasSound, int* numSpawnAreas)
{
    Type oType = Type::Still;
    bool oHasSound = false;

    bool hasAnimation = false;
    int numSA = 0;
    auto visitor = [&hasAnimation, &oHasSound, &numSA](Layer* layer, int level, unsigned int child, bool instance) -> bool
    {
        Layer::Type type = layer->GetType();

        // Keyframe animation
        if (type != Layer::Type::SpawnArea && (layer->GetNumAnimKeys(Layer::AnimProperty::Visibility) > 1
            || layer->GetNumAnimKeys(Layer::AnimProperty::Opacity) > 1
            || layer->GetNumAnimKeys(Layer::AnimProperty::Transform) > 1))
        {
            hasAnimation = true;
        }

        if (type == Layer::Type::Paint)
        {
            // Frame by frame animation
            const LayerPaint*lp = (LayerPaint*)layer->GetImplementation();
            if (lp->GetNumDrawings() > 1)
                hasAnimation = true;
        }
        else if (type == Layer::Type::Sound)
        {
            oHasSound = true;
        }
        else if (type == Layer::Type::SpawnArea)
        {
            numSA++;
        }
        return true;
    };

    this->Recurse(visitor, false, false, false, false);

    oType = hasAnimation ? Type::Animated : Type::Still;

    // Detect if the timeline has STOPs, and if so, we flag the piece as
    // a COMIC, to signal the user in Spaces' UI what they are about to watch.
    if (mRoot->GetNumAnimKeys(Layer::AnimProperty::Action) > 0)
    {
        oType = Type::Comic;
    }

    *resType = oType;
    *resHasSound = oHasSound;
    *numSpawnAreas = numSA;
}

ImmCore::vec3 Sequence::GetBackgroundColor()
{
    return mBackgroundColor;
}

} // namespace ImmExporter
