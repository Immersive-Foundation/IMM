#pragma once

#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piTick.h"
#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libBasics/piLog.h"

#include "layerPaint/element.h"
#include "layerPaint/drawing.h"

#include "layerPaint.h"

#include "layerPaint/drawingStatic.h"

#include <vector>

namespace ImmImporter
{

	class LayerPaintStatic : public LayerPaint
	{
	public:
		LayerPaintStatic() = default;
		~LayerPaintStatic() = default;

		bool Init(int numDrawings, int numFrames, int maxRepeatCount, uint32_t frameRate, unsigned int version);
		void Deinit(void);

		// rendering controlls. Return proper data based on mCurrentFrame
		const ImmCore::bound3& GetBBox(void) const;
		const bool HasBBox(void) const;
		const Drawing * GetCurrentDrawing(void) const;
        bool GetPlaying(void) const;

		// playback controlls
		void SetTime(ImmCore::piTick time); // sets current time during playback
		void SetPlaying(bool playing);
        void SetOffset(uint32_t offsetFrames);
        void SetMaxRepeatCount(uint32_t count);


		// for deserializing... (ugly!)
		unsigned int GetNumDrawings(void) const;
		unsigned int GetNumFrames(void) const;
		uint32_t *   GetFrameBuffer(void);
        unsigned int GetVersion(void) const;
        unsigned int GetMaxRepeatCount(void) const;
        unsigned int GetFrameRate(void) const;

        // for deserializing... (ugly!)
        Drawing * NewDrawing(void);
		// for gpu unloading... (ugly!)
		Drawing * GetDrawing(int drawing) const;


	private:
		// static data
		std::vector<DrawingStatic> mDrawings;
		ImmCore::piArray mFrames;
		uint32_t mFrameRate;	 // in FPS
		uint32_t mMaxRepeatCount; // 0 repeats forever
        uint32_t mVersion;

		// playback state
		bool mIsPlaying;		
        ImmCore::piTick mTime;	
        uint32_t mOffset;        // start offset in frames
		uint32_t mCurrentFrame; // derived from mCurrentTime, and always in synch

	};

}
