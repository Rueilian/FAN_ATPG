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

struct AtpgStats
{
	int dt = 0;
	int au = 0;
	int collapsed = 0;
	double fc = 0.0;
};

static AtpgStats runAtpg(Circuit *cir)
{
	FaultListExtract fl;
	fl.faultListType_ = FaultListExtract::SAF;
	fl.extractFaultFromCircuit(cir);

	fl.faultsInCircuit_.clear();
	for (size_t i = 0; i < fl.extractedFaults_.size(); ++i)
	{
		fl.faultsInCircuit_.push_back(&fl.extractedFaults_[i]);
	}

	PatternProcessor pc;
	pc.dynamicCompression_ = PatternProcessor::ON;
	pc.staticCompression_ = PatternProcessor::ON;
	pc.XFill_ = PatternProcessor::ON;
	Simulator sim(cir);
	Atpg atpg(cir, &sim);
	atpg.generatePatternSet(&pc, &fl, true);

	AtpgStats s;
	for (Fault *f : fl.faultsInCircuit_)
	{
		if (f->faultState_ == Fault::DT)
		{
			++s.dt;
		}
		else if (f->faultState_ == Fault::AU)
		{
			++s.au;
		}
	}
	s.collapsed = (int)fl.extractedFaults_.size();
	s.fc = s.collapsed > 0 ? 100.0 * s.dt / s.collapsed : 0.0;
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
	AtpgStats st = runAtpg(&cir);
	fprintf(stderr, "tiny_sdffr: FC=%.2f%% DT=%d AU=%d\n", st.fc, st.dt, st.au);
	expect(st.au == 0, "tiny_sdffr AU must stay 0");
	expect(st.fc >= 85.0, "tiny_sdffr FC must be >= 85%");

	delete nl;
	delete lib;
}

static void testB03MuxAndPi(const char *mdt, const char *nlPath)
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
	fprintf(stderr, "b03: atomic MUX gates=%d\n", mux);
	expect(mux == 33, "b03 must have 33 Gate::MUX cells");

	FaultListExtract fl;
	fl.faultListType_ = FaultListExtract::SAF;
	fl.extractFaultFromCircuit(&cir);

	AtpgStats st = runAtpg(&cir);
	fprintf(stderr, "b03 full: FC=%.2f%% DT=%d AU=%d collapsed=%d\n",
	        st.fc, st.dt, st.au, st.collapsed);
	expect(st.fc >= 70.0, "b03 FC must be >= 70% after Phase D MUX2 modeling");
	expect(st.au < 300, "b03 AU must drop below 300 after Phase D");

	delete nl;
	delete lib;
}

int main(int argc, char **argv)
{
	const char *root = argc > 1 ? argv[1] : ".";
	std::string mdt = std::string(root) + "/techlib/mod_nangate45.mdt";
	std::string tiny = std::string(root) + "/mod_netlist/tiny_sdffr.v";
	std::string b03 = std::string(root) + "/mod_netlist/b03.v";

	testTinySdffr(mdt.c_str(), tiny.c_str());
	testB03MuxAndPi(mdt.c_str(), b03.c_str());

	if (failures == 0)
	{
		printf("phase_d_test: ALL PASSED\n");
		return 0;
	}
	fprintf(stderr, "phase_d_test: %d FAILURE(S)\n", failures);
	return 1;
}
