# TankFree — Algorithmic Amp Engine (implementable reference)

How we build circuit-behaving amps (real Gain / Bass-Mid-Treble / Presence /
Master) on the AC791N. One parametric engine; each amp = a parameter set.

Budget: 396 MHz / 48 kHz ≈ **8,250 cycles/sample**. FPU yes, **no HW
transcendentals** (`tanhf/expf/powf` are software polys — the real cost driver,
not the IIR). 2–3 amps fit comfortably if coefficients are cached and saturators
use polynomial approximations; reserve 4× oversampling for the most-distorted model.

## Signal path (per amp)
```
in → HPF(coupling) → [gain stage ×2–3] → TONE STACK → presence(NFB shelf) → power-amp → master → cab(IR)
```

## 1. Tone stack — the win: FAUST `tonestacks.lib`
`faustlibraries/tonestacks.lib` has explicit `jcm800`, `jcm2000`, `jtm45`,
`bassman`, `ac30`, Soldano, Mesa… derived from David Yeh's CCRMA analysis.
**LGPL-2.1+ with the FAUST output exception → the generated C is ours to GPL-3.**

Compile one amp's stack to plain C:
```bash
faust -lang c -os -cn jcm800 jcm800.dsp -o jcm800.c
# emits struct jcm800 + initjcm800()/computejcm800(); params = Bass/Mid/Treble 0..1
```

### JCM800 component values (Yeh naming; matches tonestacks.lib)
| | R1(Treb) | R2(Bass) | R3(Mid) | R4(slope) | C1(treb) | C2(bass) | C3(mid) |
|---|---|---|---|---|---|---|---|
| **JCM800** | 220k | 1M | 22k | 33k | 470pF | 22nF | 22nF |
| JCM2000 | 250k | 1M | 25k | 56k | 500pF | 22nF | 22nF |
| '59 Bassman (Fender) | 250k | 1M | 25k | 56k | 250pF | 20nF | 20nF |

Marshall & Fender stacks are the **same 3rd-order passive RC topology** — swap
values to change amp. Transfer function `H(s)=(b1 s+b2 s²+b3 s³)/(a0+a1 s+a2 s²+a3 s³)`
(a0=1); full symbolic coeffs in Yeh & Smith DAFx-06, or verbatim from `tonestacks.lib`.
Denominator has **no Treble dependence** (Treble moves zeros only).

### Discretization & live knobs
- Bilinear `s=c(1−z⁻¹)/(1+z⁻¹)`, `c=2·fs=96000` (un-prewarped per Yeh) → 3rd-order TDF-II.
- Filtering = **7 mul + 6 add/sample** (<1% CPU). Recompute coeffs **per-block, gated
  on knob movement** (~100–200 flops + 1 divide, negligible).
- **Compute coeffs in `double`** (terms span `c³≈8.8e14` vs `RC≈1e-20`), or use the
  FAUST trapezoidal observable-form rewrite (faustlibraries PR #307) to stay float-safe.
- Smooth the **three knob inputs** (~10 ms one-pole), recompute from smoothed values →
  no zipper noise and stays on the stable/realizable locus.

## 2. Gain stages (triode preamp)
Per 12AX7 half (standard VA decomposition):
1. Input HPF (coupling): `H(s)=sRgCc/(1+sRgCc)`, Rg≈1M, Cc≈22nF → ~7 Hz.
2. Nonlinearity — pick by budget:
   - **Cheap:** `misceffects.lib::cubicnl` or a tanh/poly waveshaper (oversample 4–8×).
   - **Accurate:** Koren triode `Ip=(E1^X/Kg1)(1+sgn E1)`, `E1=(Ep/Kp)log(1+exp(Kp(1/µ+Eg/√(Kvb+Ep²))))`;
     12AX7: µ=100, X=1.4, Kg1=1060, Kp=600, Kvb=300. (No grid branch → cheaper.)
   - **Most accurate:** Dempwolf C¹ triode (adds grid-conduction/blocking distortion,
     good for cranked-Marshall bias shift). Fitted 12AX7 params in DAFx-11 paper.
3. Plate-load gain `≈ −µ·Rp/(Rp+rp)`, Rp≈100k.
4. **Cathode-bypass shelf** (bass-dependent gain): `A(s)=−gm·Rp/(1+gm·Zk(s))`,
   `Zk(s)=Rk/(1+sRkCk)`, Rk≈820Ω, Ck≈0.68–22µF (voicing knob).
5. Output coupling HPF into next stage. Bilinear-discretize all linear parts.

Cascade 2–3 stages for Gain. Alternative accurate route: WDF triode via
**chowdsp_wdf** (BSD-3, header-only, no-alloc).

## 3. Presence (negative feedback, NOT a forward EQ)
Presence reduces frequency-dependent global NFB from the output-transformer secondary.
Model closed loop `Hcl(s)=A/(1+A·β(s))` with `β(s)=β0/(1+s·Rpres·Cpres)` (first-order
LPF). Collapses to a cheap **first-order high-shelf** after the power-amp block.
Cpres ~4.7nF–0.1µF, presence pot ~5k. Bilinear-discretize.

## 4. Power amp + master
Power-amp voicing = light saturation + output-transformer HF/LF rolloff (first-order
shelves). Master = output gain (smoothed).

## Amp definitions to ship (trademark-safe names)
`tonestacks.lib` + voicing params give, at near-zero marginal code each:
Tweed/Bassman, Blackface (Fender), **Brit-800 (JCM800)**, Plexi/JTM45, Brit-2000,
AC30 (Vox), Rectifier/Mark (Mesa), SLO (Soldano), 5150 (Peavey), JC-120 (Roland).

## Code sources (all GPL-3-compatible)
- **FAUST `tonestacks.lib` / `tubes.lib`** (LGPL-2.1+, output relicensable) — primary.
- **Guitarix** `trunk/src/faust/` (GPL-2-or-later → GPL-3 OK) — modular tube amp .dsp.
- **BYOD** `src/processors/{drive,tone}/` (GPL-3) — WDF `tube_amp`, netlist models.
- **chowdsp_wdf** (BSD-3) — WDF engine (triode preamp + stack).
- **CAPS "AmpVTS"/ToneStack** (GPL-3, lean C++, Yeh math) — best hand-written-C++ route.
- ❌ Avoid **RackAFX / Will Pirkle** (proprietary).

## Theory refs
- Yeh CCRMA thesis (2009); Yeh & Smith "'59 Bassman Tone Stack" DAFx-06;
  Yeh/Abel/Smith nodal-DK IEEE TASLP 2010.
- Dempwolf & Zölzer triode DAFx-11; Koren SPICE tube models.
- Zölzer, *DAFX* 2nd ed. Ch.5 (nonlinear) + Ch.12 (virtual analog/WDF).
- Duncan Amps Tone Stack Calculator (component/values reference).
