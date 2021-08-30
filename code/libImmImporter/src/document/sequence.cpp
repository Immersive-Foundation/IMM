#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piDebug.h"
#include "libImmCore/src/libBasics/piStr.h"

#include "sequence.h"
using namespace ImmCore;

namespace ImmImporter
{


Sequence::Sequence() {}

Sequence::~Sequence() {}

bool Sequence::Init(const Type & type, uint16_t caps, const vec3 & backgroundColor)
{
    mBackgroundColor = backgroundColor;
	mType = type;
	mCaps = caps;
    mSpawnAreaNeedsUpdate = false;
    mFrameRate = 0;  // TODO --- pass this as argument, and DESTROY the SetFramerate() function

	if (!mLayers.Init(256, true)) // this array grows as needed
        return false;

	if (!mAssetTable.Init(1024, false))
		return false;

	mRoot = this->CreateLayer(nullptr);
	if (!mRoot)
		return false;

    return true;
}

void Sequence::Deinit(piLog *log) 
{ 
	// recursively destroy all resources
	mRoot->Deinit(log); 	
	// delete layer manager
    mLayers.End(); 
	mAssetTable.End();
}

bool Sequence::AddAsset(uint64_t offset, uint64_t size)
{
	Asset *as = mAssetTable.Alloc(1, true);
	if (!as) return false;
	as->mFileOffset = offset;
	as->mSize = size;
	return true;
}

const Sequence::Asset* Sequence::GetAsset(uint32_t assetID) const
{
	return mAssetTable.GetAddress(static_cast<uint64_t>(assetID));
}

const Layer* Sequence::GetInitialSpawnArea(void) const
{
	return mInitialSpawnArea;
}

bool Sequence::GetSpawnAreaNeedsUpdate() const 
{
    return mSpawnAreaNeedsUpdate;
}

void Sequence::SetSpawnAreaNeedsUpdate(bool state)
{
    mSpawnAreaNeedsUpdate = state;
}


void Sequence::SetInitialSpawnArea(const Layer* la)
{
	mInitialSpawnArea = la;
}


Layer* Sequence::GetRoot(void) const { return mRoot; }

vec3 Sequence::GetBackgroundColor(void) const { return mBackgroundColor; }

Layer* Sequence::CreateLayer(Layer* parent)
{
	const int id = static_cast<int>(mLayers.GetLength());
	
	Layer* la = new Layer(this, parent, id);

    if (!mLayers.Append(la, true))
        return nullptr;


    return la;
}


bool Sequence::Recurse(VisitorF v, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec)
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


bool Sequence::GetAnimateOnStart(void) const
{
    return mAnimateOnStart;
}

void Sequence::SetAnimateOnStart(bool b)
{
	mAnimateOnStart = b;
}

Sequence::Type Sequence::GetType(void) const
{
	return mType;
}
uint16_t Sequence::GetCaps(void) const
{
	return mCaps;
}

// --- everything below is only needed for the exporter ---
uint32_t Sequence::GetFrameRate() const
{
    return mFrameRate;
};

void Sequence::SetFrameRate(uint32_t fps)
{
    mFrameRate = fps;
}

void Sequence::GetInfo(Type *resType, bool *resHasSound, int* numSpawnAreas) const
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

    mRoot->Recurse(visitor, 0, 0, false, false, false, false, false);

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



}
