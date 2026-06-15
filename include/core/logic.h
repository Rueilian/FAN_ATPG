// **************************************************************************
// File       [ logic.h ]
// Author     [ littleshamoo ]
// Synopsis   [ Logic representation and operation ]
// Date       [ 2010/12/14 created ]
// **************************************************************************

#ifndef _CORE_LOGIC_H_
#define _CORE_LOGIC_H_

#include <iostream>

namespace CoreNs
{

	// type defines
	typedef unsigned char Value;				 // typedef uint8_t   Value;
	typedef unsigned long ParallelValue; // typedef uintptr_t ParallelValue;

	// Nine-valued ATPG logic (Muth, IEEE TC 1976) ¡X five D-algorithm values plus
	// partly-specified G/F rails: G0, FO, F1, G1.
	// Each value ni = (bg, bf): good-circuit / faulty-circuit binary (0, 1, or x).
	constexpr Value L = 0;	 // 0   (0,0)
	constexpr Value H = 1;	 // 1   (1,1)
	constexpr Value X = 2;	 // U   (x,x) unknown
	constexpr Value D = 3;	 // S1  (1,0) good 1 / faulty 0
	constexpr Value B = 4;	 // S0  (0,1) good 0 / faulty 1
	constexpr Value Z = 5;	 // High-impedance (not part of nine-valued ATPG set)
	constexpr Value G0 = 6;	 // G0  (0,x) good 0 / faulty unspecified
	constexpr Value FO = 7;	 // FO  (x,0) good unspecified / faulty 0
	constexpr Value F1 = 8;	 // F1  (x,1) good unspecified / faulty 1
	constexpr Value G1 = 9;	 // G1  (1,x) good 1 / faulty unspecified
	constexpr Value I = 255; // Invalid

	// constant multi-bit logic
	constexpr ParallelValue PARA_L = 0;				// all bits are zero
	constexpr ParallelValue PARA_H = ~PARA_L; // all bits are one

	// determine word size
	constexpr int BYTE_SIZE = 8;
	constexpr int WORD_SIZE = sizeof(ParallelValue) * BYTE_SIZE;
	inline void setBitValue(ParallelValue &parallelValue, const size_t &bit, const Value &value)
	{
		parallelValue = value == L ? parallelValue & ~((ParallelValue)0x01 << bit) : parallelValue | ((ParallelValue)0x01 << bit);
	}

	inline Value getBitValue(const ParallelValue &parallelValue, const size_t &bit)
	{
		return (parallelValue & ((ParallelValue)0x01 << bit)) == PARA_L ? L : H;
	}

	// Map nine-valued ATPG constants to 0..8 table index; -1 for Z/I/other.
	inline int nineValIndex(const Value &value)
	{
		switch (value)
		{
			case L:
				return 0;
			case G0:
				return 1;
			case B:
				return 2;
			case FO:
				return 3;
			case X:
				return 4;
			case F1:
				return 5;
			case D:
				return 6;
			case G1:
				return 7;
			case H:
				return 8;
			default:
				return -1;
		}
	}

	inline bool isNineValuedLogic(const Value &value)
	{
		return nineValIndex(value) >= 0;
	}

	inline bool isFullyUnspecified(const Value &value)
	{
		return value == X;
	}

	// S1/S0 ¡X fully specified sensitive values (D-algorithm D/D').
	inline bool isSensitiveValue(const Value &value)
	{
		return value == D || value == B;
	}

	// Values whose good and faulty rails may differ (includes partly-specified F/G).
	inline bool hasFaultEffect(const Value &value)
	{
		return value == D || value == B || value == F1 || value == FO;
	}

	inline bool atpgGoodIsLow(const Value &value)
	{
		return value == L || value == G0;
	}

	inline bool atpgGoodIsHigh(const Value &value)
	{
		return value == H || value == G1;
	}

	// Collapse nine-valued good rail to L/H/X for control/data decisions.
	inline Value atpgGoodRepresentative(const Value &value)
	{
		if (atpgGoodIsLow(value) || value == B || value == FO)
		{
			return L;
		}
		if (atpgGoodIsHigh(value) || value == D || value == F1)
		{
			return H;
		}
		return X;
	}

	inline bool atpgGoodEquals(const Value &value, const Value &binaryValue)
	{
		if (binaryValue == L)
		{
			return value == L || value == G0 || value == B;
		}
		if (binaryValue == H)
		{
			return value == H || value == G1 || value == D;
		}
		return false;
	}

	bool atpgValuesConsistent(const Value &a, const Value &b);
	Value atpgIntersect(const Value &a, const Value &b);
	Value atpgToPatternValue(const Value &value);
	Value activateStuckAt(const Value &value, const int &faultType);
	Value reconcileAtpgValue(const Value &stored, const Value &evaluated);

	void printValue(const Value &value, std::ostream &out = std::cout);
	void printParallelValue(const ParallelValue &parallelValue, std::ostream &out = std::cout);
	void printSimulationValue(const ParallelValue &low, const ParallelValue &high, std::ostream &out = std::cout);

};

#endif
