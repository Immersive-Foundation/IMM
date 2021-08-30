#pragma once

#include <functional>

#include "layer.h"
#include "layerPaint.h"

namespace ImmExporter
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

    Sequence();
    ~Sequence();

    bool Init( const Type & type, uint8_t caps, const ImmCore::vec3 & backgroundColor, uint32_t frameRate);
    void Deinit();

	Type     GetType(void) const;
    uint8_t GetCaps(void) const;
	Layer *  GetRoot(void) const;
    ImmCore::vec3 GetBackgroundColor(void) const;
    Layer* GetLayer(uint64_t layerID);
    const uint64_t GetNumLayers() const;
    inline uint32_t GetFrameRate() const { return mFrameRate; };
    void SetFrameRate(uint32_t fps); // TODO: this is only temporarily needed before we serialize global framerate at sequence level

	// pure memory manager. Does not handle hierarchy or resource allocations
    Layer* CreateLayer(Layer* parent);
    Layer* FindLayerByFullName(const wchar_t* name);

	void SetInitialSpawnArea(const Layer* vp);
	const Layer* GetInitialSpawnArea(void) const;

    bool Recurse(Layer::IMMVisitorF v, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec);
    void GetInfo(Type *resType, bool *resHasSound, int*umSpawnAreas);

    ImmCore::vec3 GetBackgroundColor();
private:
    ImmCore::piTArray<Layer*> mLayers; // just a flat array. Hierarchy is captured through the child pointers in LayerGroup
    ImmCore::vec3 mBackgroundColor;
    Layer* mRoot;
	Type mType;
    uint8_t mCaps;  // bit: can be grabbed
    uint32_t mFrameRate;
	bool mAnimateOnStart;
    bool mSpawnAreaNeedsUpdate;
    const Layer* mInitialSpawnArea;
};

} // namespace ImmExporter
