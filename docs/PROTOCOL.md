# TankFree serial/BLE line protocol (observed on real hardware)

ASCII, newline-terminated, 115200 on USB-CDC (and BLE Nordic-UART). Captured live
from a Tank-G running M5.71 (`installer/probe.py`). Documented here for
interoperability — our editor speaks it and our firmware targets a compatible
surface. Formats/ranges only (no vendor code). Replies end in `OK`/`ERR …`;
async broadcasts end their tag with `!`.

## Read (safe) commands
| Cmd | Returns |
|---|---|
| `PING` | `PONG tankg_fw M5.71 …` (version/build) |
| `INFO` | uptime, boots, `healthy=`, `fw_ok=`, guard/stack |
| `MEM` | `heap= pool= pool_used= arena= fxcap= …` (bytes) |
| `STAT` | audio/DSP telemetry; `blk=used/total` ticks = CPU load |
| `MODEL` | `'5. MESA DUAL REC' layers=23 channels=3 engine=A2L` |
| `PLIST` | `PRE <idx> … 'Name' … col=rrggbb` + `PLIST end current=N` |
| `CLIST` / `ILIST` | `CAP/IRS <idx> <size> 'Name' …` + `… slots=N` |
| `FXS` | current chain snapshot (below) |
| `FXT` | block catalog with every param + range (below) |
| `SNAPS` | `SNAPS <bitmask> current=N` (4 scenes) |
| `PSTAT` | preset store stats (`live= seq= glb=16/30 …`) |

## Chain snapshot — `FXS`
```
FX P0 id=0 gate on=1 p=-58,5,0,0,0,0,0 ticks=345 l=0 g=-1 fu=0 mem=0
FX Q0 id=1 ir   on=1 p=100,0,0,21,0,0,0 ticks=6798 … irlen=1024/48 mem=23760
FXS amp=1 on=1
FXS cpu=7602/28677 ticks (26%)   mem_fx=23760 B
```
- `FX <slot> id=<n> <type> on=<0|1> p=<csv params in native units> ticks=<cpu> mem=<B>`
- slot label = lane letter (`P`/`Q`/… ; two lanes = parallel) + position.
- **CPU budget = 28677 ticks/audio-block.** **FX memory arena ≈ 118 KB** (see RE §9b).

## Edit (write) commands — from the decrypted firmware strings
| Cmd | Format | Reply |
|---|---|---|
| set param | `FXP <slot> <param_idx> <value>` | `FXP ok <n>` / `ERR FXP …; kept <v>` |
| bypass | `FXE <slot> <0|1>` | / `ERR FXE …` |
| add block | `FXA <type>` | `FXA id=<id> …` |
| remove | `FXR <slot>` | |
| move | `FXM <…>` | `FXM ok…` |
| oversampling | `FXOS <slot> <tier>` | `FXOS ok id=<id> os=<n> lat=<..> cost=<..>` |
| IR assign | `FXIR <slot> <ir_slot>` | `FXIR ok <..> <..>` |
| parallel xover | `FXXO <lane> <a> <b> <c>` | |
| parallel mix | `FXMIX <lane> <a> <b> <c>` | |
| amp EQ | `AMPEQ volume=<> bass=<> mid=<> treble=<> presence=<> drive=<>` | |
| amp capture | `AMPX <slot> <n>` | / `ERR AMPX capture engine failed` |
| gains | `GAIN <in_dB> <out_dB>` ; `GSAVE` to persist | |
| preset | `PLOAD/PSAVE/PNEW/PDEL/PNAME/PCOLOR <idx> …` | |
| scenes | `SNAP <n>` ; `SNAPSAVE` ; `SNAPNAME` | |
| footswitch | `FSA/FSDO/FSPRESS/FSLED/FSMODE` | |
| capture/IR upload | `LDBEGIN/LDB/LDEND` ; `IRBEGIN/IRB/IREND` ; `CSAVE/ISAVE` | |

> `FXT  %d %s %d..%d (%d)` is the per-param descriptor: `<idx> <name> <min>..<max> (<default>)`.

## Block catalog — `FXT` (40 blocks, real cost in ticks; budget 28677)
IDs, category, cost, and params (name range default). Spanish param names are the
firmware's; our UI relabels. This is the engine's *interface*, a reference for our
clean-room blocks (see BLOCK_CATALOG.md), not its implementation.

