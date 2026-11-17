#!/usr/bin/env python3
"""tankctl — control a Tank-G (TankFree/UltraRig firmware) from the terminal over
USB serial. Verified against real hardware. No dependencies (stdlib), macOS/Linux.

    python3 tankctl.py list                 # show the live effects chain
    python3 tankctl.py cat                   # show the block catalog (FXT)
    python3 tankctl.py get <id>              # show one block's params + ranges
    python3 tankctl.py set <id> <param> <v>  # set a param (idx or name); echoes FXP ok
    python3 tankctl.py bypass <id> <0|1>     # enable/disable a block
    python3 tankctl.py preset <n>            # load a preset (PLOAD)

<id> is the block id from `list`. Writes are reversible (reload the preset).
"""
import sys, os, re, glob, platform
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import probe   # reuse open_raw/ask/candidate_ports


def port():
    ps = probe.candidate_ports()
    if not ps:
        print("Nenhuma porta serial do pedal. Conecte-o (firmware UltraRig/TankFree) e feche o editor.")
        sys.exit(1)
    return ps[0]


def connect():
    fd = probe.open_raw(port())
    for _ in range(4):                      # first PING after open is sometimes lost
        if "PONG" in probe.ask(fd, "PING", 1.0):
            return fd
    print("Pedal não respondeu ao PING. Feche o editor web (ele segura a porta) e tente de novo.")
    os.close(fd); sys.exit(1)


def parse_chain(lines):
    out = []
    for l in lines:
        m = re.match(r"FX\s+(\S+)\s+id=(\d+)\s+(\S+)\s+on=(\d+)\s+p=([\-\d,]+)", l)
        if m:
            out.append(dict(slot=m[1], id=int(m[2]), type=m[3], on=int(m[4]),
                            p=[int(x) for x in m[5].split(",")]))
    return out


def parse_cat(lines):
    cat, cur = {}, None
    for l in lines:
        m = re.match(r"FXT\s+(\d+)\s+(\S+)\s+cost=(\d+)\s+cat=(\S+)", l)
        if m:
            cur = dict(id=int(m[1]), name=m[2], cost=int(m[3]), cat=m[4], params=[])
            cat[m[2]] = cat[int(m[1])] = cur
            continue
        p = re.match(r"FXT\s+(\d+)\s+(\S+)\s+(-?\d+)\.\.(-?\d+)\s+\((-?\d+)\)", l)
        if p and cur is not None:
            cur["params"].append(dict(idx=int(p[1]), name=p[2], min=int(p[3]), max=int(p[4]), dflt=int(p[5])))
    return cat


def main(a):
    if not a:
        print(__doc__); return 0
    cmd = a[0]
    if cmd == "cat":
        fd = connect()
        try:
            for l in probe.ask(fd, "FXT", 1.5).splitlines():
                if l.startswith("FXT"): print(l)
        finally: os.close(fd)
        return 0
    if cmd == "list":
        fd = connect()
        try:
            ch = parse_chain(probe.ask(fd, "FXS", 1.3).splitlines())
            print("id  slot  type        on  params")
            for b in ch:
                print(f"{b['id']:<3} {b['slot']:<5} {b['type']:<11} {b['on']}   {b['p']}")
        finally: os.close(fd)
        return 0
    if cmd == "get":
        bid = int(a[1]); fd = connect()
        try:
            ch = {b['id']: b for b in parse_chain(probe.ask(fd, "FXS", 1.3).splitlines())}
            cat = parse_cat(probe.ask(fd, "FXT", 1.5).splitlines())
            b = ch.get(bid)
            if not b: print("id não está na cadeia"); return 1
            d = cat.get(b['type'], dict(params=[]))
            print(f"id={bid} {b['type']} on={b['on']}")
            for pm in d['params']:
                v = b['p'][pm['idx']] if pm['idx'] < len(b['p']) else '?'
                print(f"  [{pm['idx']}] {pm['name']:<14} {pm['min']}..{pm['max']} = {v}")
        finally: os.close(fd)
        return 0
    if cmd == "set":
        bid, pname, val = int(a[1]), a[2], int(a[3]); fd = connect()
        try:
            cat = parse_cat(probe.ask(fd, "FXT", 1.5).splitlines())
            ch = {b['id']: b for b in parse_chain(probe.ask(fd, "FXS", 1.3).splitlines())}
            b = ch.get(bid)
            pidx = int(pname) if pname.isdigit() else next(
                (p['idx'] for p in cat.get(b['type'], dict(params=[]))['params'] if p['name'] == pname), None)
            if pidx is None: print("parâmetro não encontrado"); return 1
            print(probe.ask(fd, f"FXP {bid} {pidx} {val}", 1.0).strip())
        finally: os.close(fd)
        return 0
    if cmd == "bypass":
        fd = connect()
        try: print(probe.ask(fd, f"FXE {a[1]} {a[2]}", 1.0).strip() or "OK")
        finally: os.close(fd)
        return 0
    if cmd == "preset":
        fd = connect()
        try: print(probe.ask(fd, f"PLOAD {a[1]}", 1.2).strip() or "OK")
        finally: os.close(fd)
        return 0
    print(__doc__); return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
