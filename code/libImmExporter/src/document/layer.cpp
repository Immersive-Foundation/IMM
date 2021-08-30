#include <malloc.h>

#include "layer.h"
#include "layerModel3d.h"
#include "layerPaint.h"
#include "layerPicture.h"
#include "layerSound.h"
#include "layerSpawnArea.h"

#include "sequence.h"
using namespace ImmCore;

namespace ImmExporter
{
	Layer::Layer(Sequence* sq, Layer* parent, uint32_t id)
	{
		mSequence = sq;
		mParent = parent;
		mID = id;
	}

	Layer::~Layer() {}

	bool Layer::Init(Type type, const wchar_t* name, bool visible, const trans3d & transform, const trans3d& pivot, float opacity,
                        bool isTimeLine, piTick duration, uint32_t maxRepeatCount)
	{
		mType = type;
		mVisible = visible;
		mPotentiallyVisible = visible;
		mImplementation = nullptr;
		mTransform = transform;
        mPivotTransform = pivot;
		mOpacity = opacity;
		mIsTimeline = isTimeLine;
		mDuration = duration;
		mMaxRepeatCount = maxRepeatCount;
		mDrawInTime = 1.0;
		mAnimParams[0] = 0.0;
		mAnimParams[1] = 0.7;
		mAnimParams[2] = 0.3;
		mAnimParams[3] = 0.7;

		if (!mName.InitCopyW(name))
			return false;

		if (mParent != nullptr)
		{
			if (!mParent->mChildren.Append(this, true))
				return false;
		}

		if (!iCalcFullName())
			return false;

		if (type == Type::Group)
		{
			if (!mChildren.Init(16, true))
				return false;
		}

		// Initialize an array of keys for each property
		const int maxProps = static_cast<int>(AnimProperty::MAX);
		for (int i = 0; i < maxProps; i++)
		{
			if (!mAnimKeys[i].Init(1, true))
				return false;
		}

		return true;
	}

	void Layer::Deinit()
	{
		if (mType == Type::Group)
		{
			uint64_t num = mChildren.GetLength();
			for (uint64_t i = 0; i < num; i++)
			{
				Layer* la = mChildren.Get(i);
				la->Deinit();
			}
		}

        const int maxProps = static_cast<int>(AnimProperty::MAX);
        for (int i = 0; i < maxProps; i++)
        {
            mAnimKeys[i].End();
        }

        mName.End();
        mFullName.End();
        if (mType == Type::Group)
        {
            mChildren.End();
        }

		if (mType == Type::Paint)
		{
			LayerPaint *li = (LayerPaint*)mImplementation;
			li->Deinit();
			delete li;
		}
		else if (mType == Type::Model)
		{
			LayerModel *li = (LayerModel*)mImplementation;
			li->Deinit();
			delete li;
		}
		else if (mType == Type::Picture)
		{
			LayerPicture *li = (LayerPicture*)mImplementation;
			li->Deinit();
			delete li;
		}
		else if (mType == Type::Sound)
		{
			LayerSound *li = (LayerSound*)mImplementation;
			li->Deinit();
			delete li;
		}
		else if (mType == Type::SpawnArea)
		{
			LayerSpawnArea *li = (LayerSpawnArea*)mImplementation;
			li->Deinit();
			delete li;
		}
	}

	const trans3d Layer::GetTransformToWorld(void) const
	{
		trans3d mat = mTransform;

		Layer* la = mParent;
		while (la != nullptr)
		{
			mat = la->mTransform * mat;
			la = la->mParent;
		}
		return mat;
	}

	bool Layer::iCalcFullName(void)
	{
		Layer* stack[64];

		stack[0] = this;
		int num = 0;

		Layer* la = mParent;
		while (la != nullptr)
		{
			num++;
			stack[num] = la;
			la = la->mParent;
		}

		if (!mFullName.InitCopy(&stack[num]->GetName()))
			return false;

		for (int i = num - 1; i >= 0; i--)
		{
			mFullName.AppendWC(L'/');
			mFullName.AppendS(&stack[i]->GetName());
		}
		return true;
	}

	const piString* Layer::GetFullName(void) const { return &mFullName; }

	KeepAlive * Layer::GetKeepAlive(void)
	{
		return &mKeepAlive;
	}

	LayerImplementation Layer::GetImplementation(void) const { return (LayerImplementation)mImplementation; }

	void Layer::SetImplementation(LayerImplementation implementation)
	{
		mImplementation = implementation;
	}

	const bool Layer::GetVisible(void) const { return mVisible; }

