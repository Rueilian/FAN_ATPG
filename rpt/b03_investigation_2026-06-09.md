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
| s510 | 94.86% | 60 | SDFF_X1 (no RN), 0 MUX2 |
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
2. **Industry expectation** for scanned ITC'99 gate netlists is **95–99%+** FC (commercial ATPG on b03 lists ~882 collapsed faults; DVCON Modus/Xcelium comparison reports PASS at full list size). **34% is not a normal full-scan ceiling.**
3. **957 AU dominate** — among faults FAN treats as testable (non-AU), **test coverage ≈ 89.5%** (529 DT / 591 non-AU). The gap is **mass AU classification**, not weak pattern generation on reachable faults.
4. **s510** still reaches ~95% with the same SDFF intern (_mux + _dff) model; **b03** adds SDFFR+RN, 33 comb MUX2, and heavy FSM feedback → FAN PODEM gives up on far more targets.

### Full-Scan ≡ Combinational (no frame semantics)

With SDFFR + scan chain in place, **full-scan ATPG is a single combinational pass**:
- FF outputs → PPI (controllable), FF inputs → PPO (observable)
- No multi-frame launch/capture semantics apply; `build_circuit --frame 1` is the standard full-scan model
- Low FC on b03 is **not** a “wrong frame” problem — it is observability/controllability in that combinational view + FAN engine limits

## 4. Phase C Fix (2026-06-09) — IMPLEMENTED

### Root cause (C1)

`Atpg::checkIfFaultHasPropagatedToPO()` used `totalGate_ - i - 1` for `i in [0, numPO_+numPPI_)`, assuming PO/PPO occupy the array tail. After Phase A scan-port fix, **3 scan pseudo gates sit at the tail**, so designs with `numPO_+numPPI_ ≤ 3` (e.g. `tiny_sdffr`) never observed real PO/PPO → all functional faults **AU**.

### Fix (`atpg.cpp`)

Iterate all gates; check `gateType_ == PO` or observable `PPO` for `D`/`B` values.

### Results

| Circuit | FC before | FC after | AU before | AU after |
|---------|-----------|----------|-----------|----------|
| tiny_sdffr | 58.3% | **91.7%** | 8 | **0** |
| tiny_sdff | 58.3% | **91.7%** | 8 | **0** |
| s510 | 94.9% | **99.1%** | 60 | **0** |
| b03 | 34.2% | **34.8%** | 957 | 948 |

b03 improves marginally — remaining AU is MUX2/FSM PODEM ceiling, not observation indexing.

### Tests

- C++: `pkg/core/bin/opt/phase_c_test`
- Shell: `scripts/test_phase_c_atpg.sh`

### Recommended Next Steps

> **Phase C 計劃（已完成）：** [`docs/superpowers/plans/2026-06-09-phase-c-fan-atpg-fix.md`](../../docs/superpowers/plans/2026-06-09-phase-c-fan-atpg-fix.md)

1. **C1（已完成）：** 修復 `checkIfFaultHasPropagatedToPO` — 改為遍歷 `Gate::PO`/`Gate::PPO`，不再從 array tail 索引（scan pseudo 會遮蓋真實 PO/PPO）
2. **C2（已完成）：** reset tie-high（`b03_reset_tie.v`）FC=33.96%，無改善；per-FF RN retarget 因 dangling net 無法通過 FAN check
3. **C3（已完成）：** 見 `rpt/b03_external_atpg_comparison.md` — 商業工具預期 ~99%，FAN b03 仍 ~35%
4. **G2 決策（6/11）：** FC ≥ 80% → Phase D sweep；否則 fallback 敘事
5. Partial-scan sweep — G2 後主線（frame>1 僅適用 partial-scan）

---

## 3. Continued Research (2026-06-09) — Web + Isolation Experiments

### 3.1 Industry / literature: common low-FC causes (full scan)

| Cause (commercial DFT literature) | Applies to our b03 baseline? |
|-----------------------------------|------------------------------|
| **Tied / blocked / redundant** faults (pins forced constant) | Partially: `test_se`/`CK` tied L via `TIE0` is **intentional** capture-mode constraint; not the main 957 AU source. |
| **Scan DRC** (broken chain, unscannable FF, clock/reset violations) | Netlist passes structural full-scan verify; chains export OK. Unlikely primary cause. |
| **Bad test protocol** (scan_en, capture clock, reset sequence) | FAN uses single-frame PPI/PPO combinational view — no explicit shift protocol. Standard for full-scan baseline, but intern mux still in graph. |
| **Async set/reset not deasserted in test mode** | **Plausible:** all b03 FFs are `SDFFR_X1`; RN is a functional PI, not tied high. Commercial flows typically force reset inactive during ATPG. |
| **SEQ depth too low** (non-scan logic) | **N/A for full-scan baseline** — user framing confirmed: with scan inserted, circuit ≡ combinational (PPI/PPO). |
| **ATPG abort / backtrack limit** | b03: **AB=0**; AU is not abort. Disabling compression (97 patterns vs 11) still **FC ≈ 33%**. |
| **Algorithm AU vs true untestability** | **Primary suspect:** FAN marks PODEM backtrack exhaustion as `FAULT_UNTESTABLE` → AU, not formal redundancy. |

