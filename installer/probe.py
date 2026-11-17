#!/usr/bin/env python3
"""probe.py — safe, read-only first contact with a Tank-G running TankFree or
UltraRig firmware, over its USB serial port. No dependencies (stdlib only),
macOS + Linux. Sends only query commands; writes nothing to the pedal.

    python3 probe.py            # auto-detect the port and query it
    python3 probe.py /dev/cu.usbmodem1234

What it does: open the CDC serial port at 115200, send PING, and a few
read-only status queries, and print whatever the firmware answers. This confirms
the device and the line protocol on real hardware.
"""
import sys, os, glob, time, platform

try:
    import termios, select
except ImportError:
    termios = None

BAUD = 115200
# read-only queries (they report state; none of these change anything)
QUERIES = ["PING", "INFO", "MEM", "BLE", "STAT", "PLIST", "CLIST", "ILIST"]


def candidate_ports():
    s = platform.system()
    if s == "Darwin":
        return sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/cu.usbserial*"))
    if s == "Linux":
        return sorted(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*"))
    return []


def open_raw(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    a = termios.tcgetattr(fd)
    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = a
    # raw mode
    iflag = 0
    oflag = 0
    lflag = 0
    cflag = (cflag | termios.CLOCAL | termios.CREAD) & ~termios.CSIZE
    cflag |= termios.CS8
    cflag &= ~(termios.PARENB | termios.CSTOPB)
    b = getattr(termios, "B%d" % BAUD)
    termios.tcsetattr(fd, termios.TCSANOW,
                      [iflag, oflag, cflag, lflag, b, b, cc])
    return fd


def ask(fd, line, wait=0.8):
    os.write(fd, (line + "\n").encode())
    end = time.time() + wait
    buf = b""
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.1)
        if r:
            try:
                chunk = os.read(fd, 4096)
                if chunk:
                    buf += chunk
            except (BlockingIOError, OSError):
                pass
    return buf.decode("latin1", "replace").strip()


def probe(path):
    print(f"→ abrindo {path} @ {BAUD}…")
    try:
        fd = open_raw(path)
    except Exception as e:
        print(f"  não consegui abrir ({e}). Feche o editor/outra aba que esteja usando a porta.")
        return False
    try:
        pong = ask(fd, "PING", 1.5)
        if not pong:
            print("  sem resposta ao PING (pode não ser o pedal, ou a porta está ocupada).")
            return False
        print(f"  ✓ respondeu: {pong!r}")
        print("\n── respostas (somente leitura) ──")
        for q in QUERIES[1:]:
            resp = ask(fd, q, 0.7)
            print(f"  {q:6s} ▸ {resp if resp else '(vazio / não suportado)'}")
        return True
    finally:
        os.close(fd)


def main():
    if termios is None:
        print("Windows não é suportado por este probe (use a página do editor em Chrome,"
              " ou pyserial). macOS/Linux funcionam sem instalar nada.")
        return 1
    ports = [sys.argv[1]] if len(sys.argv) > 1 else candidate_ports()
    if not ports:
        print("Nenhuma porta serial USB encontrada. Com o firmware UltraRig/TankFree,"
              " o pedal aparece como /dev/cu.usbmodem*. Conecte direto na USB (sem hub),"
              " ligue o pedal, e feche o editor web se estiver aberto.")
        return 1
    print("portas candidatas:", ", ".join(ports))
    for p in ports:
        if probe(p):
            print(f"\n✓ Pedal encontrado em {p}. Protocolo serial confirmado no hardware real.")
            return 0
    print("\nNenhuma porta respondeu como o pedal. Veja as dicas acima.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
