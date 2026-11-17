# TankFree — Block Catalog (target)

The goal: the **most complete catalog we can** of amp + effect blocks, all from
open sources. The *library* can be huge (code fits the 1 MiB easily); the number
**active at once per preset** is bounded by the runtime CPU/MEM meters.

Firmware is **GPL-3.0**, so we can reuse GPL projects directly (not just
clean-room). CPU hints are at 48 kHz on the AC791N (pi32v2, ~396 MHz, FFT accel):
🟢 cheap · 🟡 moderate · 🔴 heavy. Trademark-safe names (model the circuit, not the brand).

> Sources being confirmed by research (exact repos/licenses/CPU) — this is the
> working taxonomy. Primary suites: **Guitarix** (GPL), **BYOD/chowdsp** (GPL),
> **chowdsp_wdf** (BSD), **FAUST faustlibraries**, **mda** (MIT), **Airwindows** (MIT).

## 1. Dynamics
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Noise gate / Smart gate | threshold, hysteresis, program-dependent release | 🟢 | Guitarix gate |
| Compressor — optical | smooth, LA-style | 🟢 | chowdsp / Guitarix |
| Compressor — FET (1176-style) | fast, punchy | 🟡 | chowdsp |
| Compressor — VCA/studio | ratio/attack/release/knee | 🟢 | mda, Guitarix |
| Boost / Clean boost | pre-gain, treble boost option | 🟢 | trivial |
| Limiter / Brickwall | output safety | 🟢 | — |
| Sustainer | comp+gate hybrid | 🟡 | — |

## 2. Drive / Distortion / Fuzz (circuit-modeled, real knobs)
| Block (generic) | Models | CPU | Open ref |
|---|---|---|---|
| Green Screamer | Tube Screamer TS808/TS9 | 🟡 | BYOD, Guitarix |
| Transparent OD | Klon, Blues Breaker, OCD | 🟡 | BYOD (WDF) |
| Rodent Dist | ProCo RAT | 🟡 | BYOD |
| Orange Dist | DS-1 | 🟡 | BYOD |
| Muff Fuzz | Big Muff Pi (variants) | 🟡 | BYOD (WDF Muff) |
| Face Fuzz | Fuzz Face (Ge/Si) | 🟡 | BYOD |
| Bender Fuzz | Tone Bender | 🟡 | — |
| Octave Fuzz | Octavia | 🟡 | — |
| Metal Dist / Hi-gain preamp | 800/5150-style front end | 🟡 | Guitarix |
| Bitcrusher / Waveshaper | lo-fi, decimate | 🟢 | mda, Airwindows |

## 3. EQ / Filter
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Graphic EQ (10-band) | guitar-tuned bands | 🟢 | Guitarix, CAPS |
| Parametric EQ (3-5 band) | freq/Q/gain | 🟢 | — |
| Wah (pedal) | cry-baby model, expr-controlled | 🟢 | Guitarix wah |
| Auto-wah / Envelope filter | env-driven sweep | 🟢 | Guitarix |
| Hum / Noise filter | mains hum + spurious tone | 🟢 | Guitarix humremover |
| Formant / Talk | vowel filter | 🟡 | — |
| High/Low-pass, Tilt | shaping utilities | 🟢 | — |

## 4. Modulation
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Chorus | CE-2 / dimension, multi-voice | 🟢 | Guitarix, mda |
| Flanger | through-zero option | 🟢 | Guitarix, mda |
| Phaser | 4/8/12-stage selectable | 🟢 | Guitarix, mda |
| Tremolo | bias / optical / harmonic | 🟢 | Guitarix |
| Vibrato | pitch-based | 🟢 | — |
| Rotary / Leslie | horn+drum, speed ramp | 🟡 | Guitarix rotary |
| Univibe | 4-stage photocell | 🟡 | — |
| Ring modulator | carrier osc | 🟢 | mda |
| Auto-pan / Stereo | LFO pan (phones stereo) | 🟢 | — |

## 5. Pitch / Synth
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Pitch shifter (mono) | ±octaves, fixed interval | 🟡 | SoundTouch (LGPL) |
| Pitch shifter Pro (poly/HQ) | phase-vocoder, FFT accel | 🔴 | Rubber Band (GPL) / PV |
| Intelligent harmonizer | scale/key-aware | 🔴 | — |
| Octaver | sub + up, POG-style | 🟡 | — |
| Detune / Doubler | subtle widening | 🟡 | — |
| Whammy | expr-controlled pitch | 🔴 | — |

## 6. Delay / Time
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Digital delay | clean, tempo-sync | 🟢 | trivial + mem |
| Analog / BBD delay | dark repeats, mod | 🟢 | Guitarix echo |
| Tape delay | wow/flutter, saturation | 🟡 | — |
| Ping-pong | stereo (phones) | 🟢 | — |
| Multi-tap | rhythmic | 🟡 | — |
| Reverse delay | — | 🟡 | — |
| Modulated delay | chorus-delay | 🟢 | — |
| Looper | if RAM allows | 🟡 | — (memory-bound) |

## 7. Reverb (algorithmic + convolution)
| Block | Notes | CPU | Open ref |
|---|---|---|---|
| Room / Early reflections | — | 🟡 | Freeverb, Faust re. |
| Hall | long, lush | 🟡 | zita-rev1 (GPL), jpverb |
| Plate | bright, dense | 🟡 | Dattorro |
| Spring | drip, amp-style | 🟡 | Faust/Guitarix spring |
| Shimmer | pitch-up feedback | 🔴 | greyhole + PV |
| Spectral / Freeze | FFT pad (accel) | 🔴 | — |
| Modulated / Reverse / Gated | variants | 🟡 | — |
| Convolution reverb | IR-based (like cab) | 🟡 | FFT accel |

## 8. Amps (see SPEC.md "Amps")
- **Capture engine**: NAM A2-Lite + factory captures.
- **Algorithmic engine**: one parametric core + a table of classic-amp definitions
  (Tweed/Bassman, Blackface, Plexi/Brit-800/900, AC30, Rectifier/Mark, SLO, 5150,
  JC-120…). Ported from Guitarix/BYOD; tone stacks from Duncan values; Yeh thesis math.

## 9. Cabs / Utility
- **Cab = IR convolution** (user `.wav` + the 8 factory IRs: JVM_1960_57, MESA_412,
  VOX_AC30, V30_MD421, EGNL_412, WANGS112, HIFI_YAMAHA…). FFT-accelerated.
- Utility: volume/expression, tuner, splitter/mixer (parallel branches), global EQ, DI.

## Implementation strategy
1. Pick a DSP source per block (prefer **FAUST** → compiles to plain C we can drop in,
   or port **Guitarix/BYOD** C++). 2. Wrap each in a common block interface
   (process(buf), params, CPU/MEM cost). 3. Register in the free-chain engine.
   4. Expose params to the manager UI + the GP-5 MIDI category map.
