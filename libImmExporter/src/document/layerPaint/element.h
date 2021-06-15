#pragma once

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piTArray.h"
#include "libCore/src/libBasics/piVecTypes.h"
#include <vector>
namespace ImmExporter
{

//  Totally uncompressed. The renderer does its own compression when copying to the GPU
typedef struct
{
    ImmCore::vec3 mPos;  // (stored) position
    ImmCore::vec3 mNor;  // (stored) geometry orientation
    ImmCore::vec3 mDir;  // (stored) view direction at drawing time
    ImmCore::vec3 mCol;  // (stored) color
    float mTra; // (stored) transparency
    float mWid; // (stored) width
    float mLen; // (computed) arclength
    float mTim; // (computed) time
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


public:
    Element();
    ~Element();

    /** Initialize element.
    * Allocate memory for points data
    * \param numPoints Number of elements. Has to be greater than 2.
    * \param brushType Brush section type. Can not be BrushSectionType::Point
    * \param mode Visibility type. */
	bool Init(uint32_t numPoints, BrushSectionType brushType, VisibilityType mode);
    void Destroy();

    Point* GetPoint(uint64_t pointIndex) const;

	inline const uint32_t GetNumPoints(void) const { return mNumPoints; }
	inline const ImmCore::bound3 GetBoundingBox(void) const { return mBBox; }
	inline const VisibilityType GetVisibleMode(void) const { return mVisibleMode; }
	inline const BrushSectionType GetBrushType(void) const { return mBrushType; }


    void ComputeBoundingBox();
    ImmCore::vec3 ComputeTangent(int pointIndex) const;
    void ComputeUV(int pointIndex, ImmCore::vec3 tan, ImmCore::vec3& u, ImmCore::vec3& v) const;
	const int GetNumPolygons() const;

private:

    // geometry
    Point* mPoints = nullptr;
    uint32_t mNumPoints = 0;
	VisibilityType mVisibleMode;
	BrushSectionType mBrushType;
    ImmCore::bound3 mBBox;
};

} // namespace ImmExporter
