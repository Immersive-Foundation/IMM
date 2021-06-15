#include <new>
#include <malloc.h>
#include <math.h>
#include "libCore/src/libBasics/piVecTypes.h"
#include "element.h"
using namespace ImmCore;

namespace ImmExporter
{

Element::Element() {}

Element::~Element() {}


bool Element::Init(uint32_t numPoints, BrushSectionType brushType, VisibilityType mode)
{
    if (numPoints < 2 || brushType == BrushSectionType::Point)
        return false;
	mBrushType = brushType;
	mVisibleMode = mode;
    mNumPoints = numPoints;
	mBBox = bound3(1e20f);
    //if (!mPoints.Init(mNumPoints, sizeof(Point), true))
    //    return false;
    //mPoints.SetLength(mNumPoints);
    mPoints = new Point[mNumPoints];
    return true;
}

void Element::Destroy()
{
    delete mPoints;
}

Point* Element::GetPoint(uint64_t pointIndex) const
{
    if (pointIndex >= mNumPoints)
        return nullptr;
    return mPoints + pointIndex;
}

void Element::ComputeBoundingBox(void)
{
    if (mNumPoints < 1)
        return;
    const uint64_t nump = mNumPoints;
	if (nump < 1)
	{
		mBBox = bound3(1e20f);
		return;
	}

    Point* p = mPoints;


    mBBox = bound3(p[0].mPos - vec3(p[0].mWid),
		           p[0].mPos + vec3(p[0].mWid));
    for (uint64_t i = 1; i < nump; i++)
    {
        mBBox = include(mBBox, p[i].mPos - vec3(p[i].mWid));
		mBBox = include(mBBox, p[i].mPos + vec3(p[i].mWid));
	}


	float s = 0.0f;
	float numpf = float(nump);
	for (uint64_t i = 0; i < nump; i++)
	{
		p[i].mLen = s;
		if (i < (nump - 1))
			s += length(p[i].mPos - p[i + 1].mPos);

		//p[i].mTim = timeOffset + float(i) / 90.0f; // per curve time should come from binary file // IQIQ

		// HACK: make sure that the timing across a curve is normalized to be between 0 and 1.  timeOffset
		// is provided with the range [0., 1.]
		float timeOffset = 0.0f;
		p[i].mTim = fmin(1.f, fmax(0.f, .5f * (timeOffset + (float(i) / numpf))));

		// reduce unintentional dithering due to old UI problems with opacity control
		//if (p[i].mTra > 0.95f) p[i].mTra = 1.0f;
	}

	//mLength = s;

}


//====================================================================================================================================================


vec3 Element::ComputeTangent(int pointIndex) const
{
	const int np = mNumPoints;


    // find first valid forward difference
    vec3 ta1 = vec3(0.0f);
    for (int j = pointIndex +1; j < np; j++)
    {
        const vec3 di = mPoints[j].mPos - mPoints[pointIndex].mPos;
        const float le = length(di);
        if (le>=0.0000001f)
        {
            ta1 = di/le;
            break;
        }
    }

    // find first valid backward difference
    vec3 ta2 = vec3(0.0f);
    for (int j = pointIndex -1; j >= 0; j--)
    {
        const vec3 di = mPoints[pointIndex].mPos - mPoints[j].mPos;
        const float le = length(di);
        if (le >= 0.0000001f)
        {
            ta2 = di/le;
            break;
        }
    }

    // average
    const vec3 ta = ta1 + ta2;
    const float le = length(ta);
    if (le > 0.0000001f) return ta / le;

    // if that's still zero, go for a desperate solution - overal stroke direction + noise
    return normalize(mPoints[np-1].mPos- mPoints[0].mPos+vec3(0.000001f,0.000002f,0.000003f ));
}

void Element::ComputeUV(int pointIndex, vec3 tan, vec3& resU, vec3& resV) const
{
    piAssert(!isInf(tan));
    piAssert(!isNan(tan));
    piAssert(!isZer(tan));
    piAssert(!isInf(mPoints[pointIndex].mNor));
    piAssert(!isNan(mPoints[pointIndex].mNor));
    piAssert(!isZer(mPoints[pointIndex].mNor));

    vec3 vu = cross(mPoints[pointIndex].mNor, tan);
    float lu = length(vu);
    if (lu >= 0.0000001f)
    {
        vu = vu / lu;
    }
    else if( fabsf(tan.x)<0.9f )
    {
        vu = vec3(0.0f, tan.z, tan.y);
    }
    else if (fabsf(tan.y)<0.9f)
    {
        vu = vec3(-tan.z, 0.0f, tan.x);
    }
    else //if (fabsf(tan.z)<0.9f)
    {
        vu = vec3(tan.y, -tan.x, 0.0f);
    }

    piAssert(!isInf(vu));
    piAssert(!isNan(vu));
    piAssert(!isZer(vu));

    vec3 vv = normalize(cross(tan, vu));

    piAssert(!isInf(vv));
    piAssert(!isNan(vv));
    piAssert(!isZer(vv));

    piAssert(fabsf(dot(tan, vu)) < 0.01f);
    piAssert(fabsf(dot(vu, vv)) < 0.01f);
    piAssert(fabsf(dot(vv, tan)) < 0.01f);

    resU = vu;
    resV = vv;
}


const int Element::GetNumPolygons() const
{
    static const uint32_t kSegments[] = {1, 1, 7, 7, 4};
    const uint32_t numSegments = kSegments[static_cast<uint32_t>(mBrushType)];
    return numSegments * (mNumPoints - 1); // GetNumPolygons() * 2 for num triangles
}

}
