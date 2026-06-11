#include <cstring>

#include "scan_protocol.h"

using namespace CoreNs;

bool CoreNs::isAsyncControlPortName(const char *portName)
{
	if (!portName || portName[0] == '\0')
	{
		return false;
	}

	static const char *kAsyncPorts[] = {
		"reset",
		"rst",
		"nrst",
		"arst",
		"areset",
		"reset_n",
		"RESET",
		"RST",
		"NRST",
		"RN",
		"SN",
		nullptr,
	};

	for (int i = 0; kAsyncPorts[i] != nullptr; ++i)
	{
		if (!strcmp(portName, kAsyncPorts[i]))
		{
			return true;
		}
	}
	return false;
}

bool CoreNs::isFaultOnAsyncControlPi(const Circuit *pCircuit, const Fault *pFault)
{
	if (!pCircuit || !pFault || pFault->faultyLine_ != 0 || pFault->gateID_ < 0)
	{
		return false;
	}

	const Gate &gate = pCircuit->circuitGates_[pFault->gateID_];
	if (gate.gateType_ != Gate::PI)
	{
		return false;
	}

	const IntfNs::Port *pPort = pCircuit->pNetlist_->getTop()->getPort(gate.cellId_);
	return pPort && isAsyncControlPortName(pPort->name_);
}

int CoreNs::applyScanProtocol(Circuit *pCircuit, FaultListExtract *pFaultList)
{
	if (!pCircuit || !pFaultList)
	{
		return 0;
	}

	int marked = 0;
	for (Fault *pFault : pFaultList->faultsInCircuit_)
	{
		if (!isFaultOnAsyncControlPi(pCircuit, pFault))
		{
			continue;
		}

		pFault->faultState_ = Fault::TI;
		++marked;

		Gate &gate = pCircuit->circuitGates_[pFault->gateID_];
		gate.hasConstraint_ = true;
		gate.constraint_ = PARA_H;
	}

	return marked;
}

ScanProtocolStats CoreNs::computeScanProtocolStats(const Circuit *pCircuit,
                                                   const FaultPtrList &faults)
{
	ScanProtocolStats stats;
	size_t ud = 0;
	size_t pt = 0;
	size_t ab = 0;
	size_t to = 0;

	for (Fault *pFault : faults)
	{
		++stats.fuCollapsed;
		const int eq = pFault->equivalent_;
		stats.fuFull += eq;

		const bool scanExcluded = isFaultOnAsyncControlPi(pCircuit, pFault);

		switch (pFault->faultState_)
		{
			case Fault::UD:
				ud += eq;
				++stats.udCollapsed;
				break;
			case Fault::DT:
				stats.dtFull += eq;
				++stats.dtCollapsed;
				break;
			case Fault::PT:
				pt += eq;
				break;
			case Fault::AU:
				stats.auFull += eq;
				if (scanExcluded)
				{
					++stats.auResetCollapsed;
				}
				else
				{
					++stats.auCombCollapsed;
				}
				break;
			case Fault::TI:
				stats.tiFull += eq;
				if (scanExcluded)
				{
					stats.tiScanFull += eq;
					++stats.tiScanCollapsed;
				}
				break;
			case Fault::AB:
				ab += eq;
				break;
			case Fault::TO:
				to += eq;
				break;
			default:
				break;
		}
	}

	if (stats.fuFull > 0)
	{
		stats.fcRaw = 100.0 * (double)stats.dtFull / (double)stats.fuFull;
		const size_t scanDenom = stats.fuFull - stats.tiScanFull;
		if (scanDenom > 0)
		{
			stats.fcScan = 100.0 * (double)stats.dtFull / (double)scanDenom;
		}
	}

	const size_t rawTestDenom = ud + stats.dtFull + pt + ab + to;
	if (rawTestDenom > 0)
	{
		stats.testCovRaw = 100.0 * (double)stats.dtFull / (double)rawTestDenom;
	}

	const size_t scanTestDenom = rawTestDenom - stats.tiScanFull;
	if (scanTestDenom > 0)
	{
		stats.testCovScan = 100.0 * (double)stats.dtFull / (double)scanTestDenom;
	}

	const size_t scanCollapsedDenom = stats.fuCollapsed - stats.tiScanCollapsed;
	if (scanCollapsedDenom > 0)
	{
		stats.fcScanCollapsed = 100.0 * (double)stats.dtCollapsed / (double)scanCollapsedDenom;
	}

	return stats;
}