```
1  gate      dynamics 490   : umbral_db -90..0(-70), release_ms 5..1000(120)
2  boost     drive    340   : ganancia_db -12..24(6)
3  overdrive drive    3545  : drive_db 0..40(18), tono_hz 1k..9k(4k), nivel_db -30..0(-8), os 0..3, limpio%
4  eq        eq       1795  : graves_db ±15, medios_db ±15, medios_hz 200..5000(800), agudos_db ±15
5  delay     delay    1940  : tiempo_ms 10..2000(350), realim% 0..95(35), mezcla%, amort%, sync 0..9, lowcut_hz, mono
7  ir        cab      6366  : mezcla%(100), nivel_db -30..12, ir_slot 0..8, largo_ms 0..85(21)
8  comp      dynamics 825   : umbral_db -50..0(-24), ratio_x10 10..200(40), ataque_ms, release_ms, nivel_db, mezcla%
9  ts9       drive    4715  : drive%, tono%, nivel_db -30..6, os, limpio%
10 ds1       dist     4755  : dist%, tono%, nivel_db, os, limpio%
11 chorus    mod      1795  : vel_chz 5..500(80), prof%, mezcla%, sync 0..12, lowcut_hz, mono
12 flanger   mod      2050  : vel_chz, prof%, realim% -90..90, mezcla%, sync, lowcut_hz
13 phaser    mod      2225  : vel_chz, prof%, realim% 0..90, sync, lowcut_hz
14 tremolo   mod      445   : vel_chz 50..1500, prof%, forma%, sync, estereo
15 vibrato   mod      1785  : vel_chz, prof%, sync, mono
16 klon      drive    4175  : ganancia%, agudos%, nivel_db, mezcla%, os
17 rat       dist     3875  : dist%, filtro%, nivel_db, os, limpio%
18 muff      dist     9415  : sustain%, tono%, nivel_db, os(3), limpio%
19 fuzzface  dist     7630  : fuzz%, bias%, tono%, nivel_db, os(3), limpio%, modelo 0..1
20 bd2       drive    4145  : drive%, tono%, nivel_db, os, limpio%
21 amp_clean amp      6685  : ganancia%, graves%, medios%, agudos%, presencia%, nivel_db, os
22 amp_brit  amp      7265  : (same 7 params)
23 amp_hi    amp      16785 : (same 7 params, os up to 3)
25 doubler   mod      1750  : retardo_ms 5..40, detune_ct ±20, mezcla%, nivel_db, estereo 0..2, ancho%, modo_st
26 hum       eq       1370  : mains_hz 50/60, depth_att 3..40, ancho_hz 1..20, armonicos_n 1..4
27 reverb2   reverb   8200  : tamano%, decay%, amort%, predelay_ms 0..100, mezcla%, nivel_db, mod%
28 sgate     dynamics 1065  : umbral_db -90..-20, rango_att 6..90, release_ms, hush%
29 pitchpro  mod/pitch 4450 : semitonos ±12, cents ±50, mezcla%, nivel_db, ventana_ms 10..27, ataque%, modo 0..2
30 spectral  reverb   5400  : decay%, amort%, difus%, mezcla%, nivel_db
31 shimmer   reverb   9900  : tamano%, decay%, amort%, intervalo 0..3, mezcla%, nivel_db, shimmer%
32 hybrid    reverb   9700  : tamano%, decay%, amort%, predelay_ms, mezcla%, nivel_db, er%
33 plate     reverb   7100  : tamano%, decay%, amort%, predelay_ms, mezcla%, nivel_db, mod%
34 spring    reverb   7700  : dwell%, decay%, tono%, resortes_n 1..3, mezcla%, nivel_db, drip%
35 cloud     reverb   8200  : tamano%, decay%, amort%, mod%, mezcla%, nivel_db, freeze 0..1
36 geq       eq       4105  : g50 g125 g315 g800 g2k g5k nivel (each ±12 dB)
37 basseq    eq       3455  : graves bajos medios altos agudos nivel (±15), geb7 bitmask
38 envfilter mod      1490  : sens%, frec_lo_hz 40..1000, frec_hi_hz 200..8000, reso%, decay%, filtro_modo 0..7, mezcla%
39 rotary    mod      6530  : velocidad 0..2, drive%, rapido_chz 300..1000, inercia%, bocina%, distancia%, mezcla%
```
(IDs 6 and 24 unused. `os` = oversampling tier 0/1/3 with per-tier cost.)
