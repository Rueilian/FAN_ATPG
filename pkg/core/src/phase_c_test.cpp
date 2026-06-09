// Phase C regression: scan pseudo gates must not block PO/PPO observation.
// Run from FAN_ATPG/:  ./pkg/core/bin/opt/phase_c_test

#include <cstdio>
#include <cstdlib>
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

static void countObservationGates(const Circuit &cir, int &po, int &ppo, int &scanPseudo)
{
	po = ppo = scanPseudo = 0;
	for (int i = 0; i < cir.numGate_; ++i)
	{
		switch (cir.circuitGates_[i].gateType_)
		{
			case Gate::PO:
				++po;
				break;
			case Gate::PPO:
				++ppo;
				break;
			case Gate::TIE0:
				if (i >= cir.numGate_ - cir.numScanPseudoGates_)
				{
					++scanPseudo;
				}
				break;
			default:
				break;
		}
	}
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

	for (size_t i = 0; i < fl.extractedFaults_.size(); ++i)
	{
		fl.faultsInCircuit_.push_back(&fl.extractedFaults_[i]);
	}

	PatternProcessor pc;
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

	int po = 0, ppo = 0, scanPseudo = 0;
	countObservationGates(cir, po, ppo, scanPseudo);
	expect(po == 1, "tiny_sdffr expects 1 PO");
	expect(ppo == 1, "tiny_sdffr expects 1 PPO");
	expect(scanPseudo == 3, "tiny_sdffr expects 3 scan pseudo gates at tail");
	expect(cir.numScanPseudoGates_ == 3, "numScanPseudoGates_ == 3");

	AtpgStats st = runAtpg(&cir);
	fprintf(stderr, "tiny_sdffr: FC=%.2f%% DT=%d AU=%d collapsed=%d\n",
	        st.fc, st.dt, st.au, st.collapsed);
	expect(st.au == 0, "tiny_sdffr AU must be 0 after Phase C fix");
	expect(st.fc >= 85.0, "tiny_sdffr FC must be >= 85%");

	delete nl;
	delete lib;
}

int main(int argc, char **argv)
{
	const char *root = argc > 1 ? argv[1] : ".";
	std::string mdt = std::string(root) + "/techlib/mod_nangate45.mdt";
	std::string tiny = std::string(root) + "/mod_netlist/tiny_sdffr.v";

	testTinySdffr(mdt.c_str(), tiny.c_str());

	if (failures == 0)
	{
		printf("phase_c_test: ALL PASSED\n");
		return 0;
	}
	fprintf(stderr, "phase_c_test: %d FAILURE(S)\n", failures);
	return 1;
}
