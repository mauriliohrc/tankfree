/* protocol.js — Web Serial client for the OpenTank-G ASCII line protocol.
 *
 * Transport: USB CDC serial @ 115200 (Chrome/Edge Web Serial). Commands are
 * newline-terminated ASCII; the pedal answers with lines, ending in "OK" or
 * "ERR ...". Mirrors the reverse-engineered UltraRig command vocabulary
 * (see docs/RE_FINDINGS.md §6) so existing firmware is also drivable.
 */
export const USB_VENDOR_ID = 0x3654;

export class TankLink {
  constructor() {
    this.port = null; this.reader = null; this.writer = null;
    this._buf = ""; this._waiters = []; this.onEvent = null; this.connected = false;
  }

  async connect(existing) {
    this.port = existing || await navigator.serial.requestPort({
      filters: [{ usbVendorId: USB_VENDOR_ID }],
    });
    await this.port.open({ baudRate: 115200 });
    this.writer = this.port.writable.getWriter();
    this.reader = this.port.readable.getReader();
    this.connected = true;
    this._readLoop();
    await this.send("PING", { until: "PONG", ms: 1500 }).catch(() => {});
    return this;
  }

  async disconnect() {
    this.connected = false;
    try { await this.reader?.cancel(); } catch {}
    try { this.writer?.releaseLock(); } catch {}
    try { await this.port?.close(); } catch {}
  }

  async _readLoop() {
    const dec = new TextDecoder();
    try {
      while (this.connected) {
        const { value, done } = await this.reader.read();
        if (done) break;
        this._buf += dec.decode(value, { stream: true });
        let nl;
        while ((nl = this._buf.indexOf("\n")) >= 0) {
          const line = this._buf.slice(0, nl).replace(/\r$/, "");
          this._buf = this._buf.slice(nl + 1);
          this._dispatch(line);
        }
      }
    } catch (e) { /* port closed */ }
  }

  _dispatch(line) {
    // async events are prefixed tokens ending in '!' (EVT/STAT!/LVL/CRASH!...)
    const tok = line.split(" ", 1)[0];
    if (/[!]$/.test(tok) && this.onEvent) this.onEvent(line);
    for (const w of this._waiters) w.lines.push(line);
    // resolve waiters whose terminator matched
    this._waiters = this._waiters.filter((w) => {
      if (line === "OK" || line.startsWith("ERR") || (w.until && line.startsWith(w.until))) {
        clearTimeout(w.timer);
        if (line.startsWith("ERR")) w.reject(new Error(line));
        else w.resolve(w.lines);
        return false;
      }
      return true;
    });
  }

  /* send(cmd, {until, ms}) -> Promise<string[]> (collected reply lines). */
  send(cmd, { until = null, ms = 2500 } = {}) {
    if (!this.connected) return Promise.reject(new Error("not connected"));
    return new Promise((resolve, reject) => {
      const w = { until, lines: [], resolve, reject,
        timer: setTimeout(() => {
          this._waiters = this._waiters.filter((x) => x !== w);
          reject(new Error("timeout: " + cmd));
        }, ms) };
      this._waiters.push(w);
      this.writer.write(new TextEncoder().encode(cmd + "\n"));
    });
  }

  // --- high-level helpers (thin wrappers over the line protocol) ---
  presets()            { return this.send("PLIST"); }
  loadPreset(n)        { return this.send(`PLOAD ${n}`); }
  savePreset(n, name)  { return this.send(`PSAVE ${n} ${name ?? ""}`.trim()); }
  chain()              { return this.send("FXS"); }               // effects snapshot
  blockEnable(i, on)   { return this.send(`FXE ${i} ${on ? 1 : 0}`); }
  amp(params)          { return this.send(`AMPEQ ${params}`); }   // volume=.. bass=.. ...
  gain(inDb, outDb)    { return this.send(`GAIN ${inDb} ${outDb}`); }
  captures()           { return this.send("CLIST"); }
  irs()                { return this.send("ILIST"); }
  tuner(on)            { return this.send(`TUNER ${on ? 1 : 0}`); }
  audio(mode)          { return this.send(`AUDIO ${mode}`); }     // OFF|THRU|A2L
}
