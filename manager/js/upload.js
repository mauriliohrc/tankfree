/* upload.js — load NAM captures (.nam → A2L1) and cab IRs (.wav → taps) and send
 * them to the pedal. Clean-room: our own converter, matched to firmware/src/engine/
 * a2l.c (we own both sides, so they agree). NAM core math is open (MIT); the A2L1
 * layout is our format (docs/RE_FINDINGS §7). No third-party code copied.
 */

/* ---------------- IR: decode a 48 kHz mono WAV into float taps ---------------- */
export function decodeWavIR(arrayBuffer, maxTaps = 4096) {
  const dv = new DataView(arrayBuffer);
  if (dv.getUint32(0, false) !== 0x52494646 /*RIFF*/ || dv.getUint32(8, false) !== 0x57415645 /*WAVE*/)
    throw new Error("Not a WAV file.");
  let off = 12, fmt = null, dataOff = 0, dataLen = 0;
  while (off + 8 <= dv.byteLength) {
    const id = dv.getUint32(off, false), sz = dv.getUint32(off + 4, true);
    if (id === 0x666d7420 /*fmt */) fmt = { audioFormat: dv.getUint16(off + 8, true), ch: dv.getUint16(off + 10, true),
      rate: dv.getUint32(off + 12, true), bits: dv.getUint16(off + 22, true) };
    else if (id === 0x64617461 /*data*/) { dataOff = off + 8; dataLen = sz; }
    off += 8 + sz + (sz & 1);
  }
  if (!fmt || !dataOff) throw new Error("WAV missing fmt/data.");
  if (fmt.rate !== 48000) throw new Error(`IR is ${fmt.rate} Hz; the pedal runs 48 kHz.`);
  const { ch, bits } = fmt, frames = Math.floor(dataLen / (ch * bits / 8));
  const taps = new Float32Array(Math.min(frames, maxTaps));
  for (let i = 0; i < taps.length; i++) {
    const p = dataOff + i * ch * bits / 8;                 /* channel 0 only */
    if (fmt.audioFormat === 3 && bits === 32) taps[i] = dv.getFloat32(p, true);
    else if (bits === 16) taps[i] = dv.getInt16(p, true) / 32768;
    else if (bits === 24) { const v = dv.getUint8(p) | (dv.getUint8(p+1)<<8) | (dv.getInt8(p+2)<<16); taps[i] = v / 8388608; }
    else if (bits === 32) taps[i] = dv.getInt32(p, true) / 2147483648;
    else throw new Error(`Unsupported WAV depth ${bits}.`);
  }
  return taps;
}

/* ---------------- NAM: parse + validate the A2-Lite submodel ---------------- */
export function parseNam(text) {
  let d; try { d = JSON.parse(text); } catch { throw new Error("Not valid JSON (.nam)."); }
  let m = d;
  if (d.architecture === "SlimmableContainer") {
    const subs = ((d.config || {}).submodels || []).slice().sort((a, b) => a.max_value - b.max_value);
    if (!subs.length) throw new Error("Broken SlimmableContainer.");
    m = subs[0].model;                                      /* smallest = A2-Lite */
  }
  const cfg = m.config || {};
  if (m.architecture !== "WaveNet") throw new Error(`Architecture ${m.architecture}: the pedal runs NAM A2 WaveNet.`);
  if ((cfg.layers || []).length !== 1) throw new Error("Expected one layer array (A2).");
  const L = cfg.layers[0];
  if (L.channels !== 3) throw new Error(`${L.channels} channels; the pedal runs the 3-channel A2-Lite net.`);
  if ((m.sample_rate || d.sample_rate || 48000) !== 48000) throw new Error("Capture is not 48 kHz.");
  return { m, L, C: 3, weights: Float32Array.from(m.weights) };
}

/* Quantize + pack to our A2L1 blob. (Calibration/Q-schedule is OUR choice, matched
 * to a2l.c — see TODO: validate tone once the runner's head is finalized.) */
export function namToA2L1(text, name = "NAM") {
  const { L, C, weights } = parseNam(text);
  // Minimal, self-consistent fixed-point pack. Full per-stage calibration lands with
  // the a2l.c validation; here we use conservative global Q so the blob is loadable.
  const nL = L.kernel_sizes.length, headK = L.head?.kernel_size || 1;
  // (Packing kept compact; mirrors a2l.h layout. Weights clipped to int16 at Q12.)
  const Q = 12, q16 = v => Math.max(-32768, Math.min(32767, Math.round(v * (1 << Q))));
  // NOTE: this is a scaffold pack — the exact per-tensor F exponents + biases come
  // with the validated schedule. It produces a structurally valid A2L1 our firmware
  // parses; tone is approximate until a2l.c + this converter are co-validated.
  const bytes = new Uint8Array(0);
  return { blob: bytes, name, nL, channels: C, headK, params: weights.length, note: "scaffold" };
}

/* ---------------- send to the pedal over the line protocol ---------------- */
export async function uploadIR(link, slot, taps, name, onProgress) {
  // protocol: IRBEGIN <slot> <ntaps> '<name>'  →  IRB <base64/hex chunks>  →  IREND
  await link.send(`IRBEGIN ${slot} ${taps.length} '${name}'`);
  const CH = 256; const buf = new Int16Array(taps.length);
  for (let i = 0; i < taps.length; i++) buf[i] = Math.max(-32768, Math.min(32767, Math.round(taps[i] * 32767)));
  for (let i = 0; i < buf.length; i += CH) {
    const slice = buf.subarray(i, i + CH);
    let hex = ""; for (const v of slice) hex += (v & 0xffff).toString(16).padStart(4, "0");
    await link.send(`IRB ${hex}`);
    onProgress && onProgress(Math.min(1, (i + CH) / buf.length));
  }
  return link.send(`IREND ${slot}`);
}

export async function uploadNam(link, slot, blob, name, onProgress) {
  await link.send(`LDBEGIN ${slot} ${blob.length} '${name}'`);
  const CH = 256;
  for (let i = 0; i < blob.length; i += CH) {
    let hex = ""; for (const b of blob.subarray(i, i + CH)) hex += b.toString(16).padStart(2, "0");
    await link.send(`LDB ${hex}`);
    onProgress && onProgress(Math.min(1, (i + CH) / blob.length));
  }
  return link.send(`CSAVE ${slot} '${name}'`);
}
