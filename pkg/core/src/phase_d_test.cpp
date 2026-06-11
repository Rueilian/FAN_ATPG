// Phase D regression: relaxed PODEM init + atomic MUX2 modeling.
// Run from FAN_ATPG/:  ./pkg/core/bin/opt/phase_d_test .

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "interface/netlist_builder.h"
#include "interface/techlib_builder.h"
#include "circuit.h"
#include "fault.h"
#include "pattern.h"
#include "simulator.h"
#include "atpg.h"
#include "scan_protocol.h"

using namespace IntfNs;
using namespace CoreNs;

static int failures = 0;

static void expect(bool cond, const char *msg)
{
	if (!cond)
	{
		fprintf(stderr, "FAIL: %s\n", msg);
		++failures;
	}
}

static bool loadLib(Techlib *&lib, const char *mdtPath)
{
	lib = new Techlib;
	MdtFile *blder = new MdtLibBuilder(lib);
	if (!blder->read(mdtPath, false) || !lib->check(false))
	{
		delete blder;
		delete lib;
		lib = nullptr;
		return false;
	}
	delete blder;
	return true;
}

static bool loadNetlist(Netlist *&nl, Techlib *lib, const char *vPath)
{
	nl = new Netlist;
	nl->setTechlib(lib);
	VlogFile *blder = new VlogNlBuilder(nl);
	if (!blder->read(vPath, false) || !nl->check(false))
	{
		delete blder;
		delete nl;
		nl = nullptr;
		return false;
	}
	delete blder;
	return true;
}

static int countMuxGates(const Circuit &cir)
{
	int n = 0;
	for (int i = 0; i < cir.numGate_; ++i)
	{
		if (cir.circuitGates_[i].gateType_ == Gate::MUX)
		{
			++n;
		}
	}
	return n;
}

static int countMux2X1Cells(const Netlist &nl)
{
	int n = 0;
	Cell *top = nl.getTop();
	for (int i = 0; i < (int)top->getNCell(); ++i)
	{
		const Cell *cell = top->getCell(i);
		if (cell && cell->libc_ && !strcmp(cell->libc_->name_, "MUX2_X1"))
		{
			++n;
		}
	}
	return n;
}

static int countOaiAoiCompoundCells(const Netlist &nl)
{
	int n = 0;
	Cell *top = nl.getTop();
	for (int i = 0; i < (int)top->getNCell(); ++i)
	{
		const Cell *cell = top->getCell(i);
		if (!cell || !cell->libc_)
		{
			continue;
		}
		const char *name = cell->libc_->name_;
		if (!strncmp(name, "OAI21_X", 7) || !strncmp(name, "OAI221_X", 8) ||
		    !strncmp(name, "OAI222_X", 8) || !strncmp(name, "AOI21_X", 7) ||
		    !strncmp(name, "AOI22_X", 7) || !strncmp(name, "AOI211_X", 8))
		{
			++n;
		}
	}
	return n;
}

struct AtpgStats
{
	int dt = 0;
	int au = 0;
	int auComb = 0;
	int collapsed = 0;
	double fcScan = 0.0;
	double fcScanCollapsed = 0.0;
	double fcRaw = 0.0;
};

static AtpgStats runAtpg(Circuit *cir, bool useScanProtocol)
{
	FaultListExtract fl;
	fl.faultListType_ = FaultListExtract::SAF;
	fl.extractFaultFromCircuit(cir);

	fl.faultsInCircuit_.clear();
	for (size_t i = 0; i < fl.extractedFaults_.size(); ++i)
	{
		fl.faultsInCircuit_.push_back(&fl.extractedFaults_[i]);
	}

	if (useScanProtocol)
	{
		applyScanProtocol(cir, &fl);
	}

	PatternProcessor pc;
	pc.dynamicCompression_ = PatternProcessor::ON;
	pc.staticCompression_ = PatternProcessor::ON;
	pc.XFill_ = PatternProcessor::ON;
	Simulator sim(cir);
	Atpg atpg(cir, &sim);
	atpg.generatePatternSet(&pc, &fl, true);

	ScanProtocolStats scanStats = computeScanProtocolStats(cir, fl.faultsInCircuit_);

	AtpgStats s;
	s.dt = (int)scanStats.dtFull;
	s.au = (int)scanStats.auFull;
	s.auComb = (int)scanStats.auCombCollapsed;
	s.collapsed = (int)fl.extractedFaults_.size();
	s.fcScan = scanStats.fcScan;
	s.fcScanCollapsed = scanStats.fcScanCollapsed;
	s.fcRaw = scanStats.fcRaw;
	return s;
}

