// **************************************************************************
// File       [ fan_mgr.h ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/08/30 created ]
// **************************************************************************

#ifndef _FAN_FAN_MGR_H_
#define _FAN_FAN_MGR_H_

#include <string>
#include <vector>

#include "common/tm_usage.h"

#include "interface/netlist.h"
#include "interface/techlib.h"

#include "core/atpg.h"

namespace FanNs {

class FanMgr {
public:
    FanMgr() {
        lib            = NULL;
        nl             = NULL;
        fListExtract          = NULL;
        pcoll          = NULL;
        cir            = NULL;
        sim            = NULL;
        atpg           = NULL;
        perTargetTimeout_ = 0.0;
        atpgThreads_ = 0;
        scanProtocolEnabled_ = true;
        scanProtocolApplied_ = false;
        useTwoPhaseJustification_ = true;
        useNineValuedLogic_ = false;
        atpgStat.rTime = 0;
    }
    ~FanMgr() {}

    IntfNs::Techlib     *lib;
    IntfNs::Netlist     *nl;
    CoreNs::FaultListExtract   *fListExtract;;
    CoreNs::PatternProcessor *pcoll;
    CoreNs::Circuit     *cir;
    CoreNs::Simulator   *sim;
    CoreNs::Atpg        *atpg;
    double               perTargetTimeout_;  // stored per-target timeout until Atpg is created
    int                  atpgThreads_;       // parallel fault-partition workers (1 = sequential)
    bool                 scanProtocolEnabled_;  // auto TI async reset/control PIs (default on)
    bool                 scanProtocolApplied_;
    bool                 useTwoPhaseJustification_;
    bool                 useNineValuedLogic_;
    CommonNs::TmUsage   tmusg;
    CommonNs::TmStat    atpgStat;
    // Cell names of FFs declared as non-scan (set by set_nonscan_ff before build_circuit).
    std::vector<std::string> nonscanFfNames;
};

};

#endif


