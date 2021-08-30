//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../../libBasics/piVecTypes.h"

namespace ImmCore
{
	namespace piPredictors
	{
		class Size_1_Order_1
		{
		public:
			Size_1_Order_1();
			int  encode(int qval);
			int  decode(int qval);
			void reset(int qval);
		private:
			int mNum;
			int prev;
		};

		class Size_2_Order_1
		{
		public:
			Size_2_Order_1();
			ivec2 encode(ivec2 qval);
			ivec2 decode(ivec2 qval);
			void  reset(ivec2 qval);
		private:
			int mNum;
			ivec2 prev;
		};

		class Size_3_Order_1
		{
		public:
			Size_3_Order_1();
			ivec3 encode(ivec3 qval);
			ivec3 decode(ivec3 qval);
			void  reset(ivec3 qval);
		private:
			int mNum;
			ivec3 prev;
		};

		class Size_3_Order_2
		{
		public:
			Size_3_Order_2();
			ivec3 encode(ivec3 qval);
			ivec3 decode(ivec3 qval);
			void  reset(ivec3 qval);
		private:
			uint64_t mNum;
			ivec3 prev;
			ivec3 prev2;
		};
	}
}
