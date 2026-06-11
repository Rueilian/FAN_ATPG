// Scan-protocol helpers: async reset/control pins held inactive during
// standard full-scan stuck-at ATPG (Cummings SNUG 2002; DFT scan_mode practice).

#ifndef _CORE_SCAN_PROTOCOL_H_
#define _CORE_SCAN_PROTOCOL_H_

#include "circuit.h"
#include "fault.h"

namespace CoreNs
{

bool isAsyncControlPortName(const char *portName);

bool isFaultOnAsyncControlPi(const Circuit *pCircuit, const Fault *pFault);

// Mark async-control PI stuck-at faults as TI and deassert the pin for ATPG.
// Returns the number of collapsed faults reclassified.
int applyScanProtocol(Circuit *pCircuit, FaultListExtract *pFaultList);

struct ScanProtocolStats
{
	size_t fuFull = 0;
	size_t fuCollapsed = 0;
	size_t dtFull = 0;
	size_t auFull = 0;
	size_t tiFull = 0;
	size_t tiScanFull = 0;
	size_t tiScanCollapsed = 0;
	size_t auCombCollapsed = 0;
	size_t auResetCollapsed = 0;
	size_t udCollapsed = 0;
	size_t dtCollapsed = 0;
	double fcRaw = 0.0;
	double fcScan = 0.0;
	double fcScanCollapsed = 0.0;
	double testCovRaw = 0.0;
	double testCovScan = 0.0;
};

ScanProtocolStats computeScanProtocolStats(const Circuit *pCircuit,
                                           const FaultPtrList &faults);

} // namespace CoreNs

#endif
