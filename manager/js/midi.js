/* midi.js — Valeton GP-5-compatible MIDI layer for OpenTank-G.
 *
 * Listens on USB-MIDI and BLE-MIDI (channel 1) and translates the GP-5 CC map
 * into TankLink protocol actions. See docs/MIDI_GP5.md.
 *
 * GP-5 facts replicated: CC0(0-99)=patch; no Program Change; no CC32; no clock.
 * Block toggles CC48-57: 0-63 off, 64-127 on, mapped to the first block of each
 * category in our FREE chain (GP-5-compatible facade).
 */

// GP-5 fixed slots -> our block categories (free-chain reconciliation).
export const CC_BLOCK = {
  48: "nr", 49: "pre", 50: "dst", 51: "nam", 52: "amp",
  53: "cab", 54: "eq", 55: "mod", 56: "dly", 57: "rvb",
  58: "tuner", 69: "ctl",
};

export class Gp5Midi {
  constructor(link, { channel = 0 } = {}) {   // channel 0 == MIDI ch 1
    this.link = link; this.channel = channel; this.onAction = null;
    this.patch = 0; this.access = null;
  }

  async start() {
    this.access = await navigator.requestMIDIAccess({ sysex: true });
    for (const input of this.access.inputs.values()) this._bind(input);
    this.access.onstatechange = (e) => {
      if (e.port.type === "input" && e.port.state === "connected") this._bind(e.port);
    };
    return this;
  }

  _bind(input) { input.onmidimessage = (m) => this._onMessage(m.data); }

  _onMessage(data) {
    const status = data[0] & 0xf0, ch = data[0] & 0x0f;
    if (ch !== this.channel) return;          // GP-5 is channel 1, fixed
    if (status === 0xb0) this._cc(data[1], data[2]);
    // Program Change (0xC0) intentionally ignored — GP-5 doesn't use it.
    // SysEx (0xF0) -> editor protocol, handled elsewhere if needed.
  }

  _cc(cc, val) {
    let action = null;
    if (cc === 0) { this.patch = Math.min(99, val); action = ["patch", this.patch]; }
    else if (cc === 7) action = ["patchVolume", val];
    else if (cc === 22) action = ["bank", -1];
    else if (cc === 23) action = ["bank", +1];
    else if (cc === 24) action = ["patchStep", -1];
    else if (cc === 25) action = ["patchStep", +1];
    else if (cc === 29) action = ["songStep", -1];
    else if (cc === 30) action = ["songStep", +1];
    else if (cc in CC_BLOCK) action = ["block", CC_BLOCK[cc], val >= 64];
    if (action) { this._apply(action); if (this.onAction) this.onAction(cc, val, action); }
  }

  async _apply([kind, a, b]) {
    const L = this.link; if (!L || !L.connected) return;
    try {
      switch (kind) {
        case "patch":     await L.loadPreset(a); break;
        case "patchStep": await L.loadPreset(this.patch = clamp(this.patch + a, 0, 99)); break;
        case "bank":      await L.loadPreset(this.patch = clamp(this.patch + a * 10, 0, 99)); break;
        case "songStep":  await L.loadPreset(this.patch = clamp(this.patch + a, 0, 99)); break;
        case "patchVolume": await L.send(`GSAVE vol=${a}`).catch(() => {}); break;
        case "block":
          if (a === "tuner") await L.tuner(b);
          else await L.send(`FXCAT ${a} ${b ? 1 : 0}`).catch(() => {});  // toggle 1st block of category
          break;
      }
    } catch (e) { /* ignore transient */ }
  }
}

const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
