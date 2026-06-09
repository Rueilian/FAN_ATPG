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

## Conclusion

| Question | Answer |
|----------|--------|
| Is low FC a netlist/scan format problem? | **No** — structural verify PASS; C1 proved observation bug |
| Is low FC ITC'99 inherent? | **No** — literature/commercial reference ~99% |
| Is low FC primarily FAN engine on b03 scale? | **Yes** — tiny circuits fixed; b03 MUX/FSM still AU-heavy |
| G2 (b03 ≥ 80%)? | **FAIL** → proceed with **fallback** partial-scan narrative |

## Recommended Report Framing

- **Positive:** Phase C fix + tiny_sdffr/s510 regression proves FAN can work on full-scan when observation path is correct.
- **Negative (honest):** FAN on b03 ITC'99 netlist hits ~35% FC vs industry ~99%; progressive residual gains on AU-dominated residual are expected to be small.
- **Do not claim:** "ITC'99 full-scan ceiling is 35%" — evidence contradicts this.