static void testTinySdffr(const char *mdt, const char *nlPath)
{
	Techlib *lib = nullptr;
	Netlist *nl = nullptr;
	if (!loadLib(lib, mdt) || !loadNetlist(nl, lib, nlPath))
	{
		fprintf(stderr, "FAIL: could not load tiny_sdffr fixtures\n");
		++failures;
		return;
	}

	Circuit cir;
	expect(cir.buildCircuit(nl, 1), "tiny_sdffr buildCircuit");
	AtpgStats st = runAtpg(&cir, true);
	fprintf(stderr, "tiny_sdffr: FC_scan=%.2f%% DT=%d AU=%d\n", st.fcScan, st.dt, st.au);
	expect(st.au == 0, "tiny_sdffr AU must stay 0");
	expect(st.fcScan >= 85.0, "tiny_sdffr FC_scan must be >= 85%");

	delete nl;
	delete lib;
}

static void testB03MuxAndPi(const char *mdt, const char *nlPath, const char *label)
{
	Techlib *lib = nullptr;
	Netlist *nl = nullptr;
	if (!loadLib(lib, mdt) || !loadNetlist(nl, lib, nlPath))
	{
		fprintf(stderr, "FAIL: could not load b03 fixtures\n");
		++failures;
		return;
	}

	Circuit cir;
	expect(cir.buildCircuit(nl, 1), "b03 buildCircuit");

	int mux = countMuxGates(cir);
	const int mux2x1 = countMux2X1Cells(*nl);
	const int oaiAoi = countOaiAoiCompoundCells(*nl);
	fprintf(stderr, "b03: atomic MUX gates=%d netlist MUX2_X1=%d OAI/AOI compounds=%d\n",
	        mux, mux2x1, oaiAoi);
	expect(mux == mux2x1, "b03 Gate::MUX count must match netlist MUX2_X1 count");
	expect(oaiAoi == 0, "b03 netlist must not contain OAI/AOI compound cells");

	FaultListExtract fl;
	fl.faultListType_ = FaultListExtract::SAF;
	fl.extractFaultFromCircuit(&cir);

	AtpgStats st = runAtpg(&cir, true);
	fprintf(stderr, "%s: FC_scan=%.2f%% FC_scan_coll=%.2f%% FC_raw=%.2f%% DT=%d AU=%d AU_comb=%d collapsed=%d\n",
	        label, st.fcScan, st.fcScanCollapsed, st.fcRaw, st.dt, st.au, st.auComb, st.collapsed);
	if (strstr(label, "scan_proto") != nullptr)
	{
		// Base-gate netlist (no OAI/AOI compounds): primitive PODEM ceiling ~90.5% on b03.
		expect(st.fcScan >= 90.0, "b03.v FC_scan must be >= 90% (base-gate pipeline)");
		expect(st.auComb <= 12, "b03.v comb AU must be <= 12 (base-gate pipeline)");
	}
	else
	{
		expect(st.fcScan >= 90.0, "b03_reset_tie FC_scan must be >= 90% (base-gate pipeline)");
		expect(st.auComb <= 12, "b03_reset_tie comb AU must be <= 12 (base-gate pipeline)");
	}

	delete nl;
	delete lib;
}

int main(int argc, char **argv)
{
	const char *root = argc > 1 ? argv[1] : ".";
	std::string mdt = std::string(root) + "/techlib/mod_nangate45.mdt";
	std::string tiny = std::string(root) + "/mod_netlist/tiny_sdffr.v";
	std::string b03 = std::string(root) + "/mod_netlist/b03.v";
	std::string b03ResetTie = std::string(root) + "/mod_netlist/b03_reset_tie.v";

	testTinySdffr(mdt.c_str(), tiny.c_str());
	testB03MuxAndPi(mdt.c_str(), b03.c_str(), "b03_scan_proto");
	testB03MuxAndPi(mdt.c_str(), b03ResetTie.c_str(), "b03_reset_tie");

	if (failures == 0)
	{
		printf("phase_d_test: ALL PASSED\n");
		return 0;
	}
	fprintf(stderr, "phase_d_test: %d FAILURE(S)\n", failures);
	return 1;
}
