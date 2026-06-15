// **************************************************************************
// File       [ main.cpp ]
// Author     [ littleshamoo ]
// Synopsis   [ ]
// Date       [ 2011/07/05 created ]
// **************************************************************************

#include <cstdlib>
#include <thread>

#include "common/sys_cmd.h"
#include "setup_cmd.h"
#include "atpg_cmd.h"
#include "misc_cmd.h"

using namespace CommonNs;
using namespace IntfNs;
using namespace CoreNs;
using namespace FanNs;

void printWelcome();
void initOpt(OptMgr &mgr);
void initCmd(CmdMgr &cmdMgr, FanMgr &fanMgr);
void printGoodbye(TmUsage &tmusg);

int main(int argc, char **argv)
{

	// start calculating resource usage
	TmUsage tmusg;
	tmusg.totalStart();

	// initialize option manager
	OptMgr optMgr;
	initOpt(optMgr);

	optMgr.parse(argc, argv);
	if (optMgr.isFlagSet("h"))
	{
		optMgr.usage();
		exit(0);
	}

	// initialize command manager and FAN manager
	FanMgr fanMgr;
	CmdMgr cmdMgr;
	initCmd(cmdMgr, fanMgr);
	CmdMgr::Result res = CmdMgr::SUCCESS;

	// welcome message
	printWelcome();

	// run startup commands
	if (optMgr.isFlagSet("f"))
	{
		std::string cmd = "source " + optMgr.getFlagVar("f");
		res = cmdMgr.exec(cmd);
	}

	// enter user interface
	while (res != CmdMgr::EXIT)
	{
		res = cmdMgr.read();
		if (res == CmdMgr::NOT_EXIST)
		{
			std::cerr << "**ERROR main(): command `" << cmdMgr.getErrorStr();
			std::cerr << "' not found" << "\n";
			continue;
		}
	}

	// goodbye
	printGoodbye(tmusg);
	return 0;
}

void printWelcome()
{
	std::cout << "#  ==========================================================================" << "\n";
	std::cout << "#" << "\n";
	std::cout << "#                                   FAN ATPG" << "\n";
	std::cout << "#" << "\n";
	std::cout << "#                Copyright(c) Laboratory of Dependable Systems," << "\n";
	std::cout << "#                Graduate Institute of Electronics Engineering," << "\n";
	std::cout << "#                          National Taiwan University" << "\n";
	std::cout << "#                             All Rights Reserved." << "\n";
	std::cout << "#" << "\n";
	std::cout << "#  ==========================================================================" << "\n";
	std::cout << "#" << "\n";

	// system information
	// OS kernel
	FILE *systemOutput;
	systemOutput = popen("uname -s 2> /dev/null", "r");
	char buf[128];
	std::cout << "#  Kernel:   ";
	if (!systemOutput)
		std::cout << "UNKNOWN" << "\n";
	else
	{
		if (fgets(buf, sizeof(buf), systemOutput))
			std::cout << buf;
		else
			std::cout << "UNKNOWN" << "\n";
		pclose(systemOutput);
	}

	// platform
	systemOutput = popen("uname -i 2> /dev/null", "r");
	std::cout << "#  Platform: ";
	if (!systemOutput)
		std::cout << "UNKNOWN" << "\n";
	else
	{
		if (fgets(buf, sizeof(buf), systemOutput))
			std::cout << buf;
		else
			std::cout << "UNKNOWN" << "\n";
		pclose(systemOutput);
	}

	// memory
	FILE *meminfo = fopen("/proc/meminfo", "r");
	std::cout << "#  Memory:   ";
	if (!meminfo)
		std::cout << "UNKNOWN" << "\n";
	else
	{
		while (fgets(buf, 128, meminfo))
		{
			char *ch;
			if ((ch = strstr(buf, "MemTotal:")))
			{
				std::cout << (double)atol(ch + 9) / 1024.0 << " MB" << "\n";
				break;
			}
		}
		fclose(meminfo);
	}

	std::cout << "#" << "\n";
}

void printGoodbye(TmUsage &tmusg)
{
	TmStat stat;
	tmusg.getTotalUsage(stat);
	std::cout << "#  Goodbye" << "\n";
	std::cout << "#  Runtime        ";
	std::cout << "real " << (double)stat.rTime / 1000000.0 << " s        ";
	std::cout << "user " << (double)stat.uTime / 1000000.0 << " s        ";
	std::cout << "sys " << (double)stat.sTime / 1000000.0 << " s" << "\n";
	std::cout << "#  Memory         ";
	std::cout << "peak " << (double)stat.vmPeak / 1024.0 << " MB" << "\n";
}

