#pragma once

#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piString.h"

#include "sequence.h"

namespace ImmImporter
{

	class LayerInstance
	{
	public:
		LayerInstance();
		~LayerInstance();

		bool Init(void);
		void Deinit(void);

		const ImmCore::piString& GetLayer(void) const;

		Layer* GetInstance(void) const;

	private:
		ImmCore::piString mLayer;
		Layer* mInstance;
	};

}
