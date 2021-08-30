#pragma once

#include "libImmCore/src/libBasics/piVecTypes.h"

using namespace ImmCore;

namespace ImmImporter
{

typedef struct
{
    vec3  mPos;  // (stored) position
    vec3  mNor;  // (stored) geometry orientation
    vec3  mDir;  // (stored) view direction at drawing time
    vec3  mCol;  // (stored) color
    int   mTra;  // (stored) transparency                       8 bits
    int   mWid;  // (stored) width:                            15 bits:  float mWidFloat =  (1.7f*biggestStroke)*piQuantize::ibits15(mWid) = (1.7f*biggestStroke)*float(mWid)/32767.0f;
    float mLen;  // (computed) arclength
    float mTim;  // (computed) time

} Point;

class Element
{
public:
	enum class BrushSectionType : uint32_t
	{
		Point = 0,
		Segment = 1,
		Circle = 2,
		Ellipse = 3,
		Square = 4,
		Count = 5
	};

	enum class VisibilityType : uint32_t
	{
		FadePow2 = 0,
		Always = 1
		// FadePow1
		// FadePow4
		// FadePow8
	};

	static const int kSectionsLUT[];

public:
    Element();
    ~Element();

	void Make(int num, BrushSectionType bid, VisibilityType mode);


    void Compute(float biggestStroke);
    vec3 ComputeTangent(int i) const;
    void ComputeBasis(int i, vec3 * tan, vec3 *u, vec3 * v) const;


	inline const uint32_t GetNumPoints(void) const { return mNumPoints; }
	inline Point* GetPoints(void) { return mPoints; }
           float GetWidth(int vertex, float biggestStroke) const;
	inline const bound3   GetBBox(void) const { return mBBox; }
	inline const BrushSectionType GetBrush(void) const { return mBrush; }
	inline const VisibilityType GetVisibleMode(void) const { return mVisibleMode; }
	const int GetNumPolygons() const;

private:
    // geometry
	Point mPoints[8192];
	uint32_t mNumPoints;
	VisibilityType mVisibleMode;
	BrushSectionType mBrush;
    bound3 mBBox;
    float mLength;
};

}
