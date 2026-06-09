# b03 Investigation: add_scan_chains Segfault & Low Full-Scan FC

## 1. add_scan_chains Segfault — ROOT CAUSE & FIX

### Symptom
`add_scan_chains` after `run_atpg` on b03 → SIGSEGV inside `Atpg::calSCOAP()`.

### Root Causes (two bugs)

**A. calSCOAP post-ATPG**
- `calSCOAP()` assumed `cc0_/cc1_/co_` were zero and called `std::cin.get()` on violation.
- After ATPG, gate fields could be non-zero; SCOAP pass also indexed empty `faninVector_` → crash.

**Fix:** Reset SCOAP fields at start; guard `numFI_>0` / `numFO_>0`; handle `Gate::NA`.

**B. Scan-port gate aliasing (see §2)**
- Brief fix that inserted pseudo gates between PI and PPI broke `numPI_+i` PPI layout → `AddScanChainsCmd` read wrong `cellId_` → `strlen` crash on bad pointer.

**Fix:** Place CK/test_si/test_se pseudo gates at **tail** of gate array; `createCircuitScanPorts()` after PPO; adjust PPO index with `numScanPseudoGates_` (otherwise PPO overlaps tail gates → s510 FC collapse).

### Verification
```bash
./pkg/fan/bin/opt/fan -f script/fanScripts/b03_fs.script
# → add_scan_chains OK, results/b03.sf exported
```

---

## 2. Low Full-Scan FC (~32–35%) — ROOT CAUSE

### Symptom
| Circuit | FC | AU | Notes |
|---------|-----|-----|-------|
| s510 | 99.14% | 0 | 0 MUX2, reference |
| b03 | 34.17% | 957 | 33 MUX2, 31 SDFFR (post-fix) |
| b07 | 42.75% | 1529 | larger FSM |

### Critical Bug: Unmapped Scan/Clock Ports

`calculateNumGate()` skips `CK`, `test_si`, `test_se` from PI list but **never assigns** `portIndexToGateIndex_[port]`.

Uninitialized index defaults to **0** → all SDFFR intern `_mux` **SE** pins and DFF **CK** pins fanin to **gate 0** (first functional PI).

On b03, gate 0 = **`reset`** → scan enable and clock structurally tied to reset during ATPG.

On s510, gate 0 = first data PI (`john`) → wrong but less destructive; explains why s510 still reaches ~99%.

### Fix (circuit.cpp)
- Append pseudo gates for CK/test_si/test_se at end of gate array.
- `test_se` / `test_si` → `Gate::TIE0` (capture-mode ATPG, SE=0).
- `CK` → `Gate::TIE0`.

### Additional FAN Gaps (secondary)

| Gap | Impact on b03 |
|-----|----------------|
| `Gate::MUX` missing in simulator | SDFFR intern mux good-sim wrong → fixed |
| `Gate::MUX` missing in ATPG implication | Justification through scan mux failed → fixed |
| `evaluateGoodVal` MUX case | Forward eval → fixed |
| 33× `MUX2_X1` comb cells | Decomposed to AND/OR in MDT — OK |
| Async `RN` on all SDFFR | 62 UD faults on QN only |

### AU Fault Pattern (b03)
- `request1–4` (all PIs): AU — inputs not observable at POs in frame-1 model
- ~21/31 SDFFR PPIs: AU
- Majority of comb gates (INV, OR3, MUX2 decomposed logic): AU

Interpretation: **FAN declares most of the controller core structurally untestable** in single-frame full-scan mode, even with correct SDFFR+scan netlist.

### Why Not ~100% After Netlist Format Fix?

1. **Real bug fixed** (port aliasing) — necessary but not sufficient.
2. **ITC'99 b03** is a mutex arbiter FSM; many states/feedback paths are not observable in one capture frame with FAN's PPI/PPO combinational model.
3. **s510** has no MUX2, simpler logic → FAN ATPG works.
4. FC did not jump to commercial-tool levels (~90%+) — likely FAN engine + frame-1 limitation, not netlist format alone.

### Full-Scan ≡ Combinational (no frame semantics)

With SDFFR + scan chain in place, **full-scan ATPG is a single combinational pass**:
- FF outputs → PPI (controllable), FF inputs → PPO (observable)
- No multi-frame launch/capture semantics apply; `build_circuit --frame 1` is the standard full-scan model
- Low FC on b03 is **not** a “wrong frame” problem — it is observability/controllability in that combinational view + FAN engine limits

### Recommended Next Steps
1. External reference (TetraMAX/ATPG on same `b03.v`) to separate FAN vs tool ceiling.
2. Partial-scan sweep (`set_nonscan_ff`) — project main line (frame>1 matters there).
3. Keep `add_scan_chains` + SCOAP fixes in FAN fork.

---

## Files Changed (FAN_ATPG)

- `pkg/core/src/circuit.cpp` — scan-port pseudo gates
- `pkg/core/src/atpg.cpp` — calSCOAP reset/guards, MUX implication
- `pkg/core/src/atpg.h` — MUX evaluateGoodVal
- `pkg/core/src/simulator.h` — MUX good/fault sim
