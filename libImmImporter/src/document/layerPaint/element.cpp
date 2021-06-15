#include <math.h>
#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libCompression/basic/piQuantize.h"
#include "element.h"
using namespace ImmCore;

namespace ImmImporter
{

Element::Element() {}

Element::~Element() {}

// static
const int Element::kSectionsLUT[] = { 2, 2, 7, 7, 4 };

void Element::Make( int num, BrushSectionType bid, VisibilityType mode)
{
	mBrush = bid;
	mVisibleMode = mode;

	mNumPoints = num;

	mBBox = bound3(1e20f);
}

float Element::GetWidth(int vertex, float biggestStroke) const
{
    return piQuantize::ibits15(mPoints[vertex].mWid) * (1.7f*biggestStroke);
}

void Element::Compute(float biggestStroke)
{
    const uint64_t nump = mNumPoints;
	if (nump < 1)
	{
		mBBox = bound3(1e20f);
		return;
	}

    Point* p = mPoints;

    const float wid0 = GetWidth(0, biggestStroke);
    mBBox = bound3(p[0].mPos - vec3(wid0),
		           p[0].mPos + vec3(wid0));
    for (uint64_t i = 1; i < nump; i++)
    {
        const float widi = GetWidth(static_cast<int>(i), biggestStroke);
        mBBox = include(mBBox, p[i].mPos - vec3(widi));
		mBBox = include(mBBox, p[i].mPos + vec3(widi));
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
		p[i].mTim = clamp01( 0.5f * (timeOffset + (float(i) / numpf)));

		// reduce unintentional dithering due to old UI problems with opacity control
		//if (p[i].mTra > 0.95f) p[i].mTra = 1.0f;
	}

	mLength = s;

}


//====================================================================================================================================================

vec3 Element::ComputeTangent(int i) const
{
	const int np = mNumPoints;


    // find first valid forward difference
    vec3 ta1 = vec3(0.0f);
    for (int j = i+1; j < np; j++)
    {
        const vec3 di = mPoints[j].mPos - mPoints[i].mPos;
        const float le = length(di);
        if (le>=0.0000001f)
        {
            ta1 = di/le;
            break;
        }
    }

    // find first valid backward difference
    vec3 ta2 = vec3(0.0f);
    for (int j = i-1; j >= 0; j--)
    {
        const vec3 di = mPoints[i].mPos - mPoints[j].mPos;
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

void Element::ComputeBasis(int i, vec3 * resTan, vec3 *resU, vec3 * resV) const
{
    vec3 nt = this->ComputeTangent(i);

    piAssert(!isInf(nt));
    piAssert(!isNan(nt));
    piAssert(!isZer(nt));
    piAssert(!isInf(mPoints[i].mNor));
    piAssert(!isNan(mPoints[i].mNor));
    piAssert(!isZer(mPoints[i].mNor));

    vec3 vu = cross(mPoints[i].mNor, nt);
    float lu = length(vu);
    if (lu >= 0.0000001f)
    {
        vu = vu / lu;
    }
    else if( fabsf(nt.x)<0.9f )
    {
        vu = vec3(0.0f, nt.z, nt.y);
    }
    else if (fabsf(nt.y)<0.9f)
    {
        vu = vec3(-nt.z, 0.0f, nt.x);
    }
    else //if (fabsf(nt.z)<0.9f)
    {
        vu = vec3(nt.y, -nt.x, 0.0f);
    }

    piAssert(!isInf(vu));
    piAssert(!isNan(vu));
    piAssert(!isZer(vu));

    vec3 vv = normalize(cross(nt, vu));

    piAssert(!isInf(vv));
    piAssert(!isNan(vv));
    piAssert(!isZer(vv));

    piAssert(fabsf(dot(nt, vu)) < 0.01f);
    piAssert(fabsf(dot(vu, vv)) < 0.01f);
    piAssert(fabsf(dot(vv, nt)) < 0.01f);

    *resTan = nt;
    *resU = vu;
    *resV = vv;
}


const int Element::GetNumPolygons() const
{
    static const uint32_t kSegments[] = {1, 1, 7, 7, 4};
    const uint32_t numSegments = kSegments[static_cast<uint32_t>(mBrush)];
    return numSegments * (mNumPoints - 1); // GetNumPolygons() * 2 for num triangles
}

}
