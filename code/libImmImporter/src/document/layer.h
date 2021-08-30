#pragma once

#include <functional>

#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libBasics/piTArray.h"
#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piTick.h"

#include "behaviors/keepalive.h"

namespace ImmImporter
{
    enum class AssetFormat : uint32_t
    {
        NONE = 0,
        PNG = 1,
        JPG = 2,
        WAV = 3,
        OGG = 4,
        OPUS = 5
    };


	typedef void* LayerImplementation;

	class Sequence;

	class Layer
	{
	public:
		Layer(Sequence* sq, Layer* parent, uint32_t id);
		~Layer();

		enum class Type  : uint32_t
		{
			Group,     // a group
			Paint,     // a paint layer containing brush strokes
			Effect,    // an effect layer (a blur, a color correct, etc). This is defined in a volume
			Model,     // a 3d model imported form FBX, OBJ, etc
			Picture,   // a flat picture
			Sound,     // 3D (or plain) sound source
			Reference, // a reference to an external file
			Instance ,  // an instance to an internal layer
			SpawnArea , // a view point (location + locomotion volume and other viewing properties)
			MAX
		} ;


		enum class InterpolationType : uint32_t
		{
			None,
			Linear,
			Smoothstep,
            EaseIn,
            EaseOut,
            Spline,
            Auto,
            MAX
		};


		enum class AnimAction : uint32_t
		{
			Stop,
			Play,
            Loop,
            MakeDefault,
			MAX
		};


		enum class AnimProperty : uint32_t
		{
			Visibility,				// bool
			Opacity,				// float
			Position,
			Rotation,
			Scale,
			DrawInTime,				// double
			Action,					// uint
            Loop,                   // bool
            Offset,                 // uint
            Transform,
			MAX
		};

		struct AnimValue
		{
			bool mBool;
			uint32_t mInt;
			float mFloat;
			double mDouble;
			ImmCore::trans3d mTransform;
			void init(void) { mBool=false; mInt=0; mFloat=0.0f; mDouble=0.0; mTransform = ImmCore::trans3d::identity(); }
		} ;

		typedef struct
		{
            ImmCore::piTick mTime;
			AnimValue mValue;
			InterpolationType mInterpolation;
		} AnimKey;



		bool Init(Type type, const wchar_t* name, bool visible, const ImmCore::trans3d & transform, const ImmCore::trans3d& pivot, float opacity, bool isTimeLine, ImmCore::piTick duration, uint32_t maxRepeatCount, uint32_t assetID, ImmCore::piLog *log);
		void Deinit(ImmCore::piLog *log);

		uint32_t GetID(void) const;

		LayerImplementation GetImplementation(void) const; // if not Group
		void SetImplementation(LayerImplementation implementation);

		const   Type GetType(void) const;
		const   ImmCore::piString& GetName(void) const;
		const   ImmCore::piString* GetFullName(void) const;
		bool    GetVisible(void) const;
		bool    GetPotentiallyVisible(void) const;
        bool    GetWorldVisible(void) const; // recurses parents
		float   GetOpacity(void) const;
        float   GetWorldOpacity(void) const;

		ImmCore::trans3d GetTransform(void) const;
		bool    HasBBox(void) const;
		const ImmCore::bound3d GetBBox(void);
		bool* GetVisibleRef(void);
		float* GetOpacityRef(void);
		double GetDrawInTime(void) const;
		double GetAnimParam(int id) const;
		bool GetLayerUsesDrawin(void) const;
		bool GetPlaying(void) const;
		bool GetIsTimeline(void) const;
		Layer* GetTimeline();
		Layer* GetParent() const;                          // a group!
		ImmCore::trans3d GetTransformToWorld(void) const;
		KeepAlive * GetKeepAlive(void);
		uint32_t GetMaxRepeatCount(void) const;
		ImmCore::piTick GetStopTime() const;
		ImmCore::piTick GetDuration() const;
		ImmCore::piTick GetStartTime() const;
		uint32_t GetNumChildren(void) const;                    // if group
		Layer* GetChild(uint32_t id);                           // if group
		uint32_t GetNumAnimKeys(AnimProperty property) const;
		const AnimKey* GetAnimKey(AnimProperty property, unsigned int index) const;
        bool GetLoaded(void) const;
		const uint32_t GetAssetID(void) const;

		// hm, do we really need these?
		void SetOpacity(float opacity);
		void SetVisible(bool visible);
		void SetPotentiallyVisible(bool visible);
		void SetStartTime(ImmCore::piTick time);
		void SetStopTime(ImmCore::piTick time);
		void SetDrawInTime(double v);
		void SetTransform(const ImmCore::trans3d & t);
		void SetPlaying(bool playing);
		void    SetPosition(const ImmCore::vec3d& pos);
		void    SetRotation(const ImmCore::quatd& rot);
		void    SetScale(double scale);
        void SetMaxRepeatCount(uint32_t count);
        void SetIsTimeline(bool timeline);
        void SetDuration(ImmCore::piTick duration);
        void SetPivot(const ImmCore::trans3d& p);
        void SetLoaded(bool loaded);

        void GetLocalTimeOffsetAndVisiblity(ImmCore::piTick rootTime, ImmCore::piTick * offset, bool * visible);

        void SetStateAt(ImmCore::piTick time, ImmCore::piLog* log);

        // sets default pass through visiblity of groups without animation keys, hides all layers that have keys
        void SetDefaultPlaybackVisibility();

        // sets all actions before time as fired, and after as not fired
        void ResetActionStatesAt(ImmCore::piTick time);

        bool AddKey(ImmCore::piTick time, AnimProperty property, const AnimValue& value, InterpolationType interpolation);
        const AnimKey* GetAnimKeyAt(AnimProperty property, ImmCore::piTick time) const;

		typedef std::function<bool(Layer* layer, int level, int child, bool instance)> VisitorF;

		bool Recurse(VisitorF v, int level, unsigned int child, bool instance, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec);

	private:
		bool iCalcFullName(void);


	private:
		uint32_t mID;
		Type mType;
		bool mVisible;
        bool mLoaded;
		bool mPotentiallyVisible;
		ImmCore::piString mName;
		ImmCore::piString mFullName;
		float mOpacity;
		ImmCore::trans3d mTransform;
        ImmCore::trans3d mPivotTransform;

		double mDrawInTime;
		double mAnimParams[4];
        ImmCore::piTick mLastActiveSpanStart; // keep track of gapless retriggering
        ImmCore::piTick mStartOffset;
		ImmCore::piTick mDuration;
		ImmCore::piTick mStartTime;
		ImmCore::piTick mStopTime;
		bool mIsPlaying;
		bool mIsTimeline;
		uint32_t mMaxRepeatCount;
		ImmCore::piTArray<AnimKey> mAnimKeys[static_cast<int>(AnimProperty::MAX)];

		LayerImplementation mImplementation; // if not group
		ImmCore::piTArray<Layer*> mChildren;          // if group
		Layer* mParent;                      // can only be of type group
		Sequence* mSequence; // TODO: delete!

		KeepAlive     mKeepAlive;           // subtle/minimal procedural effects on top of animated or static content

		// for streaming
		uint32_t mAssetID;

	};
}