	const bool Layer::GetPotentiallyVisible(void) const { return mPotentiallyVisible; }

    bool Layer::GetLocalTimeOffsetAndVisiblity(piTick rootTime)
    {
        // Make a stack of parents
        int numParents = 0;
        Layer* parentStack[64];
        Layer* tmp = mParent;
        while (tmp)
        {
            parentStack[numParents++] = tmp;
            tmp = tmp->GetParent();
        }

        piTick rootToLocal = piTick(0);
        piTick localTime = rootTime;

        // Walk from root
        for (int64_t i = numParents - 1; i >= 0; i--)
        {
            const int numVisKeys = parentStack[i]->GetNumAnimKeys(Layer::AnimProperty::Visibility);
            bool foundKey = false;

            for (int j = numVisKeys - 1; j >= 0; j--)
            {
                const Layer::AnimKey* key = parentStack[i]->GetAnimKey(Layer::AnimProperty::Visibility, j);
                if (key->mTime <= localTime)
                {
                    // we're at the first key before time
                    if (key->mValue.mBool == true)
                    {
                        // we're in a span
                        if (parentStack[i]->GetIsTimeline())
                        {
                            rootToLocal += key->mTime;
                            localTime -= key->mTime;
                        }
                        foundKey = true;
                        break;
                    }
                    else
                    {
                        foundKey = false;
                        break;
                    }
                }
            }
            // we're not world visible at this point
            if (foundKey == false)
                return false;
        }

        return true;
    }

    const bool Layer::GetWorldVisible(void) const
    {
        bool visible = mVisible;
        if (visible)
        {
            Layer* tmpLa = mParent;
            while (tmpLa)
            {
                if (!tmpLa->mVisible) { visible = false; break; }
                tmpLa = tmpLa->mParent;
            }
        }
        return visible;
    }

	void Layer::SetVisible(bool visible)
    {
        mVisible = visible;
    }

	void Layer::SetPotentiallyVisible(bool visible) { mPotentiallyVisible = visible; }

	const Layer::Layer::Type Layer::GetType(void) const { return mType; }

	const piString& Layer::GetName(void) const { return mName; }

	const float Layer::GetOpacity(void) const { return mOpacity; }

    const float Layer::GetWorldOpacity(void) const
    {
        float opacity = mOpacity;

        Layer* tmpLa = mParent;
        while (tmpLa)
        {
            opacity = opacity * tmpLa->GetOpacity();
            tmpLa = tmpLa->mParent;
        }

        return opacity;
    }

	void Layer::SetOpacity(float opacity)
	{
		mOpacity = opacity;
	}

	const trans3d Layer::GetTransform(void) const { return mTransform; }

	void Layer::SetTransform(const trans3d & mat) { mTransform = mat; }

	void Layer::SetDrawInTime(double v) { mDrawInTime = v; }

	const double Layer::GetDrawInTime(void) const { return mDrawInTime; }

	const uint32_t Layer::GetID(void) const
	{
        return mID;
	}

	const bool Layer::GetLayerUsesDrawin(void) const
	{
		return mAnimKeys[static_cast<int>(AnimProperty::DrawInTime)].GetLength() > 0;
	}

	const double Layer::GetAnimParam(int id) const
	{
		return mAnimParams[id];
	}

	const bool Layer::HasBBox(uint32_t frameID) const
	{
		if (mType == Type::Group)   return mChildren.GetLength() > 0;
		if (mType == Type::Paint)   return ((LayerPaint*)mImplementation)->GetDrawingInFrame(frameID)->GetNumElements() > 0;
		if (mType == Type::Picture) return true;
		if (mType == Type::Model)   return true;

		return false;
	}

	const bound3d Layer::GetBBox(uint32_t frameID)
	{
		if (mType == Type::Paint)   return f2d(((LayerPaint  *)mImplementation)->GetBoundingBox(frameID));
		if (mType == Type::Picture) return f2d(((LayerPicture*)mImplementation)->GetBBox());
		if (mType == Type::Model)   return f2d(((LayerModel  *)mImplementation)->GetBBox());
		if (mType == Type::Group)
		{
			bound3d bbox = bound3d(1e30);    // this whole thing can probably be cached as long as no children or subchildren has changed position, oritnetation, scale or content bounds
			const uint64_t numChildren = mChildren.GetLength();
			for (uint64_t i = 0; i < numChildren; i++)
			{
				Layer* ch = mChildren.Get(i);
				if (ch->HasBBox(frameID))
				{
					bound3d tmp = ch->GetBBox(frameID);
					tmp = btransform(tmp, ch->GetTransform());
					bbox = include(bbox, tmp);
				}
			}

			return bbox;
		}

		piAssert(false); // we should never arrive here
		return bound3d(1e30);
	}

