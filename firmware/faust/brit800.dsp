// brit800.dsp — "Brit 800" algorithmic amp (JCM800-style), trademark-safe name.
// Circuit-behaving controls via FAUST tonestacks.lib. Compile to plain C:
//   faust -lang c -os -cn brit800 brit800.dsp -o brit800.c
// Then wrap it in a tf_block (blocks/amp_brit800.c) and feed a cab IR after it.
//
// Sources: tonestacks.lib (LGPL-2.1+, output relicensable to our GPL-3),
// Yeh CCRMA tone-stack analysis. See ../../docs/AMP_MODELING.md.

import("stdfaust.lib");

// --- user controls (map these to tf_params / GP-5 CCs) ---
gain     = hslider("gain[style:knob]",     0.5, 0, 1, 0.001);
bass     = hslider("bass[style:knob]",     0.5, 0, 1, 0.001);
mid      = hslider("mid[style:knob]",      0.5, 0, 1, 0.001);
treble   = hslider("treble[style:knob]",   0.5, 0, 1, 0.001);
presence = hslider("presence[style:knob]", 0.3, 0, 1, 0.001);
master   = hslider("master[style:knob]",   0.6, 0, 1, 0.001);

// --- input coupling HPF (~7 Hz grid-leak) ---
inhpf = fi.highpass(1, 7.0);

// --- preamp: cascaded saturating gain stages (cubic soft-clip; swap for a
//     Koren/Dempwolf triode or a chowdsp_wdf stage for more accuracy) ---
drive  = 1.0 + gain*60.0;                       // Gain knob -> preamp gain
stage  = *(drive) : ma.tanh : *(1.0/drive*2.0); // one saturating stage
preamp = stage : fi.highpass(1, 30) : stage;    // 2 stages w/ inter-stage coupling

// --- tone stack: real JCM800 topology from tonestacks.lib (Bass/Mid/Treble) ---
tonestack = ve.tonestack_jcm800(bass, mid, treble);   // see tonestacks.lib

// --- presence: NFB high-shelf (treble lift by dumping HF from the feedback) ---
presshelf = fi.highshelf(1, presence*12.0, 2000.0);

// --- power amp: gentle sat + output master ---
poweramp = ma.tanh : *(0.3 + master*1.2);

process = inhpf : preamp : tonestack : presshelf : poweramp <: _, _;  // mono -> L,R
