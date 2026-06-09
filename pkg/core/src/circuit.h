// **************************************************************************
// File       [ circuit.h ]
// Author     [ littleshamoo ]
// Synopsis   [ This file define the circuits. ]
// Date       [ 2010/12/29 created ]
// **************************************************************************

#ifndef _CORE_CIRCUIT_H_
#define _CORE_CIRCUIT_H_

#include "interface/netlist.h"
#include "gate.h"

namespace CoreNs
{
	class Circuit
	{
	public:
		// Specify how to connect multiple time frames of circuits.
		// CAPTURE means Launch-on-capture; SHIFT means launch-on-shift.
		// PARTIAL_SEQUENTIAL: non-scan FFs carry state across frames; scan FFs remain free.
		enum TIME_FRAME_CONNECT_TYPE
		{
			CAPTURE = 0,
			SHIFT,
			PARTIAL_SEQUENTIAL
		};

		inline Circuit();

		// Build the circuit from the netlist.
		bool buildCircuit(IntfNs::Netlist *const pNetlist, const int &numFrame = 1,
		                  const TIME_FRAME_CONNECT_TYPE &timeFrameConnectType = CAPTURE);

		// Non-scan FF cell names; set before buildCircuit() to enable PARTIAL_SEQUENTIAL mode.
		std::vector<std::string> nonscanCellNames_;
		// Per-PPI flag: true = non-scan FF (state carries across frames). Populated by buildCircuit().
		std::vector<bool> isPpiNonscan_;
		inline bool isObservablePpoGateId(const int &gateId) const;

		// Info for one time frame.
		IntfNs::Netlist *pNetlist_; // Corresponding netlist.
		int numPI_;                 // Number of PIs.
		int numPPI_;                // Number of PPIs (PPOs).
		int numPO_;                 // Number of POs.
		int numComb_;               // Number of combinational gates.
		int numScanPseudoGates_;    // CK/test_si/test_se pseudo gates at array tail.
		int numGate_;               // Number of gates.
		int numNet_;                // Number of nets.
		int circuitLvl_;            // Circuit level, starting from inputs.

		// Info for multiple time frames.
		int numFrame_;                                 // Number of time frames.
		TIME_FRAME_CONNECT_TYPE timeFrameConnectType_; // Time frame connection type.
		int totalGate_;                                // Number of total gates. Equal to numGate_ * numFrame_.
		int totalLvl_;                                 // Total levels. Equal to circuitLvl_ * numFrame_.

		// Structure
		// **********************************************************************
		// frame 1                frame 2                      frame n
		// |--- ---- ---- --- ----|--- ---- ---- --- ----|     |--- ---- ----
		// |PI1|PPI1|cell|PO1|PPO1|PI2|PPI2|cell|PO2|PPO2| ... |PIn|PPIn|cell|...
		// |--- ---- ---- --- ----|--- ---- ---- --- ----|     |--- ---- ----
		// **********************************************************************

		std::vector<Gate> circuitGates_;        // Gates in the circuit.
		std::vector<int> cellIndexToGateIndex_; // Map cells in the netlist to gates.
		std::vector<int> portIndexToGateIndex_; // Map ports in the netlist to gates.

	protected:
		// For circuit building.
		void mapNetlistToCircuit();
		void calculateNumGate();
		void calculateNumNet();
		void createCircuitGates();
		void createCircuitPI();
		void createCircuitScanPorts();
		void createCircuitPPI();
		void createCircuitComb();
		void createCircuitMux2(const int &gateID, IntfNs::Cell *const cell);
		void createCircuitPmt(const int &gateID, const IntfNs::Cell *const cell,
		                      const IntfNs::Pmt *const pmt);
		void determineGateType(const int &gateID, const IntfNs::Cell *const cell,
		                       const IntfNs::Pmt *const pmt);
		void createCircuitPO();
		void createCircuitPPO();
		void connectMultipleTimeFrame();
		void assignMinLevelOfFanins();
	};

	inline Circuit::Circuit()
	{
		pNetlist_ = NULL;
		numPI_ = 0;
		numPPI_ = 0;
		numPO_ = 0;
		numComb_ = 0;
		numScanPseudoGates_ = 0;
		numGate_ = 0;
		numNet_ = 0;
		circuitLvl_ = -1;
		numFrame_ = 1;
		timeFrameConnectType_ = CAPTURE;
	}

	inline bool Circuit::isObservablePpoGateId(const int &gateId) const
	{
		if (isPpiNonscan_.empty())
		{
			return true;
		}
		const int finalPpoStart = totalGate_ - numScanPseudoGates_ - numPPI_;
		const int ppiIndex = gateId - finalPpoStart;
		return ppiIndex >= 0 && ppiIndex < numPPI_ && !isPpiNonscan_[ppiIndex];
	}
};

#endif