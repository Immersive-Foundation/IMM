//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "../../libBasics/piTypes.h"
#include "piPredictors.h"

namespace ImmCore
{
	//  0 -> 0
	//  1 -> 1
	// -1 -> 2
	//  2 -> 3
	// -2 -> 4
	//  3 -> 5
	static inline uint32_t encodeSigned(int32_t n)
	{
		return (n > 0) ? (n << 1) - 1 : ((-n) << 1);
	}

	//  0 ->  0
	//  1 ->  1
	//  2 -> -1
	//  3 ->  2
	//  4 -> -2
	//  5 ->  3
	static inline int32_t decodeSigned(uint32_t n)
	{
		const int val = (n + 1) >> 1;
		return ((n & 1) == 1) ? val : -val;
	}

	namespace piPredictors
	{
		Size_1_Order_1::Size_1_Order_1()
		{
			mNum = 0;
			prev = 0;
		}

		void Size_1_Order_1::reset(int qval)
		{
			mNum = 1;
			prev = qval;
		}

		int Size_1_Order_1::encode(int qval)
		{
			mNum++;

			const int err = qval - prev;       // error
			prev = qval;                         // history

			if (mNum <= 1)
			{
				return qval;
			}
			else
			{
				const int res = encodeSigned(err);
				return res;
			}
		}

		int Size_1_Order_1::decode(int err)
		{
			mNum++;

			if (mNum > 1)
			{
				err = decodeSigned(err);
				const int qval = prev + err;         // add error to prediction to get correct result
				prev = qval;                         // history
				return qval;
			}
			else
			{
				prev = err;                          // history
				return err;
			}
		}
		//-------------------------------------------------

		Size_2_Order_1::Size_2_Order_1()
		{
			mNum = 0;
			prev = ivec2(0, 0);
		}

		void Size_2_Order_1::reset(ivec2 qval)
		{
			mNum = 1;
			prev = qval;
		}

		ivec2 Size_2_Order_1::encode(ivec2 qval)
		{
			mNum++;

			const ivec2 err = qval - prev;       // error
			prev = qval;                         // history

			if (mNum <= 1)
			{
				return qval;
			}
			else
			{
				return ivec2(encodeSigned(err.x), encodeSigned(err.y));
			}
		}

		ivec2 Size_2_Order_1::decode(ivec2 err)
		{
			mNum++;

			if (mNum > 1)
			{
				err.x = decodeSigned(err.x);
				err.y = decodeSigned(err.y);
				const ivec2 qval = prev + err;       // add error to prediction to get correct result
				prev = qval;                         // history
				return qval;
			}
			else
			{
				prev = err;                          // history
				return err;
			}
		}
		//-------------------------------------------------


		Size_3_Order_1::Size_3_Order_1()
		{
			mNum = 0;
			prev = ivec3(0, 0, 0);
		}

		void Size_3_Order_1::reset(ivec3 qval)
		{
			mNum = 1;
			prev = qval;
		}

		ivec3 Size_3_Order_1::encode(ivec3 qval)
		{
			mNum++;

			const ivec3 err = qval - prev;       // error
			prev = qval;                         // history

			if (mNum <= 1)
			{
				return qval;
			}
			else
			{
				return ivec3(encodeSigned(err.x), encodeSigned(err.y), encodeSigned(err.z));
			}
		}

		ivec3 Size_3_Order_1::decode(ivec3 err)
		{
			mNum++;

			if (mNum > 1)
			{
				err.x = decodeSigned(err.x);
				err.y = decodeSigned(err.y);
				err.z = decodeSigned(err.z);
				const ivec3 qval = prev + err;       // add error to prediction to get correct result
				prev = qval;                         // history
				return qval;
			}
			else
			{
				prev = err;                          // history
				return err;
			}
		}

		//=========================================================

		Size_3_Order_2::Size_3_Order_2()
		{
			mNum = 0;
			prev = ivec3(0, 0, 0);
			prev2 = ivec3(0, 0, 0);
		}

		ivec3 Size_3_Order_2::encode(ivec3 qval)
		{
			mNum++;

			if (mNum == 1)
			{
				prev2 = prev;                        // history
				prev = qval;                         // history
				return qval;
			}
			else if (mNum == 2)
			{
				const ivec3 pval = prev;             // predict
				const ivec3 err = qval - pval;       // error
				prev2 = prev;                        // history
				prev = qval;                         // history
				return ivec3(encodeSigned(err.x), encodeSigned(err.y), encodeSigned(err.z));
			}
			else
			{
				const ivec3 pval = 2 * prev - prev2; // predict
				const ivec3 err = qval - pval;       // error
				prev2 = prev;                        // history
				prev = qval;                         // history
				return ivec3(encodeSigned(err.x), encodeSigned(err.y), encodeSigned(err.z));
			}
		}

		void Size_3_Order_2::reset(ivec3 qval)
		{
			mNum = 1;
			prev2 = ivec3(0,0,0);
			prev = qval;
		}

		ivec3 Size_3_Order_2::decode(ivec3 err)
		{
			mNum++;

			if (mNum > 2)
			{
				err.x = decodeSigned(err.x);
				err.y = decodeSigned(err.y);
				err.z = decodeSigned(err.z);

				const ivec3 pval = 2 * prev - prev2; // predict
				const ivec3 qval = pval + err;       // add error to get correct result
				prev2 = prev;                        // history
				prev = qval;                         // history

				return qval;
			}
			else if( mNum>1 )
			{
				err.x = decodeSigned(err.x);
				err.y = decodeSigned(err.y);
				err.z = decodeSigned(err.z);

				const ivec3 pval = prev;             // predict
				const ivec3 qval = pval + err;       // add error to get correct result
				prev2 = prev;                        // history
				prev = qval;                         // history

				return qval;
			}
			else
			{
				prev2 = prev;                        // history
				prev = err;                          // history
				return err;
			}
		}
	}

}