	const uint32_t Layer::GetNumChildren(void) const { return static_cast<uint32_t>(mChildren.GetLength()); }

	Layer* Layer::GetChild(uint32_t id) { return mChildren.Get(id); }

	Layer* Layer::GetParent() const { return mParent; }

	bool Layer::Recurse(IMMVisitorF func, int level, unsigned int child, bool instance, bool doNotRecurseCollapsedGroups,
                           bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec)
	{
		if (!doPostExec)
		{
			if (!func(this, level, child, instance))
				return false;
		}

		if (mType == Type::Group)
		{
			if (!(doNotRecurseHiddenGroups && !this->mVisible))
			{
				const uint64_t numChildren = mChildren.GetLength();
				for (uint64_t i = 0; i < numChildren; i++)
				{
					Layer* ch = mChildren.Get(i);
					if (!ch->Recurse(func, level + 1, static_cast<unsigned int>(i), instance, doNotRecurseCollapsedGroups,
                        doNotRecurseHiddenGroups, doNotRecurseLockedGroups, doPostExec))
						return false;
				}
			}
		}

		if (doPostExec)
		{
			if (!func(this, level, child, instance))
				return false;
		}


		return true;
	}

	const bool Layer::GetIsTimeline(void) const
	{
		return mIsTimeline;
	}

	Layer * Layer::GetTimeline()
	{
		Layer* layer = mParent;
		while (layer != nullptr && layer->GetIsTimeline() == false)
			layer = layer->GetParent();

		if (layer == nullptr)
			return mSequence->GetRoot();
		else
			return layer;
	}

	uint32_t Layer::GetMaxRepeatCount() const
	{
		return mMaxRepeatCount;
	}

	const piTick Layer::GetDuration() const
	{
		return mDuration;
	}

    void Layer::SetMaxRepeatCount(uint32_t count)
    {
        mMaxRepeatCount = count;
    }

    void Layer::SetIsTimeline(bool timeline)
    {
		    mIsTimeline = timeline;
    }

    void Layer::SetDuration(piTick duration)
    {
        mDuration = duration;
    }

	const uint32_t Layer::GetNumAnimKeys(AnimProperty property) const
	{
		return static_cast<uint32_t>(mAnimKeys[static_cast<int>(property)].GetLength());
	}

	const Layer::AnimKey* Layer::GetAnimKey(AnimProperty property, unsigned int index) const
	{
		if (index >= mAnimKeys[static_cast<int>(property)].GetLength())
			return nullptr;

		return mAnimKeys[static_cast<int>(property)].GetAddress(index);
	}

    const Layer::AnimKey * Layer::GetAnimKeyAt(AnimProperty property, piTick time) const
    {
        const int propIndex = static_cast<int>(property);
        const uint64_t numKeys = mAnimKeys[propIndex].GetLength();
        for (uint64_t i = 0; i < numKeys; i++)
        {
            AnimKey * key = mAnimKeys[propIndex].GetAddress(i);
            if (key->mTime == time)
                return key;
        }
        return nullptr;
    }

    const trans3d Layer::GetPivot() const
    {
        return mPivotTransform;
    }

	bool Layer::AddKey(piTick time, AnimProperty property, const AnimValue& value, InterpolationType interpolation)
	{
		AnimKey* key;
		const int propIndex = static_cast<int>(property);

		// Find the key
		int64_t keyIndex = -1;
		bool found = false;
		const uint64_t numKeys = mAnimKeys[propIndex].GetLength();
		for (uint64_t i = 0; i < numKeys; i++)
		{
			AnimKey * key = mAnimKeys[propIndex].GetAddress(i);

			if (key->mTime == time)
				found = true;

			if (key->mTime >= time)
			{
				keyIndex = i;
				break;
			}
		}

		if (found)
		{
			key = mAnimKeys[propIndex].GetAddress(keyIndex);
		}
		else
		{
			// Insert in place so the list stays sorted
			AnimKey newkey;
			if (keyIndex == -1)
				key = mAnimKeys[propIndex].Append(&newkey, true);
			else
				key = mAnimKeys[propIndex].InsertAndShift(&newkey, keyIndex, true);
		}

		key->mTime = time;
		key->mValue = value;
		key->mInterpolation = interpolation;

		return true;
	}

    void Layer::SetPivot(const trans3d& p)
    {
        mPivotTransform = p;
    }


}
