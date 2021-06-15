#pragma once

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piString.h"

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
