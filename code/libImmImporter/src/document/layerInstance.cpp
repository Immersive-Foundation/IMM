#include "layerInstance.h"
#include "libImmCore/src/libBasics/piStr.h"
using namespace ImmCore;

namespace ImmImporter
{

	LayerInstance::LayerInstance() {}

	LayerInstance::~LayerInstance() {}

	bool LayerInstance::Init(void)
	{
		mInstance = nullptr;
		return true;
	}

	void LayerInstance::Deinit(void) {}

	const piString& LayerInstance::GetLayer(void) const { return mLayer; }

	Layer* LayerInstance::GetInstance(void) const { return mInstance; }

}