void initOpt(OptMgr &mgr)
{
	// set program information
	mgr.setName("fan");
	mgr.setShortDes("FAN based ATPG");
	mgr.setDes("FAN based ATPG");

	// register options
	Opt *opt = new Opt(Opt::BOOL, "print this usage", "");
	opt->addFlag("h");
	opt->addFlag("help");
	mgr.regOpt(opt);

	opt = new Opt(Opt::STR_REQ, "execute command file at startup", "file");
	opt->addFlag("f");
	mgr.regOpt(opt);
}

void initCmd(CmdMgr &cmdMgr, FanMgr &fanMgr)
{
	// system commands
	Cmd *listCmd = new SysListCmd("ls");
	Cmd *cdCmd = new SysCdCmd("cd");
	Cmd *catCmd = new SysCatCmd("cat");
	Cmd *pwdCmd = new SysPwdCmd("pwd");
	Cmd *setCmd = new SysSetCmd("set", &cmdMgr);
	Cmd *exitCmd = new SysExitCmd("exit", &cmdMgr);
	Cmd *quitCmd = new SysExitCmd("quit", &cmdMgr);
	Cmd *sourceCmd = new SysSourceCmd("source", &cmdMgr);
	Cmd *helpCmd = new SysHelpCmd("help", &cmdMgr);
	cmdMgr.regCmd("SYSTEM", listCmd);
	cmdMgr.regCmd("SYSTEM", cdCmd);
	cmdMgr.regCmd("SYSTEM", catCmd);
	cmdMgr.regCmd("SYSTEM", pwdCmd);
	cmdMgr.regCmd("SYSTEM", setCmd);
	cmdMgr.regCmd("SYSTEM", exitCmd);
	cmdMgr.regCmd("SYSTEM", quitCmd);
	cmdMgr.regCmd("SYSTEM", sourceCmd);
	cmdMgr.regCmd("SYSTEM", helpCmd);

	// setup commands
	Cmd *readLibCmd = new ReadLibCmd("read_lib", &fanMgr);
	Cmd *readNlCmd = new ReadNlCmd("read_netlist", &fanMgr);
	Cmd *setFaultTypeCmd = new SetFaultTypeCmd("set_fault_type", &fanMgr);
	Cmd *buildCirCmd = new BuildCircuitCmd("build_circuit", &fanMgr);
	Cmd *reportNlCmd = new ReportNetlistCmd("report_netlist", &fanMgr);
	Cmd *reportCellCmd = new ReportCellCmd("report_cell", &fanMgr);
	Cmd *reportLibCmd = new ReportLibCmd("report_lib", &fanMgr);
	Cmd *setPatternTypeCmd = new SetPatternTypeCmd("set_pattern_type", &fanMgr);
	Cmd *setStaticCompressionCmd = new SetStaticCompressionCmd("set_static_compression", &fanMgr);
	Cmd *setDynamicCompressionCmd = new SetDynamicCompressionCmd("set_dynamic_compression", &fanMgr);
	Cmd *setXFillCmd = new SetXFillCmd("set_X-Fill", &fanMgr);
	Cmd *setScanProtocolCmd = new SetScanProtocolCmd("set_scan_protocol", &fanMgr);
	Cmd *setNonscanFfCmd = new SetNonscanFfCmd("set_nonscan_ff", &fanMgr);

	// set_per_target_timeout <seconds>
	class SetPerTargetTimeoutCmd : public CommonNs::Cmd {
	public:
		SetPerTargetTimeoutCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_per_target_timeout <seconds>\n"; return false; }
			double sec;
			try { sec = std::stod(argv[1]); }
			catch (...) { std::cerr << "**ERROR invalid timeout value: " << argv[1] << "\n"; return false; }
			if (!fm_->atpg) {
				fm_->perTargetTimeout_ = sec;
			} else {
				fm_->atpg->setPerTargetTimeoutSec(sec);
			}
			std::cout << "#  per-target timeout set to " << sec << " s\n";
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setPerTargetTimeoutCmd = new SetPerTargetTimeoutCmd("set_per_target_timeout", &fanMgr);

	class SetTwoPhaseJustificationCmd : public CommonNs::Cmd {
	public:
		SetTwoPhaseJustificationCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_two_phase_justification <on/off>\n"; return false; }
			std::string val = argv[1];
			if (val == "on" || val == "1" || val == "true") {
				fm_->useTwoPhaseJustification_ = true;
				std::cout << "#  two-phase state justification enabled\n";
			} else {
				fm_->useTwoPhaseJustification_ = false;
				std::cout << "#  two-phase state justification disabled\n";
			}
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setTwoPhaseJustificationCmd = new SetTwoPhaseJustificationCmd("set_two_phase_justification", &fanMgr);

	class SetNineValuedLogicCmd : public CommonNs::Cmd {
	public:
		SetNineValuedLogicCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_nine_valued_logic <on/off>\n"; return false; }
			std::string val = argv[1];
			if (val == "on" || val == "1" || val == "true") {
				fm_->useNineValuedLogic_ = true;
				if (fm_->atpg) {
					fm_->atpg->useNineValuedLogic_ = true;
				}
				std::cout << "#  nine-valued ATPG logic enabled\n";
			} else {
				fm_->useNineValuedLogic_ = false;
				if (fm_->atpg) {
					fm_->atpg->useNineValuedLogic_ = false;
				}
				std::cout << "#  five-valued ATPG logic enabled\n";
			}
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setNineValuedLogicCmd = new SetNineValuedLogicCmd("set_nine_valued_logic", &fanMgr);

	class SetAtpgThreadsCmd : public CommonNs::Cmd {
	public:
		SetAtpgThreadsCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) {
				std::cerr << "**ERROR usage: set_atpg_threads <N>  (1=sequential, 0=auto)\n";
				return false;
			}
			int n;
			try { n = std::stoi(argv[1]); }
			catch (...) { std::cerr << "**ERROR invalid thread count: " << argv[1] << "\n"; return false; }
			if (n == 0) {
				n = static_cast<int>(std::thread::hardware_concurrency());
				if (n < 1) n = 1;
			}
			if (n < 1) {
				std::cerr << "**ERROR thread count must be >= 1\n";
				return false;
			}
			fm_->atpgThreads_ = n;
			if (fm_->atpg) {
				fm_->atpg->setNumThreads(n);
			}
			std::cout << "#  ATPG parallel workers set to " << n << "\n";
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setAtpgThreadsCmd = new SetAtpgThreadsCmd("set_atpg_threads", &fanMgr);

	// --- ATPG optimization flags ---
	class SetEnhancedBacktraceCmd : public CommonNs::Cmd {
	public:
		SetEnhancedBacktraceCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_enhanced_backtrace <on/off>\n"; return false; }
			std::string val = argv[1];
			fm_->useEnhancedBacktrace_ = (val == "on" || val == "1" || val == "true");
			std::cout << "#  enhanced backtrace " << (fm_->useEnhancedBacktrace_ ? "enabled" : "disabled") << "\n";
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setEnhancedBacktraceCmd = new SetEnhancedBacktraceCmd("set_enhanced_backtrace", &fanMgr);

	class SetBackjumpCmd : public CommonNs::Cmd {
	public:
		SetBackjumpCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_backjump <on/off>\n"; return false; }
			std::string val = argv[1];
			fm_->useBackjump_ = (val == "on" || val == "1" || val == "true");
			std::cout << "#  non-chronological backtracking " << (fm_->useBackjump_ ? "enabled" : "disabled") << "\n";
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setBackjumpCmd = new SetBackjumpCmd("set_backjump", &fanMgr);

	class SetDominatorCheckCmd : public CommonNs::Cmd {
	public:
		SetDominatorCheckCmd(const std::string &name, FanMgr *fm) : Cmd(name) { fm_ = fm; }
		bool exec(const std::vector<std::string> &argv) override {
			if (argv.size() < 2) { std::cerr << "**ERROR usage: set_dominator_check <on/off>\n"; return false; }
			std::string val = argv[1];
			fm_->useDominatorCheck_ = (val == "on" || val == "1" || val == "true");
			std::cout << "#  dominator check " << (fm_->useDominatorCheck_ ? "enabled" : "disabled") << "\n";
			return true;
		}
	private: FanMgr *fm_;
	};
	Cmd *setDominatorCheckCmd = new SetDominatorCheckCmd("set_dominator_check", &fanMgr);

	cmdMgr.regCmd("SETUP", readLibCmd);
	cmdMgr.regCmd("SETUP", readNlCmd);
	cmdMgr.regCmd("SETUP", setFaultTypeCmd);
	cmdMgr.regCmd("SETUP", buildCirCmd);
	cmdMgr.regCmd("SETUP", reportNlCmd);
	cmdMgr.regCmd("SETUP", reportCellCmd);
	cmdMgr.regCmd("SETUP", reportLibCmd);
	cmdMgr.regCmd("SETUP", setPatternTypeCmd);
	cmdMgr.regCmd("SETUP", setStaticCompressionCmd);
	cmdMgr.regCmd("SETUP", setDynamicCompressionCmd);
	cmdMgr.regCmd("SETUP", setXFillCmd);
	cmdMgr.regCmd("SETUP", setScanProtocolCmd);
	cmdMgr.regCmd("SETUP", setNonscanFfCmd);
	cmdMgr.regCmd("SETUP", setPerTargetTimeoutCmd);
	cmdMgr.regCmd("SETUP", setTwoPhaseJustificationCmd);
	cmdMgr.regCmd("SETUP", setNineValuedLogicCmd);
	cmdMgr.regCmd("SETUP", setAtpgThreadsCmd);
	cmdMgr.regCmd("SETUP", setEnhancedBacktraceCmd);
	cmdMgr.regCmd("SETUP", setBackjumpCmd);
	cmdMgr.regCmd("SETUP", setDominatorCheckCmd);

	// ATPG commands
	Cmd *readPatCmd = new ReadPatCmd("read_pattern", &fanMgr);
	Cmd *reportPatCmd = new ReportPatCmd("report_pattern", &fanMgr);
	Cmd *addFaultCmd = new AddFaultCmd("add_fault", &fanMgr);
	Cmd *reportFaultCmd = new ReportFaultCmd("report_fault", &fanMgr);
	Cmd *addPinConsCmd = new AddPinConsCmd("add_pin_constraint", &fanMgr);
	Cmd *runLogicSimCmd = new RunLogicSimCmd("run_logic_sim", &fanMgr);
	Cmd *runFaultSimCmd = new RunFaultSimCmd("run_fault_sim", &fanMgr);
	Cmd *runAtpgCmd = new RunAtpgCmd("run_atpg", &fanMgr);
	Cmd *reportCircuitCmd = new ReportCircuitCmd("report_circuit", &fanMgr);
	Cmd *reportGateCmd = new ReportGateCmd("report_gate", &fanMgr);
	Cmd *reportValueCmd = new ReportValueCmd("report_value", &fanMgr);
	Cmd *reportStatsCmd = new ReportStatsCmd("report_statistics", &fanMgr);
	Cmd *writePatCmd = new WritePatCmd("write_pattern", &fanMgr);
	Cmd *writeStilCmd = new WriteStilCmd("write_to_STIL", &fanMgr);
	Cmd *writeProcCmd = new WriteProcCmd("write_test_procedure_file", &fanMgr);
	Cmd *addScanChainsCmd = new AddScanChainsCmd("add_scan_chains", &fanMgr);
	cmdMgr.regCmd("ATPG", readPatCmd);
	cmdMgr.regCmd("ATPG", reportPatCmd);
	cmdMgr.regCmd("ATPG", addFaultCmd);
	cmdMgr.regCmd("ATPG", reportFaultCmd);
	cmdMgr.regCmd("ATPG", addPinConsCmd);
	cmdMgr.regCmd("ATPG", runLogicSimCmd);
	cmdMgr.regCmd("ATPG", runFaultSimCmd);
	cmdMgr.regCmd("ATPG", runAtpgCmd);
	cmdMgr.regCmd("ATPG", reportCircuitCmd);
	cmdMgr.regCmd("ATPG", reportGateCmd);
	cmdMgr.regCmd("ATPG", reportValueCmd);
	cmdMgr.regCmd("ATPG", reportStatsCmd);
	cmdMgr.regCmd("ATPG", writePatCmd);
	cmdMgr.regCmd("ATPG", writeStilCmd);
	cmdMgr.regCmd("ATPG", writeProcCmd);
	cmdMgr.regCmd("ATPG", addScanChainsCmd);

	// misc commands
	Cmd *reportPatFormatCmd = new ReportPatFormatCmd("report_pattern_format");
	Cmd *reportMemUsgCmd = new ReportMemUsgCmd("report_memory_usage");
	cmdMgr.regCmd("MISC", reportPatFormatCmd);
	cmdMgr.regCmd("MISC", reportMemUsgCmd);

	// user interface
	cmdMgr.setComment('#');
	cmdMgr.setPrompt("fan> ");
	cmdMgr.setColor(CmdMgr::YELLOW);
}
