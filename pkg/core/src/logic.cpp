// **************************************************************************
// File       [ logic.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include "logic.h"

using namespace CoreNs;

namespace
{
	void valueSets(const Value &value, bool &good0, bool &good1, bool &fault0, bool &fault1)
	{
		good0 = good1 = fault0 = fault1 = false;
		switch (value)
		{
			case L:
				good0 = fault0 = true;
				break;
			case H:
				good1 = fault1 = true;
				break;
			case X:
				good0 = good1 = fault0 = fault1 = true;
				break;
			case D:
				good1 = true;
				fault0 = true;
				break;
			case B:
				good0 = true;
				fault1 = true;
				break;
			case G0:
				good0 = true;
				fault0 = fault1 = true;
				break;
			case FO:
				good0 = good1 = true;
				fault0 = true;
				break;
			case F1:
				good0 = good1 = true;
				fault1 = true;
				break;
			case G1:
				good1 = true;
				fault0 = fault1 = true;
				break;
			default:
				good0 = good1 = fault0 = fault1 = true;
				break;
		}
	}

	int railAnd(int a, int b)
	{
		if (a == 0 && b == 0)
			return 0;
		if (a == 1 && b == 1)
			return 1;
		if ((a == 0 && b == 1) || (a == 1 && b == 0))
			return 0;
		if (a == 2 && b == 0)
			return 2;
		if (a == 0 && b == 2)
			return 2;
		if (a == 1 && b == 2)
			return 1;
		if (a == 2 && b == 1)
			return 2;
		return 2;
	}

	Value pairToValue(int goodRail, int faultRail)
	{
		if (goodRail == 0 && faultRail == 0)
			return L;
		if (goodRail == 0 && faultRail == 2)
			return G0;
		if (goodRail == 0 && faultRail == 1)
			return B;
		if (goodRail == 2 && faultRail == 0)
			return FO;
		if (goodRail == 2 && faultRail == 2)
			return X;
		if (goodRail == 2 && faultRail == 1)
			return F1;
		if (goodRail == 1 && faultRail == 0)
			return D;
		if (goodRail == 1 && faultRail == 2)
			return G1;
		if (goodRail == 1 && faultRail == 1)
			return H;
		return I;
	}
}

bool CoreNs::atpgValuesConsistent(const Value &a, const Value &b)
{
	bool ag0, ag1, af0, af1, bg0, bg1, bf0, bf1;
	valueSets(a, ag0, ag1, af0, af1);
	valueSets(b, bg0, bg1, bf0, bf1);
	const bool goodOk = (ag0 && bg0) || (ag1 && bg1);
	const bool faultOk = (af0 && bf0) || (af1 && bf1);
	return goodOk && faultOk;
}

Value CoreNs::atpgIntersect(const Value &a, const Value &b)
{
	if (!isNineValuedLogic(a) || !isNineValuedLogic(b))
	{
		return I;
	}
	bool ag0, ag1, af0, af1, bg0, bg1, bf0, bf1;
	valueSets(a, ag0, ag1, af0, af1);
	valueSets(b, bg0, bg1, bf0, bf1);
	int goodRail = -1;
	int faultRail = -1;
	if (ag0 && bg0 && !(ag1 && bg1))
		goodRail = 0;
	else if (ag1 && bg1 && !(ag0 && bg0))
		goodRail = 1;
	else
		goodRail = 2;
	if (af0 && bf0 && !(af1 && bf1))
		faultRail = 0;
	else if (af1 && bf1 && !(af0 && bf0))
		faultRail = 1;
	else
		faultRail = 2;
	return pairToValue(goodRail, faultRail);
}

Value CoreNs::atpgToPatternValue(const Value &value)
{
	switch (value)
	{
		case L:
		case G0:
		case B:
		case FO:
			return L;
		case H:
		case G1:
		case D:
		case F1:
			return H;
		case X:
		default:
			return X;
	}
}

Value CoreNs::activateStuckAt(const Value &value, const int &faultType)
{
	if (isSensitiveValue(value))
	{
		return value;
	}
	if ((faultType == 1 || faultType == 3) && atpgGoodEquals(value, L))
	{
		return B;
	}
	if ((faultType == 0 || faultType == 2) && atpgGoodEquals(value, H))
	{
		return D;
	}
	return value;
}

Value CoreNs::reconcileAtpgValue(const Value &stored, const Value &evaluated)
{
	if (stored == evaluated)
	{
		return stored;
	}
	if (stored == X)
	{
		return evaluated;
	}
	if (evaluated == X)
	{
		return stored;
	}
	if (!atpgValuesConsistent(stored, evaluated))
	{
		return I;
	}
	return atpgIntersect(stored, evaluated);
}

void CoreNs::printValue(const Value &value, std::ostream &out)
{
	switch (value)
	{
		case L:
			out << "0";
			break;
		case H:
			out << "1";
			break;
		case X:
			out << "X";
			break;
		case D:
			out << "D";
			break;
		case B:
			out << "B";
			break;
		case G0:
			out << "G0";
			break;
		case FO:
			out << "FO";
			break;
		case F1:
			out << "F1";
			break;
		case G1:
			out << "G1";
			break;
		case Z:
			out << "Z";
			break;
		default:
			out << "I";
			break;
	}
	out << std::flush;
}

void CoreNs::printParallelValue(const ParallelValue &parallelValue, std::ostream &out)
{
	for (int bit = WORD_SIZE - 1; bit >= 0; --bit)
	{
		ParallelValue mask = 0x01;
		mask <<= bit;
		if ((parallelValue & mask) != PARA_L)
		{
			out << "1";
		}
		else
		{
			out << "0";
		}
		out << std::flush;
	}
}

void CoreNs::printSimulationValue(const ParallelValue &low, const ParallelValue &high,
												std::ostream &out)
{
	for (int bit = WORD_SIZE - 1; bit >= 0; --bit)
	{
		ParallelValue mask = 0x01;
		mask <<= bit;
		if ((low & mask) != PARA_L)
		{
			out << "0";
		}
		else if ((high & mask) != PARA_L)
		{
			out << "1";
		}
		else
		{
			out << "X";
		}
		out << std::flush;
	}
}
