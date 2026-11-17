# FAUST amp/effect sources → embeddable C

We author amps and some effects in [FAUST](https://faust.grame.fr) and compile
them to **plain C** that drops into the firmware as `tf_block`s.

## Why FAUST
- `faust -lang c` emits one self-contained C99 file: a `struct` + `init*()` +
  `compute*()` — **no heap, no filesystem, only `math.h`**.
- `tonestacks.lib` has real circuit-matched tone stacks (`jcm800`, `bassman`,
  `ac30`, …); the generated C is ours to license under GPL-3 (FAUST output exception).

## Build
```bash
# install faust (brew install faust / apt install faust), then:
faust -lang c -os -cn brit800 brit800.dsp -o ../src/blocks/gen/brit800.c
```
- `-os` = one-sample mode: splits knob math (`control()`) from the audio loop so
  coefficients recompute per-block, not per-sample.
- `-cn <name>` gives each amp a unique struct/symbol name (compile many into one app).
- Default is single-precision float (`-single`).

## Wrapping
Each generated `*.c` is wrapped by a tiny `tf_block` adapter (see
`../src/blocks/amp_brit800.c`, TODO) that maps `tf_param`s → the FAUST controls
and calls `compute()` on each audio frame. Feed a cab IR after the amp.

## Amps to generate (swap the tonestack + voicing, near-zero marginal code)
`brit800` (JCM800) · `plexi`/`jtm45` · `tweed`/`bassman` · `blackface` · `ac30`
· `rectifier` · `slo` · `5150` · `jc120`. See `../../docs/AMP_MODELING.md`.
