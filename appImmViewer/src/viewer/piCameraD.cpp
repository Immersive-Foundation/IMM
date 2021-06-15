#include <math.h>
#include "libCore/src/libBasics/piVecTypes.h"
#include "piCameraD.h"

namespace ImmCore {

piCameraD::piCameraD()
{
	mWorldToCamera = mat4x4d::identity();
	mCameraToWorld = mat4x4d::identity();
}

piCameraD::~piCameraD()
{
}

void piCameraD::Set(const vec3d & pos, const vec3d & dir, const double roll)
{
	mWorldToCamera = setLookat( pos, pos + dir, vec3d(std::sin(roll), std::cos(roll), std::sin(roll) ));
	mCameraToWorld = invert(mWorldToCamera);
}

void piCameraD::SetPos(const vec3d & pos)
{
	mWorldToCamera[ 3] = -(mWorldToCamera[0] * pos[0] + mWorldToCamera[1] * pos[1] + mWorldToCamera[ 2] * pos[2]);
	mWorldToCamera[ 7] = -(mWorldToCamera[4] * pos[0] + mWorldToCamera[5] * pos[1] + mWorldToCamera[ 6] * pos[2]);
	mWorldToCamera[11] = -(mWorldToCamera[8] * pos[0] + mWorldToCamera[9] * pos[1] + mWorldToCamera[10] * pos[2]);
	mCameraToWorld = invert(mWorldToCamera);
}

vec3d piCameraD::GetPos( void ) const
{
    return (mCameraToWorld * vec4d(0.0, 0.0, 0.0, 1.0)).xyz();
}

vec3d piCameraD::GetDir(void) const
{
    return normalize((mCameraToWorld * vec4d(0.0, 0.0, -1.0, 0.0)).xyz());
}

const mat4x4d & piCameraD::GetWorldToCamera(void) const
{
    return mWorldToCamera;
}

const mat4x4d & piCameraD::GetCameraToWorld(void) const
{
    return mCameraToWorld;
}

void piCameraD::SetWorldToCamera(const mat4x4d & mat)
{
	mWorldToCamera = mat;
	mCameraToWorld = invert(mWorldToCamera);
}

void piCameraD::LocalMove( const vec3d & pos )
{
	mWorldToCamera[ 3] -= pos.x;
	mWorldToCamera[ 7] -= pos.y;
	mWorldToCamera[11] -= pos.z;
	mCameraToWorld = invert(mWorldToCamera);
}

void piCameraD::GlobalMove(const vec3d & pos)
{
	mWorldToCamera[ 3] -= (mWorldToCamera[0] * pos[0] + mWorldToCamera[1] * pos[1] + mWorldToCamera[ 2] * pos[2]);
	mWorldToCamera[ 7] -= (mWorldToCamera[4] * pos[0] + mWorldToCamera[5] * pos[1] + mWorldToCamera[ 6] * pos[2]);
	mWorldToCamera[11] -= (mWorldToCamera[8] * pos[0] + mWorldToCamera[9] * pos[1] + mWorldToCamera[10] * pos[2]);
	mCameraToWorld = invert(mWorldToCamera);
}

void piCameraD::RotateXY(const double x, const double y )
{
	mWorldToCamera = mat4x4d::rotateY(x) * mWorldToCamera;
	mCameraToWorld = invert(mWorldToCamera);
}

}
