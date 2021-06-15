#pragma once

#include <functional>

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piTArray.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piTick.h"

#include "behaviors/keepalive.h"

namespace ImmExporter
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

		typedef struct
		{
			bool mBool = false;
			uint32_t mInt = 0;
			float mFloat = 0.0f;
			double mDouble =0.0;
			ImmCore::trans3d mTransform = ImmCore::trans3d::identity();
		} AnimValue;

		typedef struct
		{
            ImmCore::piTick mTime;
			AnimValue mValue;
			InterpolationType mInterpolation;
		} AnimKey;

		bool Init(Type type, const wchar_t* name, bool visible, const ImmCore::trans3d & transform, const ImmCore::trans3d& pivot,
            float opacity, bool isTimeLine, ImmCore::piTick duration, uint32_t maxRepeatCount);
		void Deinit();

		const uint32_t GetID(void) const;
		Layer* GetParent() const;
        Layer* GetChild(uint32_t id);  // if group
		const Type GetType(void) const;
		const ImmCore::piString& GetName(void) const;
		const ImmCore::piString* GetFullName(void) const;

		void SetVisible(bool visible);
		const bool GetVisible(void) const;
        const bool GetWorldVisible(void) const; // recurses parents
        bool GetLocalTimeOffsetAndVisiblity(ImmCore::piTick rootTime);

		LayerImplementation GetImplementation(void) const; // if not Group
		void SetImplementation(LayerImplementation implementation);

        void SetOpacity(float opacity);
		const float GetOpacity(void) const;
        const float GetWorldOpacity(void) const;

		void SetTransform(const ImmCore::trans3d & t);
        const ImmCore::trans3d GetTransform(void) const;
        const ImmCore::trans3d GetTransformToWorld(void) const;
        void SetPivot(const ImmCore::trans3d& p);
        const ImmCore::trans3d GetPivot() const;

		const bool HasBBox(uint32_t frameID) const;
		const ImmCore::bound3d GetBBox(uint32_t frameID);

        // Animation
        void SetIsTimeline(bool timeline);
        const bool GetIsTimeline(void) const;
        Layer* GetTimeline();
        void SetDuration(ImmCore::piTick duration);
		const ImmCore::piTick GetDuration() const;
		const uint32_t GetNumChildren(void) const;
		const uint32_t GetNumAnimKeys(AnimProperty property) const;
        const AnimKey* GetAnimKeyAt(AnimProperty property, ImmCore::piTick time) const;
		const AnimKey* GetAnimKey(AnimProperty property, unsigned int index) const;
        void SetMaxRepeatCount(uint32_t count);
        uint32_t GetMaxRepeatCount(void) const;

        void SetLoaded(bool loaded);
        const bool GetLoaded() const;

        // Edit
        bool AddKey(ImmCore::piTick time, AnimProperty property, const AnimValue& value, InterpolationType interpolation);
		typedef std::function<bool(Layer* layer, int level, int child, bool instance)> IMMVisitorF;
		bool Recurse(IMMVisitorF v, int level, unsigned int child, bool instance, bool doNotRecurseCollapsedGroups, bool doNotRecurseHiddenGroups, bool doNotRecurseLockedGroups, bool doPostExec);

        // Effects
		KeepAlive * GetKeepAlive(void);
        void SetDrawInTime(double v);
        const double GetDrawInTime(void) const;
        const double GetAnimParam(int id) const;
        const bool GetLayerUsesDrawin(void) const;
        void SetPotentiallyVisible(bool visible);
        const bool GetPotentiallyVisible(void) const;

	private:
        bool iCalcFullName(void);

	private:
        // Serialization data

		uint32_t mID;
		ImmCore::trans3d mTransform;
        ImmCore::trans3d mPivotTransform;
		float mOpacity;
		ImmCore::piString mName;
		bool mIsTimeline;
		ImmCore::piTick mDuration;
		uint32_t mMaxRepeatCount;
		Type mType;
		ImmCore::piTArray<AnimKey> mAnimKeys[static_cast<int>(AnimProperty::MAX)];
        //=============================================================================//

        // Internal Code
        // Scenegraph data
		Layer* mParent;                      // can only be of type group
		bool mVisible;
		ImmCore::piTArray<Layer*> mChildren;          // if group
		Sequence* mSequence; // TODO: delete!
		LayerImplementation mImplementation; // if not group
		ImmCore::piString mFullName;

        // Effect
		double mAnimParams[4];
        double mDrawInTime;
		KeepAlive mKeepAlive;           // subtle/minimal procedural effects on top of animated or static content
        // spatial culling
		bool mPotentiallyVisible;

	};
}
