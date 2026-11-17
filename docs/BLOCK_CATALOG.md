# TankFree — Block Catalog (open-source sources, concrete)

The most complete catalog we can, all from open code. The *library* can be huge
(fits 1 MiB); the number **active per preset** is CPU/MEM-bounded (meters enforce).

Firmware is **GPL-3.0** → we reuse GPL directly (+ permissive BSD/MIT). CPU at 48 kHz
on AC791N (~396 MHz = **~8,000 cyc/sample**; FPU, **no HW transcendentals**; FFT accel):
🟢 cheap · 🟡 moderate · 🔴 heavy. Trademark-safe names (model the circuit, not the brand).

## Reuse strategy (priority order)
1. **FAUST `-lang c`** — `faust -lang c foo.dsp -o foo.c` emits one self-contained,
   heap-free C file (struct + init + compute). Compiler is GPL but **generated C is
   ours to relicense**. Most DSP below exists as FAUST. Primary path.
2. **Direct lift** of MIT/BSD per-sample C/C++ (Airwindows, mda, chowdsp_wdf, Freeverb).
3. **Extract DSP cores** from GPL suites (strip LV2/JUCE wrapper).

## Suites to harvest
| Suite | License | What it gives | URL |
|---|---|---|---|
| **FAUST faustlibraries** ⭐ | LGPL-2.1+ w/ output exception (generated C relicensable) | everything: `re.`(reverb) `pf.`(phase/flange) `ve.`(va) `co.`(comp) `ef.`(misc) `fi.`(filter) `de.`(delay) + `tonestacks.lib` `tubes.lib` | github.com/grame-cncm/faustlibraries |
| **Guitarix** ⭐ | GPL-2-or-later | ~200 guitar `.dsp` in `trunk/src/faust/`: amps, TS9/DS1/MXR/Muff/FuzzFace, chorus/flanger/phaser/tremolo/vibe/ring, delays, freeverb, dattorro, comp/gate, EQ, big wah set, tuner | github.com/brummer10/guitarix |
| **GxPlugins.lv2** | GPL-3.0 | 50+ pedals: SD1, Guvnor, Fuzz Face (AxisFace/SunFace), SuperFuzz(oct), Tonebender, Quack(autowah), SlowGear | github.com/brummer10/GxPlugins.lv2 |
| **BYOD / chowdsp** | GPL-3.0 | WDF/nodal circuit pedals in `src/processors/{drive,modulation,tone}/` | github.com/Chowdhury-DSP/BYOD |
| **chowdsp_wdf** ⭐ | **BSD-3** | header-only WDF engine (no alloc/JUCE); best analog-pedal route | github.com/Chowdhury-DSP/chowdsp_wdf |
| **Airwindows** | **MIT** | zero-dep per-sample C++: Density/Drive (sat), Pressure (comp), Gatelope, Galactic/Verbity (reverb), ChorusEnsemble, Vibrato, TapeDelay | github.com/airwindows/airwindows |
| **mda** | MIT / GPL-2+ | tiny plain: Overdrive, Combo, **Leslie**, RingMod, Detune, Chorus, ThruZero Flanger, Ambience, Dynamics | github.com/drobilla/mda-lv2 |
| **CAPS** | GPL (v2→v3) | ToneStack/AmpVTS (Yeh), Plate reverb, Eq10/4p, Compress, Saturate, Noisegate | quitte.de/dsp/caps.html |
| **ZAM-plugins** | GPL-2+ | ZamComp, **ZamChild670** (Fairchild vari-mu), **ZaMaximX2** (limiter), ZamGate, ZamEQ2, ZamGEQ31 | github.com/zamaudio/zam-plugins |
| **Dragonfly / Freeverb3** | GPL-3 (Freeverb3 GPL-2+) | `common/freeverb/`: nrev/strev (plate), zrev (hall), progenitor (room), earlyref, irmodel (FFT conv) | github.com/michaelwillis/dragonfly-reverb |
| **TAP / Invada / Rakarrack** | GPL-2 (verify per-file) | plain-C FX (echo, chorus, tremolo, rotspeak, pitch). GPL-2-only → reimplement if strict | tap-plugins / invada / rakarrack-plus |
| ⚠️ **TheOmegaCake** | **AGPL-3** | 45 fuzz solvers — network-copyleft, **contaminates product** → reference only |
| ⚠️ **RackAFX / Will Pirkle** | proprietary | **avoid** in GPL firmware |

