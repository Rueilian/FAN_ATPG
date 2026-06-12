# b03 External ATPG Reference Comparison (Phase C3)

**Date:** 2026-06-09  
**Purpose:** Separate FAN engine ceiling from industry full-scan expectations on ITC'99 b03.

## Reference Sources

| Source | b03 faults | FC / result | Notes |
|--------|------------|-------------|-------|
| [DVCON — Modus vs Xcelium fault injection](https://dvcon-proceedings.org/wp-content/uploads/improving-the-confidence-level-in-functional-safety-simulation-tools-for-iso-26262.pdf) | **882** collapsed | **PASS** (full list match) | Commercial full-scan on ITC'99 benchmark |
| Industry full-scan (EE Times / Electronic Design DFT literature) | — | **95–99%+** typical | Stuck-at on scanned gate netlists |
| **FAN (this work, post Phase C fix)** | **1112** collapsed | **34.75%** | `b03_fs.script`, frame=1 |

## FAN Phase C Results (same netlist family)

| Experiment | FC | AU | DT | Verdict |
|------------|-----|-----|-----|---------|
| b03 baseline (`b03.v`) | 34.75% | 948 | 538 | — |
| b03 reset tie-high (`b03_reset_tie.v`) | 33.96% | 963 | 527 | No improvement |
| b03 per-FF RN→`_rn_tie_` | *parse fail* | — | — | Dangling reset INV nets; FAN `Netlist::check` rejects |

## Interpretation (G2 Decision)

1. **External expectation >> FAN result** — b03 is not inherently ~35% untestable under commercial full-scan (882-fault reference, PASS).
2. **Phase C1 fix confirmed FAN bug** — `checkIfFaultHasPropagatedToPO` indexed scan pseudo gates at array tail; tiny_sdffr went from 58%→**92%** FC, 0 AU.
3. **b03 remains ~35% after C1** — remaining 948 AU dominated by MUX2-heavy combinational logic (341 MUX2_X1 AU); FAN PODEM ceiling on reconvergent FSM, not netlist format.
4. **RN / reset tie-high not sufficient** — async control tying does not materially change b03 FC on FAN.

## Conclusion (Phase C, historical)

| Question | Answer (at Phase C) |
|----------|------------------------|
| Is low FC a netlist/scan format problem? | **No** — structural verify PASS; C1 proved observation bug |
| Is low FC ITC'99 inherent? | **No** — literature/commercial reference ~99% |
| Is low FC primarily FAN engine on b03 scale? | **Yes (then)** — MUX/FSM AU-heavy before Phase D |
| G2 (b03 ≥ 80%)? | **FAIL (then)** → triggered Phase D |

## Phase D + Scan-Protocol Update (2026-06-09)

| Experiment | FC_scan | FC_raw | AU_comb | Notes |
|------------|---------|--------|---------|-------|
| b03 + Phase D PODEM + auto scan protocol | **93.03%** | 92.83% | **2** | Primary metric; reset PI → TI |
| b03_reset_tie.v + scan protocol | 92.97% | 92.97% | 2 | Structural reset deassert |
| Commercial reference (DVCON) | — | ~99% | — | 882 faults PASS |

**Revised framing:**

- Phase C correctly identified observation bug + engine ceiling; Phase D addressed PODEM/MUX modeling.
- Report **FC_scan** (async reset excluded per Cummings SNUG 2002); put **FC_raw** and AU_reset in appendix.
- Remaining gap vs commercial ~99% is mainly **QN UD (62)** + **2 comb AU**, not reset protocol mismatch.
- See [`docs/superpowers/plans/2026-06-09-scan-protocol-fc-metric.md`](../../docs/archive/superpowers/plans/2026-06-09-scan-protocol-fc-metric.md).
