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
  loadPreset(n)        { return this.send(`PLOAD ${n}`); }
  savePreset(n, name)  { return this.send(`PSAVE ${n} ${name ?? ""}`.trim()); }
  chain()              { return this.send("FXS"); }               // effects snapshot
  blockEnable(i, on)   { return this.send(`FXE ${i} ${on ? 1 : 0}`); }
  amp(params)          { return this.send(`AMPEQ ${params}`); }   // volume=.. bass=.. ...
  gain(inDb, outDb)    { return this.send(`GAIN ${inDb} ${outDb}`); }
  tuner(on)            { return this.send(`TUNER ${on ? 1 : 0}`); }
  audio(mode)          { return this.send(`AUDIO ${mode}`); }     // OFF|THRU|A2L

  // --- real-device readers (parse the firmware's actual reply formats) ---
  async version()  { const l = await this.send("PING", { until: "PONG" }); return (l.find(x => x.startsWith("PONG")) || "").replace("PONG ", ""); }
  async presets()  { return parsePresets(await this.send("PLIST")); }
  async captures() { return parseList(await this.send("CLIST"), "CAP"); }
  async irs()      { return parseList(await this.send("ILIST"), "IRS"); }
  async meters()   { return parseMeters(await this.send("MEM"), await this.send("STAT")); }
}

// PLIST → "PRE <idx> <a> <b> <c> 'Name' t=.. s=.. col=rrggbb" + "PLIST end current=N"
export function parsePresets(lines) {
  const out = [];
  for (const l of lines) {
    const m = l.match(/^PRE\s+(\d+)\b.*?'([^']*)'.*?(?:col=([0-9a-fA-F]{6}))?\s*$/);
    if (m) out.push({ idx: +m[1], name: m[2], color: m[3] ? "#" + m[3] : "#e6a648" });
  }
  return out;
}
// CLIST/ILIST → "<TAG> <idx> <size> 'Name' ..." + "<...> end ... slots=N"
export function parseList(lines, tag) {
  const out = [];
  for (const l of lines) {
    const m = l.match(new RegExp("^" + tag + "\\s+(\\d+)\\s+(\\d+)\\s+'([^']*)'(.*)$"));
    if (m) out.push({ idx: +m[1], size: +m[2], name: m[3], info: m[4].trim() });
    const e = l.match(/slots=(\d+)/); if (e) out._slots = +e[1];
  }
  return out;
}
// MEM + STAT → { cpuPct, memPct } from real figures.
export function parseMeters(memLines, statLines) {
  const g = (arr, re) => { for (const l of arr) { const m = l.match(re); if (m) return m; } return null; };
  let cpu = 0, mem = 0;
  const blk = g(statLines, /\bblk=(\d+)\/(\d+)/);        // used/budget ticks per block
  if (blk) cpu = Math.min(100, Math.round(100 * +blk[1] / +blk[2]));
  const pool = g(memLines, /\bpool_used=(\d+).*?\bpool=(\d+)/) || g(memLines, /\bpool=(\d+).*?pool_used=(\d+)/);
  const poolM = g(memLines, /\bpool=(\d+)/), usedM = g(memLines, /\bpool_used=(\d+)/);
  if (poolM && usedM) mem = Math.min(100, Math.round(100 * +usedM[1] / +poolM[1]));
  return { cpuPct: cpu, memPct: mem };
}