## 1. Dynamics
| Block | Source | CPU |
|---|---|---|
| Noise/smart gate | FAUST `ef.gate_mono` (hold=hysteresis); ZamGate; Airwindows Gatelope | 🟢 |
| Compressor (VCA) | FAUST `co.compressor_mono`; mda Dynamics; CTAGDRC `Source/dsp/*` (GPL-3) | 🟢 |
| Compressor (opto/vari-mu) | ZamChild670 (Fairchild); chowdsp_compressor refs | 🟡 |
| Limiter | FAUST `co.limiter_1176_R4` (FET); lookahead `co.limiter_lad_*` (+latency) | 🟢/🟡 |
| Boost / Sustainer | Guitarix gxbooster; high-ratio `co.compressor` | 🟢 |

## 2. Drive / Distortion / Fuzz (two tiers: Guitarix-Faust cheap, BYOD-WDF accurate)
| Generic | Model | Source | CPU |
|---|---|---|---|
| Green Screamer | Tube Screamer | BYOD `drive/tube_screamer/*WDF.h`; Guitarix `gxts9` | 🟡/🟢 |
| Rodent | ProCo RAT | BYOD `drive/mouse_drive/*WDF.h` (LM308); Guitarix GX Rat | 🟡/🟢 |
| Orange Dist | DS-1 | Guitarix `gx_bossds1`+`bossds1.dsp` (best open) | 🟢 |
| Muff Fuzz | Big Muff | BYOD `drive/big_muff/*`; Guitarix `gx_muff`+`bigmuff.dsp` | 🟡/🟢 |
| Face Fuzz | Fuzz Face | BYOD `drive/fuzz_machine/FuzzFaceNDK.cpp` (nodal); GxPlugins GxSunFace | 🟡 |
| Transparent OD | Klon / KoT / OCD-ish | BYOD `drive/centaur/`, `king_of_tone/`, `zen_drive/` | 🟡 |
| Metal/MXR Dist+ | — | BYOD `drive/mxr_distortion/*WDF.h`; Guitarix `gx_mxrdist` | 🟡/🟢 |
| Bender / Octave | Tonebender / Octavia | GxPlugins Tonebender / GxSuperFuzz | 🟢 |
| Bitcrush/Waveshape | — | mda, Airwindows Density/Drive/Spiral | 🟢 |

> BYOD `DiodePairT` uses closed-form Lambert-W (`omega4`) → no Newton loop (stays 🟡).
> Budget **2–4× oversampling** on the nonlinearity (tone stacks stay at 48 kHz).
> Neural captures (GuitarML + RTNeural BSD-3) = 🔴, one at a time, FFT accel doesn't help.

## 3. EQ / Filter — foundation: RBJ Audio EQ Cookbook (public domain), one biquad/band 🟢
Graphic EQ (FAUST `fi.graphic_eq`, CAPS Eq10, ZamGEQ31) · Parametric (cookbook /
`fi.peak_eq`/shelves) · Wah (FAUST `ve.crybaby`, Guitarix `gx_gcb_95` + voxwah/foxwah…) ·
Auto-wah/env filter (`ve.autowah`) · Hum/noise filter (`fi.notchw` 50/60 Hz + `fi.dcblocker`) ·
Formant (`vocal.lib`). All 🟢 (formant 🟡).

## 4. Modulation — LFO + modulated delay/allpass, all 🟢
Chorus (Guitarix `chorus.dsp`, mda, Airwindows ChorusEnsemble) · Flanger (`pf.flanger`,
mda ThruZero) · Phaser (`ve.mxrPhase90`, `pf.phaser2`, BYOD Phase90/Phaser8) ·
Tremolo bias/harmonic (Guitarix, BYOD) · Vibrato (`pf.vibrato2`, BYOD scanner_vibrato) ·
**Rotary/Leslie** (mda Leslie = best lift; BYOD Rotary; tap_rotspeak) · Univibe
(Guitarix vibe_lfo, BYOD uni_vibe) · Ring mod (mda RingMod; `sig*osc`) · Auto-pan (equal-power).

## 5. Pitch — pick by quality (FFT accel helps the PV path)
| Block | Source | CPU |
|---|---|---|
| Whammy / detune / doubler | FAUST `ef.transpose`/granular (drive `s` from expr) | 🟢/🟡 |
| Mono pitch / harmonizer (HQ) | **smbPitchShift** (Bernsee, PD) — 1 FFT/hop, MCU-proven; uses FFT accel | 🟡 |
| Harmonizer (scale-aware) | Rakarrack `Harmonizer.C` (algo ref) over smbPitchShift | 🔴 |
| Time/pitch lib | **SoundTouch** (LGPL, WSOLA, no FFT); Rubber Band (GPL/commercial, 🔴) | 🟡 |
| Pitch detect (track/tuner) | **aubio** `pitchyinfft.c` (GPL-3, FFT accel) | 🟡 |
| Octave | sub = zero-cross divider (≈free); up = rectify; poly POG = 🔴 out of budget | 🟢 |

