#pragma once

#include <functional>

#include "libImmCore/src/libVR/piVR.h"

#include "layer.h"
#include "layerPaint.h"

namespace ImmImporter
{

class Sequence
{
public:

	typedef enum { NoComicFeatures = 0, ComicFeatures = 1 } FileVersion;

	enum class Type : int
	{
		Still = 0,
		Animated = 1,
		Comic = 2,
		COUNT = 3
	};

	struct Asset
	{
		uint64_t mFileOffset;
		uint64_t mSize;
	};

    Sequence();
    ~Sequence();

    bool Init( const Type & type, uint16_t caps, const ImmCore::vec3 & backgroundColor );
    void Deinit(ImmCore::piLog *log);

	Type     GetType(void) const;
	uint16_t GetCaps(void) const;
	Layer *  GetRoot(void) const;
    ImmCore::vec3 GetBackgroundColor(void) const;

	// pure memory manager. Does not handle hierarchy or resource allocations
    Layer* CreateLayer(Layer* parent);
    Layer* FindLayerByFullName(const wchar_t* name);
	bool   AddAsset(uint64_t offset, uint64_t size);
	const Asset* GetAsset(uint32_t assetID) const;

    bool GetAnimateOnStart(void) const;
	void SetAnimateOnStart(bool b);
	void SetInitialSpawnArea(const Layer* vp);
	const Layer* GetInitialSpawnArea(void) const;
    void SetSpawnAreaNeedsUpdate(bool state);
    bool GetSpawnAreaNeedsUpdate() const;

    typedef std::function<bool(Layer* layer, int level, int child, bool instance)> VisitorF;
    bool Recurse(VisitorF v, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec);

    // only needed for exporter
    void  GetInfo(Type *resType, bool *resHasSound, int*umSpawnAreas) const;
    uint32_t GetFrameRate() const;
    void SetFrameRate(uint32_t fps); // TODO: this is only temporarily needed before we serialize global framerate at sequence level

private:
    ImmCore::piTArray<Layer*> mLayers; // just a flat array. Hierarchy is captured through the child pointers in LayerGroup
	ImmCore::piTArray<Asset>  mAssetTable;
    ImmCore::vec3             mBackgroundColor;
    Layer *          mRoot;
	Type             mType;
	uint16_t         mCaps;  // bit: can be grabbed

	bool             mAnimateOnStart;
    bool             mSpawnAreaNeedsUpdate;
    const Layer*     mInitialSpawnArea;
    uint32_t         mFrameRate;

};

}
