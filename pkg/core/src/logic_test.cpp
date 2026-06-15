#include "logic.h"

#include <cassert>

using namespace CoreNs;

int main()
{
	assert(isNineValuedLogic(L));
	assert(isNineValuedLogic(G0));
	assert(isNineValuedLogic(F1));
	assert(!isNineValuedLogic(Z));

	assert(isFullyUnspecified(X));
	assert(isDecisionNeeded(X));
	assert(!isDecisionNeeded(G0));
	assert(isPartlySpecifiedAtpgValue(G0));
	assert(isPartlySpecifiedAtpgValue(FO));
	assert(isPartlySpecifiedAtpgValue(F1));
	assert(isPartlySpecifiedAtpgValue(G1));
	assert(!isPartlySpecifiedAtpgValue(D));

	assert(atpgGoodEquals(G0, L));
	assert(atpgGoodEquals(B, L));
	assert(atpgGoodEquals(G1, H));
	assert(atpgGoodEquals(D, H));
	assert(!atpgGoodEquals(FO, H));
	assert(!atpgGoodEquals(F1, L));

	assert(atpgValuesConsistent(G0, B));
	assert(atpgValuesConsistent(G1, D));
	assert(!atpgValuesConsistent(G0, D));
	assert(!atpgValuesConsistent(F1, FO));

	assert(atpgIntersect(X, D) == D);
	assert(atpgIntersect(G0, B) == B);
	assert(atpgIntersect(G1, D) == D);
	assert(atpgIntersect(FO, L) == L);
	assert(atpgIntersect(F1, H) == H);
	assert(reconcileAtpgValue(G0, D) == I);
	assert(reconcileAtpgValue(F1, FO) == I);

	assert(atpgToPatternValue(G0) == L);
	assert(atpgToPatternValue(FO) == L);
	assert(atpgToPatternValue(F1) == H);
	assert(atpgToPatternValue(G1) == H);

	return 0;
}
