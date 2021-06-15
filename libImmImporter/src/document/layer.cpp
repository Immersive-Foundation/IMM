#include <malloc.h>

#include "layer.h"
#include "layerInstance.h"
#include "layerModel3d.h"
#include "layerPaint.h"
#include "layerPicture.h"
#include "layerSound.h"
#include "layerSpawnArea.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piDebug.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"
#include "sequence.h"
using namespace ImmCore;

namespace ImmImporter
{
	Layer::Layer(Sequence* sq, Layer* parent, uint32_t id)
	{
		mSequence = sq;
		mParent = parent;
		mID = id;
	}

	Layer::~Layer() {}

	bool Layer::Init(Type type, const wchar_t* name, bool visible, const trans3d & transform, const trans3d& pivot, float opacity, bool isTimeLine, piTick duration, uint32_t maxRepeatCount, uint32_t assetID, piLog *log)
	{
		mType = type;
        mLoaded = false;
		mVisible = visible;
		mPotentiallyVisible = visible;
		mImplementation = nullptr;
		mTransform = transform;
        mPivotTransform = pivot;
		mOpacity = opacity;
		mIsTimeline = isTimeLine;
		mDuration = duration;
		mMaxRepeatCount = maxRepeatCount;
        mStartOffset = 0;
		mAssetID = assetID;
		mDrawInTime = 1.0;
		mAnimParams[0] = 0.0;
		mAnimParams[1] = 0.7;
		mAnimParams[2] = 0.3;
		mAnimParams[3] = 0.7;
        mLastActiveSpanStart = piTick(0);

		if (!mName.InitCopyW(name))
			return false;

		//log->Printf(LT_MESSAGE, mName.GetS());

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
			mImplementation = nullptr;
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


	void Layer::Deinit(piLog *log)
	{
		//log->Printf(LT_MESSAGE, L"  Layer::Deinit 1 : name : %s,   type: %d,    imp = %x", this->mName.GetS(), static_cast<int>(mType), (mImplementation ==nullptr)?0:uint64_t(mImplementation));

		if (mType == Type::Group)
		{
			uint64_t num = mChildren.GetLength();
			for (uint64_t i = 0; i < num; i++)
			{
				Layer* la = mChildren.Get(i);
				la->Deinit(log);
			}
		}

        const int maxProps = static_cast<int>(AnimProperty::MAX);
        for (int i = 0; i < maxProps; i++)
        {
            mAnimKeys[i].End();
        }

        mName.End();

        if (mType == Type::Group)
        {
            mChildren.End();
        }
        
        // don't deinit aync loaded layers not loaded (were not inited)
        if (!mLoaded) return;

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

	trans3d Layer::GetTransformToWorld(void) const
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

	bool* Layer::GetVisibleRef(void) { return &mVisible; }

	float * Layer::GetOpacityRef(void)
	{
		return &mOpacity;
	}

	bool Layer::GetVisible(void) const { return mVisible; }
	bool Layer::GetPotentiallyVisible(void) const { return mPotentiallyVisible; }
    bool Layer::GetWorldVisible(void) const
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


	void Layer::SetVisible(bool visible) { mVisible = visible; }
	void Layer::SetPotentiallyVisible(bool visible) { mPotentiallyVisible = visible; }

	const Layer::Type Layer::GetType(void) const { return mType; }


	const piString& Layer::GetName(void) const { return mName; }

	float Layer::GetOpacity(void) const { return mOpacity; }
    float Layer::GetWorldOpacity(void) const
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

	trans3d Layer::GetTransform(void) const { return mTransform; }

	void Layer::SetTransform(const trans3d & mat) { mTransform = mat; }

	void Layer::SetDrawInTime(double v) { mDrawInTime = v; }
	double Layer::GetDrawInTime(void) const { return mDrawInTime; }



	uint32_t Layer::GetID(void) const
	{
		return mID;
	}

	bool Layer::GetLayerUsesDrawin(void) const
	{
		return mAnimKeys[static_cast<int>(AnimProperty::DrawInTime)].GetLength() > 0;
	}


	double Layer::GetAnimParam(int id) const
	{
		return mAnimParams[id];
	}



	bool Layer::HasBBox(void) const
	{
		if (mType == Type::Group)   return mChildren.GetLength() > 0;
		if (mType == Type::Paint)   return ((LayerPaint  *)mImplementation)->HasBBox();
		if (mType == Type::Picture) return ((LayerPicture*)mImplementation)->HasBBox();
		if (mType == Type::Model)   return ((LayerModel  *)mImplementation)->HasBBox();

		return false;
	}

	const bound3d Layer::GetBBox(void)
	{
		if (mType == Type::Paint)   return f2d(((LayerPaint  *)mImplementation)->GetBBox());
		if (mType == Type::Picture) return f2d(((LayerPicture*)mImplementation)->GetBBox());
		if (mType == Type::Model)   return f2d(((LayerModel  *)mImplementation)->GetBBox());
		if (mType == Type::Group)
		{
			bound3d bbox = bound3d(1e30);    // this whole thing can probably be cached as long as no children or subchildren has changed position, oritnetation, scale or content bounds
			const uint64_t numChildren = mChildren.GetLength();
			for (uint64_t i = 0; i < numChildren; i++)
			{
				Layer* ch = mChildren.Get(i);
				if (ch->HasBBox())
				{
					bound3d tmp = ch->GetBBox();
					tmp = btransform(tmp, ch->GetTransform());
					bbox = include(bbox, tmp);
				}
			}

			return bbox;
		}

		piAssert(false); // we should never arrive here
		return bound3d(1e30);
	}

	uint32_t Layer::GetNumChildren(void) const { return static_cast<uint32_t>(mChildren.GetLength()); }

	Layer* Layer::GetChild(uint32_t id) { return mChildren.Get(id); }

	Layer* Layer::GetParent() const { return mParent; }

	bool Layer::Recurse(VisitorF func, int level, unsigned int child, bool instance, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec)
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
					if (!ch->Recurse(func, level + 1, static_cast<unsigned int>(i), instance, doNotRecurseCollapsedGroups, doNotRecurseHiddenGroups, doNotRecurseLockedGroups, doPostExec))
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

	bool Layer::GetPlaying() const
	{
		return mIsPlaying;
	}

	void Layer::SetPlaying(bool playing)
	{
		mIsPlaying = playing;
	}

	bool Layer::GetIsTimeline(void) const
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

	void Layer::SetStartTime(piTick time)
	{
		mStartTime = time;
	}

	piTick Layer::GetStartTime() const
	{
		return mStartTime;
	}

	void Layer::SetStopTime(piTick time)
	{
		mStopTime = time;
	}

	piTick Layer::GetStopTime() const
	{
		return mStopTime;
	}


	piTick Layer::GetDuration() const
	{
		return mDuration;
	}

	void Layer::SetPosition(const vec3d & pos)
	{
		mTransform.mTranslation = pos;
	}

	void Layer::SetRotation(const quatd & rot)
	{
		mTransform.mRotation = rot;
	}

	void Layer::SetScale(double scale)
	{
		mTransform.mScale = scale;
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

	uint32_t Layer::GetNumAnimKeys(AnimProperty property) const
	{
		return static_cast<uint32_t>(mAnimKeys[static_cast<int>(property)].GetLength());
	}

	const Layer::AnimKey* Layer::GetAnimKey(AnimProperty property, unsigned int index) const
	{
		if (index >= mAnimKeys[static_cast<int>(property)].GetLength())
			return nullptr;

		return mAnimKeys[static_cast<int>(property)].GetAddress(index);
	}

    bool Layer::GetLoaded(void) const
    {
        return mLoaded;
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

    void Layer::SetStateAt(piTick now, piLog* log)
    {
        Layer* root = mSequence->GetRoot();
        const bool isRoot = this == root;
        const bool isGroup = mType == Type::Group;
        const bool isTimeline = isGroup && mIsTimeline;
        const bool isPaintLayer = mType == Type::Paint;
        const bool isSpawnArea = mType == Type::SpawnArea;
        LayerPaint* lp = isPaintLayer ? (LayerPaint*)GetImplementation() : nullptr;
        const bool isSoundLayer = mType == Type::Sound;
        LayerSound * ls = isSoundLayer ? (LayerSound*)GetImplementation() : nullptr;

        Layer* timeline = GetTimeline();
        const piTick timelineDuration = timeline->GetDuration();

        if (mParent && !mParent->GetWorldVisible())
        {
            if (isSoundLayer) ls->SetPlaying(false);
            return;
        }
            

        // handle stop frames to correct for stop time
        piTick timeFromStart = timeline->GetStopTime() > timeline->GetStartTime() ?
            timeline->GetStopTime() - timeline->GetStartTime()
            : now - timeline->GetStartTime();

        const piTick timelineWrapped = (timelineDuration != 0) ? timeFromStart % timelineDuration : timeFromStart;

        // handle looping clips
        const int64_t numPlays = (timelineDuration != 0) ? piTick::CastInt(timeFromStart) / piTick::CastInt(timelineDuration) : 0;
        const piTick timelineTime = (timeline->GetMaxRepeatCount() == 0 || numPlays < timeline->GetMaxRepeatCount()) ?
            timelineWrapped : timelineDuration;

        piTick timeSinceStart = piTick(0);

        const int maxProps = static_cast<int>(AnimProperty::MAX);
        for (int i = 0; i < maxProps; i++)
        {
            const AnimProperty prop = static_cast<AnimProperty>(i);

            const int numKeys = GetNumAnimKeys(prop);
            if (numKeys == 0) continue; // skip property

            // Find the last key, assuming keys are sorted
            AnimKey* lastKey = nullptr;
            int lastKeyIndex = -1;
            for (int j = 0; j < numKeys; j++)
            {
                const AnimKey* key = GetAnimKey(prop, j);
                // we only start a key if we're past it.
                // this allows stopping at the exact time where keys start without starting them
                if (key->mTime > timelineTime)
                    break;
                lastKeyIndex = j;
                lastKey = (AnimKey*)key;
            }

            // Time is before the first key, do nothing
            if (lastKeyIndex == -1)
            {
                if (prop == AnimProperty::Visibility)
                {
                    mVisible = false;
                    if (isSoundLayer) ls->SetPlaying(false);
                    continue;
                }
                else if (prop == AnimProperty::Action)
                {
                    if (isSpawnArea) ResetActionStatesAt(piTick(0));
                    continue;
                }
                lastKey = (AnimKey*)GetAnimKey(prop, 0);
                lastKeyIndex = numKeys - 1;
            }


            // Find the next key is there is one
            const bool isLastKey = lastKeyIndex == numKeys - 1;
            const AnimKey* nextKey = isLastKey ? nullptr : GetAnimKey(prop, lastKeyIndex + 1);

            // interpolation param between last and next keys
            const piTick totalTime = isLastKey ? piTick(0) : nextKey->mTime - lastKey->mTime;
            double t = isLastKey ? 0.0 : piTick::CastDouble(timelineTime - lastKey->mTime) / piTick::CastDouble(totalTime);

            // apply easing curve if needed
            switch (lastKey->mInterpolation)
            {
            case InterpolationType::EaseIn:
                t = t * t*t;
                break;
            case InterpolationType::EaseOut:
            {
                const double t1 = 1.0 - t;
                t = 1.0 - (t1*t1*t1);
                break;
            }
            case InterpolationType::Smoothstep:
                t = t * t*(3.0 - 2.0*t);
                break;
            default:
                break;
            }

            // set the state from the keys
            switch (prop)
            {
            case AnimProperty::Action:
            {
                AnimAction action = static_cast<AnimAction>(lastKey->mValue.mInt);
                if (lastKey->mValue.mBool == false)
                {
                    lastKey->mValue.mBool = true; // flag to only run actions once

                    if (action == AnimAction::Stop)
                    {
                        SetStopTime(GetStartTime() + lastKey->mTime + piTick(-1));
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Stop action %s", mName.GetS());
#endif
                    }
                    if (action == AnimAction::MakeDefault && isSpawnArea)
                    {
                        mSequence->SetInitialSpawnArea(this);
                        mSequence->SetSpawnAreaNeedsUpdate(true);
                        log->Printf(LT_DEBUG, L"[LayerSetState] SpawnArea action %s", mName.GetS());
                    }
                }
                break;
            }
            case AnimProperty::Visibility:
            {
                // Visibility controls if some layers are alive or dead for playback

                 // span change since last evaluation retriggers sounds
                const bool isNewSpan = lastKey->mTime != mLastActiveSpanStart;
                if (isNewSpan) mLastActiveSpanStart = lastKey->mTime;

                if (lastKey->mValue.mBool)
                {
                    // If there is an offset key at this time, we need to run it first
                    const AnimKey* offsetKey = nullptr;
                    const int numOffsetKeys = GetNumAnimKeys(AnimProperty::Offset);
                    for (int j = 0; j < numOffsetKeys; j++)
                    {
                        const AnimKey* key = GetAnimKey(AnimProperty::Offset, j);
                        if (key->mTime == lastKey->mTime)
                        {
                            offsetKey = key;
                            break;
                        }
                    }
                    if (offsetKey != nullptr) mStartOffset = piTick(int64_t(offsetKey->mValue.mInt));

                    timeSinceStart = (timelineTime - lastKey->mTime) + mStartOffset;
                    const piTick startTime = now - timeSinceStart;

                    if (!timeline->GetPlaying())
                        break;
                    
                    // looping timeline
                    bool isParentSequenceLooping = false;
                    Layer* ti = timeline;
                    while (ti != root)
                    {
                        if (ti->GetMaxRepeatCount() == 0 && ti->GetDuration() != 0)
                        {
                            isParentSequenceLooping = true;
                            break;
                        }
                        ti = ti->GetParent();
                    }

                    // this could be due to parent sequence looping or restart from a pause
                    const bool hasRestarted = startTime > mStartTime;
                    const bool clipRestartedAndParentLooping = isParentSequenceLooping && hasRestarted;
                   
                    if (isTimeline && ( (!mIsPlaying && !isRoot) || clipRestartedAndParentLooping))
                    {
                        SetPlaying(true);
                        SetStartTime(startTime);
                        SetStopTime(startTime - 1);
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Starting timeline %s", mName.GetS());
#endif
                    }
                    else if (isPaintLayer && lp->GetNumFrames() > 1 && !lp->GetPlaying())
                    {
                        lp->SetTime(timeSinceStart);
                        lp->SetPlaying(true);
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Starting anim clip %s", mName.GetS());
#endif
                    }
                    else if (isSoundLayer && ((!mVisible && !ls->GetPlaying()) || isNewSpan || clipRestartedAndParentLooping))
                    {
                        mStartTime = startTime;
                        ls->SetPlaying(true, piTick::ToMicroSeconds(timeSinceStart));
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Starting sound %s", mName.GetS());
#endif
                    }
                }
                else if (!lastKey->mValue.mBool)
                {
                    const piTick timeSinceStop = (timelineTime - lastKey->mTime) + mStartOffset;
                    const piTick stopTime = now - timeSinceStop;

                    if (isTimeline && mIsPlaying)
                    {
                        SetPlaying(false);
                        SetStopTime(stopTime);
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Stopping timeline %s", mName.GetS());
#endif
                    }
                    else if (isPaintLayer && lp->GetNumFrames() > 1 && lp->GetPlaying())
                    {
                        lp->SetPlaying(false);
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Stopping animclip %s", mName.GetS());
#endif
                    }
                    else if (isSoundLayer && ls->GetPlaying())
                    {
                        ls->SetPlaying(false);
                        mStopTime = stopTime;
#ifdef _DEBUG
                        log->Printf(LT_DEBUG, L"[LayerSetState] Stopping sound %s", mName.GetS());
#endif
                    }
                }
                // never interpolate visiblity
                mVisible = lastKey->mValue.mBool;
                break;
            }
            case AnimProperty::Opacity:
                // opacity means volume for sound layers
                if (lastKey->mInterpolation == InterpolationType::None || isLastKey)
                    mOpacity = lastKey->mValue.mFloat;
                else
                    mOpacity = lastKey->mValue.mFloat + (nextKey->mValue.mFloat - lastKey->mValue.mFloat)*float(t);
                break;

            case AnimProperty::Transform:
                if (lastKey->mInterpolation == InterpolationType::None || isLastKey)
                    mTransform = lastKey->mValue.mTransform;
                else
                {
                    mTransform = mix(lastKey->mValue.mTransform * mPivotTransform, nextKey->mValue.mTransform * mPivotTransform, t) * invert(mPivotTransform);
                }
                break;
           
            case AnimProperty::DrawInTime:
                if (lastKey->mInterpolation == InterpolationType::None || isLastKey)
                    mDrawInTime = lastKey->mValue.mDouble;
                else
                    mDrawInTime = lastKey->mValue.mDouble + (nextKey->mValue.mDouble - lastKey->mValue.mDouble)*t;
                break;
            case AnimProperty::Loop:
                if (isPaintLayer)
                    lp->SetMaxRepeatCount(lastKey->mValue.mBool ? 0 : 1);
                else
                    mMaxRepeatCount = (lastKey->mValue.mBool ? 0 : 1);
                break;
            case AnimProperty::Offset:
                mStartOffset = piTick(int64_t(lastKey->mValue.mInt));
                break;
            default:
                break;

            } // switch
        } // props loop


        // Handle layerpaint animations
        if (GetWorldVisible())
        {
            if (isPaintLayer && lp->GetPlaying())
            {
                lp->SetTime(timeSinceStart);
            }
            if (isSoundLayer && ls->GetPlaying())
            {
                ls->SetVolume(GetWorldOpacity());
            }
        }
    }

    void Layer::SetPivot(const trans3d& p)
    {
        mPivotTransform = p;
    }

    void Layer::SetLoaded(bool loaded)
    {
        mLoaded = loaded;
    }

    void Layer::SetDefaultPlaybackVisibility()
    {
        mVisible = false;
    }

    void Layer::ResetActionStatesAt(piTick time)
    {
        // reset action states
        const int numActionKeys = GetNumAnimKeys(Layer::AnimProperty::Action);
        if (numActionKeys > 0)
        {
            for (int i = 0; i < numActionKeys; i++)
            {
                Layer::AnimKey* key = (Layer::AnimKey*) GetAnimKey(Layer::AnimProperty::Action, i);
                key->mValue.mBool = key->mTime < time;
            }
        }
    }

	const uint32_t Layer::GetAssetID(void) const
	{
		return mAssetID;
	}


    void Layer::GetLocalTimeOffsetAndVisiblity(piTick rootTime, piTick* offset, bool* visible)
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
            {
                *visible = false;
                *offset = rootToLocal;
                return;
            }
        }

        // we're world visible, return the offset to root
        *visible = true;
        *offset = rootToLocal;
        return;
    }


}