## 6. Delay — RAM is the limit (mono float ≈192 KB/s → use int16, or cap time)
FAUST `delays.lib`: `de.fdelay`/`de.fdelayltv` (fractional, for modulated/tape movement),
`de.sdelay` (click-free tempo change), `de.multiTapSincDelay`. Reverse = read backward;
ping-pong = cross-feedback. 🟢/🟡. **Analog/BBD:** `jatinchowdhury18/BBDDelay` (**BSD-3**,
companding + charge-transfer nonlin). **Tape:** CHOW AnalogTapeModel (GPL-3; use its
`HysteresisSTN` neural-speedup path to skip the Newton solver), 🟡–🔴.

## 7. Reverb (algorithmic + convolution)
| Block | Source | CPU |
|---|---|---|
| Default/room | **Freeverb** (Jezar, **PD**) / FAUST `re.stereo_freeverb`; `re.jcrev` (Schroeder) | 🟢 |
| Plate | **el-visio/dattorro-verb** (**MIT, plain C**) or **MVerb** (GPL-3, single header) | 🟡 |
| Hall | **zita-rev1** (GPL-2+) / FAUST `re.zita_rev1_stereo` | 🔴 |
| Spring | FAUST `re.springreverb` (RAM-light) or modal resonator bank | 🟡 |
| Shimmer | cheap reverb + (+12) pitch in feedback (Freeverb + smbPitchShift) | 🔴 |
| Spectral/freeze/gated | FFT frame capture+sustain (**FFT accel**); gated = reverb + fast gate | 🟡/🔴 |
| **Convolution** (cab too) | **FFTConvolver** (HiFi-LoFi, **MIT**) — `TwoStageFFTConvolver`, swap `AudioFFT` for the AC791N accelerator | 🟡 |

## 8. Utility
Volume/expr/DI = gain + `si.smoo` (🟢). **Tuner:** `dywapitchtrack` (**MIT, pure C**, best
embedded) or aubio yinfft (FFT accel). Splitter/mixer for parallel chains = add + equal-power
pan (🟢). Global EQ = output biquad bank.

## 9. Amps / Cabs
Amps: see [`AMP_MODELING.md`](AMP_MODELING.md) (FAUST `tonestacks.lib` + triode stages +
NFB presence) and [`SPEC.md`](SPEC.md) (NAM capture engine). **Cab = IR convolution**
(FFTConvolver on the accelerator) with user `.wav` + the 8 factory IRs.

## Build order (for the AC791N)
1. **Foundation (all 🟢):** biquad EQ/filter bank (cookbook) + volume/DI/splitter + LFO
   engine → unlocks EQ, wah, auto-wah, hum, chorus/flanger/phaser/tremolo/vibrato/univibe/
   ring/auto-pan + tuner.
2. **Drive two-tier:** Guitarix/GxPlugins Faust (broad roster, 🟢) + BYOD WDF (marquee
   TS/RAT/Muff/MXR/Klon/FuzzFace, 🟡, oversample ×2–4).
3. **Dynamics:** FAUST `co.*` + `co.limiter_1176` + ZamChild670.
4. **Time:** `delays.lib` (int16 storage) + BBDDelay (analog) + Freeverb/Dattorro/MVerb.
5. **FFT accelerator for:** convolution reverb/cab, yinfft tuner, smbPitchShift pitch.
6. **Avoid on-chip:** poly pitch, Rubber Band, multiple neural captures, full CHOW Tape.

## License cheat-sheet
- **Anywhere (MIT/BSD/ISC/PD):** FAUST-generated C, chowdsp_wdf, RTNeural, Airwindows,
  mda, Freeverb, dattorro-verb, FFTConvolver, dywapitchtrack, smbPitchShift, EQ Cookbook, BBDDelay.
- **GPL-3 (= ours):** BYOD, GxPlugins, GuitarML, CHOW Tape, Dragonfly, MVerb.
- **GPL-2-or-later → GPL-3 OK:** Guitarix, aubio, ZAM, CAPS.
- **Verify per-file / GPL-2-only risk → reimplement if strict:** Rakarrack, TAP, Invada.
- **Traps:** TheOmegaCake = AGPL-3 (contaminates); circuit-decimator = no license (ref only);
  Rubber Band GPL arm needs commercial license if ever shipped non-GPL.