References: [Electronic Design — Debugging Low Test-Coverage](https://www.electronicdesign.com/news/products/article/21760377/debugging-low-test-coverage-situations); [EE Times DFT rules](https://www.eetimes.com/systematic-methodology-with-df-t-rules-reduces-fault-coverage-analysis/); [DVCON Modus/Xcelium b03 comparison](https://dvcon-proceedings.org/wp-content/uploads/improving-the-confidence-level-in-functional-safety-simulation-tools-for-iso-26262.pdf) (882 faults, PASS).

### 3.2 Minimal-circuit isolation (FAN-specific)

| Netlist | FC | AU | Functional-path DT (d, q, PPI, PPO)? |
|---------|-----|-----|--------------------------------------|
| `tiny_dffr` (DFFR, no scan) | 87.5% | 0 | **Yes** — d, q, PPI, PPO all DT |
| `tiny_sdffr` (SDFFR + scan) | 58.3% | 8 | **No** — d, q, PPI, PPO all **AU** |
| `tiny_sdff` (SDFF + scan, no RN) | 58.3% | 8 | **No** — same AU set |

**DT faults on tiny_sdff(r)** are only: pseudo ports (`CK`, `test_si`, `test_se`, `test_so`) and cell-level CK/SE/SI pin stubs — **not** the functional datapath.

**Conclusion:** After scan insertion, FAN fails to detect even **single-FF** PI→FF→PO faults, while DFFR without scan works. The regression is tied to **SDFF/SDFFR intern (_mux + `_dff` as `Gate::NA`)** coexisting with PPI/PPO abstraction — not to b03 FSM size alone.

s510 still achieves ~95% because large combinational logic between PPI/PPO provides alternative observe/control paths; b03's mutex arbiter + MUX2-heavy logic hits FAN implication/backtrack limits more often (341/957 AU on `MUX2_X1`).

### 3.3 b03 AU breakdown (unchanged after fixes)

- **341** `MUX2_X1` (37% of AU)
- **46** `SDFFR_X1`
- **8** primary inputs (`request1–4` SA0/SA1)
- **6** primary outputs (`grant_o_*` SA0/SA1) — PO stuck-at marked AU suggests FAN cannot justify observation even at PO
- **62** UD (all QN pins) — structurally hard with RN as PI; separate from AU bucket

### 3.4 Ranked root-cause hypothesis

1. **FAN ATPG engine ceiling** — excessive AU from incomplete handling of scan-cell internals + reconvergent MUX/FSM logic (confirmed by tiny_sdff isolation).
2. **Async RN not tied in test mode** — industry practice; may convert some blocked paths (secondary).
3. **NOT** netlist format / scan-chain structure / frame count — format verified; full-scan ≡ frame-1 combinational.
4. **NOT** commercial-style untestability — external tools expect ~99% on comparable scanned ITC'99 circuits.

---

## Files Changed (FAN_ATPG)

- `pkg/core/src/circuit.cpp` — scan-port pseudo gates
- `pkg/core/src/atpg.cpp` — calSCOAP reset/guards, MUX implication
- `pkg/core/src/atpg.h` — MUX evaluateGoodVal
- `pkg/core/src/simulator.h` — MUX good/fault sim

---

## 5. Phase D + Scan-Protocol Update (2026-06-09)

Phase D PODEM fixes (atomic MUX2/compound gates) raised b03 from ~35% to **FC_scan ≈ 93%**. Scan-protocol metric aligns with industry practice (reset held inactive during ATPG):

| Metric | b03.v（auto scan protocol） | b03_reset_tie.v |
|--------|------------------------------|-----------------|
| **FC_scan** | **93.03%** | 92.97% |
| FC_raw (appendix) | 92.83% | 92.97% |
| AU_comb | 2 | 2 |
| TI_scan (reset) | 2 | 0 (no reset PI) |

**Residual AU:** `_157_` (AOI211) + `_159_/A3` (NOR4) — likely UD in frame-1 FSM context; not worth further PODEM patching.

**Docs:** [`docs/superpowers/plans/2026-06-09-scan-protocol-fc-metric.md`](../../docs/superpowers/plans/2026-06-09-scan-protocol-fc-metric.md), [`2026-06-09-phase-d-podem-fix.md`](../../docs/superpowers/plans/2026-06-09-phase-d-podem-fix.md).
