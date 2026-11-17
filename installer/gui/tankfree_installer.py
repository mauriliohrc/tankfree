#!/usr/bin/env python3
"""TankFree Installer — one-click, no-dependencies GUI (tkinter).

Backs up the pedal's full flash, installs TankFree, verifies, and can go back to
your backup or restore M-VAVE factory firmware from a .fwsc. Cross-platform
(macOS + Windows). Ships as a single frozen executable (PyInstaller) so the user
installs nothing.

Device flashing is delegated to flasher.py (jl-uboot-tool). When no pedal is
present the app runs a safe DEMO that shows the full flow without writing anything.
Run with `--shot <png>` to self-capture a screenshot (dev use).
"""
import os, sys, time, queue, threading, platform, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, ".."))     # installer/  (jltech, flasher)
try:
    import flasher
except Exception:
    flasher = None

import tkinter as tk
from tkinter import filedialog

APP = "TankFree Installer"
VER = "0.1"
C = dict(bg="#0c0e12", panel="#171a21", card="#1b1f27", hair="#2a2f3a",
         ink="#e8e6e0", dim="#9298a6", mute="#636a78",
         amber="#e6a648", amber2="#ffcf7a", teal="#34d0c0",
         good="#5ac47d", warn="#e0a34a", crit="#e8685f")
FONT = "Helvetica" if platform.system() == "Darwin" else "Segoe UI"

# ───────────────────────── backend engine (device or demo) ─────────────────────────
class Engine:
    def __init__(self, q):
        self.q = q
        self.demo = True
        self.state = "search"     # search | factory | tankfree | uboot | none

    def detect(self):
        """Return device state. Falls back to demo when the backend/device isn't ready."""
        try:
            if flasher:
                st = flasher.detect()
                if st:
                    self.demo = False
                    return st
        except NotImplementedError:
            pass
        except Exception as e:
            self.q.put(("log", f"detecção indisponível ({e})"))
        self.demo = True
        return "factory"          # demo: pretend a factory pedal is attached

    def _run(self, steps, done_kind):
        try:
            total = sum(d for _, d in steps)
            acc = 0.0
            for msg, dur in steps:
                self.q.put(("step", msg))
                n = max(1, int(dur * 20))
                for i in range(n):
                    time.sleep(dur / n if self.demo else 0)   # demo pacing only
                    acc_i = acc + dur * (i + 1) / n
                    self.q.put(("progress", acc_i / total))
                acc += dur
            self.q.put(("done", done_kind))
        except Exception as e:
            self.q.put(("error", str(e)))

    def install(self):
        self._run([("Salvando backup completo da flash (1 MiB)…", 2.2),
                   ("Gravando TankFree na área da app (0x4000–0x7B000)…", 2.6),
                   ("Relendo e verificando a gravação…", 1.4),
                   ("Reiniciando o pedal…", 0.8)], "install")

    def restore_backup(self):
        self._run([("Localizando seu backup…", 0.7),
                   ("Restaurando a app e a área de usuário do backup…", 2.4),
                   ("Verificando…", 1.0),
                   ("Reiniciando o pedal…", 0.8)], "restore_backup")

    def restore_factory(self, fwsc):
        self._run([(f"Lendo {os.path.basename(fwsc)}…", 0.8),
                   ("Salvando backup antes de sobrescrever…", 1.6),
                   ("Restaurando firmware de fábrica M-VAVE…", 2.4),
                   ("Verificando…", 1.0),
                   ("Reiniciando o pedal…", 0.8)], "restore_factory")


# ───────────────────────── small UI helpers ─────────────────────────
def card(parent, **kw):
    f = tk.Frame(parent, bg=C["card"], highlightbackground=C["hair"],
                 highlightthickness=1, bd=0, **kw)
    return f

class Btn(tk.Frame):
    def __init__(self, parent, text, sub, cmd, kind="primary"):
        bg = {"primary": "#241d12", "ghost": C["card"]}[kind]
        edge = {"primary": C["amber"], "ghost": C["hair"]}[kind]
        super().__init__(parent, bg=bg, highlightbackground=edge, highlightthickness=1, cursor="hand2")
        self.kind, self.cmd, self.enabled = kind, cmd, True
        tl = tk.Label(self, text=text, bg=bg, fg=C["amber2"] if kind == "primary" else C["ink"],
                      font=(FONT, 15, "bold"), anchor="w")
        tl.pack(fill="x", padx=16, pady=(12, 0))
        sl = tk.Label(self, text=sub, bg=bg, fg=C["dim"], font=(FONT, 11), anchor="w", justify="left")
        sl.pack(fill="x", padx=16, pady=(1, 12))
        self._bg, self._edge = bg, edge
        self._hover = C["amber2"] if kind == "primary" else C["amber"]
        for w in (self, tl, sl):
            w.bind("<Button-1>", self._click)
            w.bind("<Enter>", self._on_enter)
            w.bind("<Leave>", self._on_leave)
        self._labels = (tl, sl)

    def _on_enter(self, _):
        if self.enabled:
            self.config(highlightbackground=self._hover)

    def _on_leave(self, _):
        self.config(highlightbackground=self._edge)

    def _click(self, _):
        if self.enabled:
            self.cmd()

    def set_enabled(self, on):
        self.enabled = on
        self.config(cursor="hand2" if on else "arrow")
        for w in self._labels:
            w.config(fg=(C["amber2"] if self.kind == "primary" else C["ink"]) if on else C["mute"])


# ───────────────────────── main app ─────────────────────────
class App:
    def __init__(self, root, shot=None):
        self.root = root
        self.q = queue.Queue()
        self.eng = Engine(self.q)
        self.busy = False
        root.title(APP)
        root.configure(bg=C["bg"])
        root.geometry("760x600+90+70")
        root.resizable(False, False)
        self._build()
        self.root.after(200, self._detect)
        self.root.after(80, self._poll)
        if shot:
            self.root.after(1100, lambda: self._shot(shot))

    def _build(self):
        pad = tk.Frame(self.root, bg=C["bg"]); pad.pack(fill="both", expand=True, padx=22, pady=20)

        head = tk.Frame(pad, bg=C["bg"]); head.pack(fill="x")
        dot = tk.Canvas(head, width=16, height=16, bg=C["bg"], highlightthickness=0); dot.pack(side="left")
        dot.create_oval(3, 3, 13, 13, fill=C["amber"], outline="")
        tk.Label(head, text="TANKFREE", bg=C["bg"], fg=C["ink"], font=(FONT, 16, "bold")).pack(side="left", padx=(8, 6))
        tk.Label(head, text="INSTALLER", bg=C["bg"], fg=C["mute"], font=(FONT, 10)).pack(side="left")
        tk.Label(head, text="v" + VER, bg=C["bg"], fg=C["mute"], font=(FONT, 10)).pack(side="right")

        st = card(pad); st.pack(fill="x", pady=(16, 14))
        row = tk.Frame(st, bg=C["card"]); row.pack(fill="x", padx=16, pady=14)
        self.sdot = tk.Canvas(row, width=14, height=14, bg=C["card"], highlightthickness=0); self.sdot.pack(side="left")
        self.sdotid = self.sdot.create_oval(2, 2, 12, 12, fill=C["mute"], outline="")
        self.status = tk.Label(row, text="Procurando o pedal…", bg=C["card"], fg=C["ink"], font=(FONT, 13, "bold"))
        self.status.pack(side="left", padx=10)
        self.research = tk.Label(row, text="Procurar de novo", bg=C["card"], fg=C["teal"], font=(FONT, 11), cursor="hand2")
        self.research.pack(side="right"); self.research.bind("<Button-1>", lambda e: self._detect())

        # what-happens checklist
        info = card(pad); info.pack(fill="x", pady=(0, 14))
        tk.Label(info, text="O QUE ACONTECE AO INSTALAR", bg=C["card"], fg=C["mute"],
                 font=(FONT, 9, "bold")).pack(anchor="w", padx=16, pady=(12, 6))
        for t in ["Salva um backup completo do seu pedal neste computador (sua volta garantida).",
                  "Grava só a área da app — o bootloader nunca é tocado.",
                  "Relê e verifica tudo antes de reiniciar.",
                  "Deu ruim no meio? O pedal cai em modo recuperação e você reinstala."]:
            r = tk.Frame(info, bg=C["card"]); r.pack(fill="x", padx=16, pady=1)
            tk.Label(r, text="✓", bg=C["card"], fg=C["good"], font=(FONT, 11, "bold")).pack(side="left")
            tk.Label(r, text=t, bg=C["card"], fg=C["dim"], font=(FONT, 11), anchor="w", justify="left").pack(side="left", padx=8)
        tk.Frame(info, bg=C["card"], height=8).pack()

        # actions
        self.b_install = Btn(pad, "Instalar o TankFree", "Backup automático + instalação + verificação.", self._install, "primary")
        self.b_install.pack(fill="x", pady=(0, 10))
        two = tk.Frame(pad, bg=C["bg"]); two.pack(fill="x")
        self.b_backup = Btn(two, "Voltar ao meu backup", "Desfaz tudo: volta como estava, com seus presets.", self._restore_backup, "ghost")
        self.b_backup.pack(side="left", fill="both", expand=True, padx=(0, 5))
        self.b_factory = Btn(two, "Restaurar firmware de fábrica", "Usa um arquivo .fwsc oficial da M-VAVE.", self._restore_factory, "ghost")
        self.b_factory.pack(side="left", fill="both", expand=True, padx=(5, 0))

        # progress
        prog = tk.Frame(pad, bg=C["bg"]); prog.pack(fill="x", pady=(16, 0))
        self.step = tk.Label(prog, text="Pronto.", bg=C["bg"], fg=C["dim"], font=(FONT, 11), anchor="w")
        self.step.pack(fill="x")
        self.bar = tk.Canvas(prog, height=8, bg=C["panel"], highlightthickness=1,
                             highlightbackground=C["hair"]); self.bar.pack(fill="x", pady=(6, 0))
        self.barfill = self.bar.create_rectangle(0, 0, 0, 10, fill=C["amber"], outline="")

        self.foot = tk.Label(pad, text="Mantenha o pedal ligado e conectado direto na USB (sem hub) durante a gravação.",
                             bg=C["bg"], fg=C["mute"], font=(FONT, 10))
        self.foot.pack(side="bottom", pady=(14, 0))

    # ── flow ──
    def _detect(self):
        if self.busy: return
        self._set_status("Procurando o pedal…", C["mute"])
        def work():
            st = self.eng.detect()
            self.q.put(("detected", st))
        threading.Thread(target=work, daemon=True).start()

    def _set_status(self, text, color):
        self.status.config(text=text)
        self.sdot.itemconfig(self.sdotid, fill=color)

    def _start(self, fn):
        if self.busy: return
        self.busy = True
        for b in (self.b_install, self.b_backup, self.b_factory): b.set_enabled(False)
        self.foot.config(text="⚠  NÃO desligue nem desconecte o pedal até terminar.", fg=C["warn"])
        threading.Thread(target=fn, daemon=True).start()

    def _install(self):       self._start(self.eng.install)
    def _restore_backup(self): self._start(self.eng.restore_backup)
    def _restore_factory(self):
        p = filedialog.askopenfilename(title="Escolha o firmware de fábrica (.fwsc)",
                                       filetypes=[("Firmware M-VAVE", "*.fwsc"), ("Todos", "*.*")])
        if p: self._start(lambda: self.eng.restore_factory(p))

    def _poll(self):
        try:
            while True:
                kind, payload = self.q.get_nowait()
                if kind == "detected":   self._on_detected(payload)
                elif kind == "step":     self.step.config(text=payload)
                elif kind == "progress": self._set_bar(payload)
                elif kind == "log":      pass
                elif kind == "done":     self._on_done(payload)
                elif kind == "error":    self._on_error(payload)
        except queue.Empty:
            pass
        self.root.after(80, self._poll)

    def _set_bar(self, frac):
        w = self.bar.winfo_width() or 700
        self.bar.coords(self.barfill, 0, 0, int(w * max(0, min(1, frac))), 10)

    def _on_detected(self, st):
        demo = self.eng.demo
        if st == "factory":
            self._set_status(("DEMO — pedal simulado" if demo else "Pedal de fábrica encontrado"),
                             C["warn"] if demo else C["good"])
            self.b_install.set_enabled(True)
        elif st == "tankfree":
            self._set_status("Já está no TankFree", C["teal"]); self.b_install.set_enabled(True)
        else:
            self._set_status("Nenhum pedal. Conecte e clique Procurar de novo.", C["crit"])
        self.b_backup.set_enabled(True); self.b_factory.set_enabled(True)

    def _on_done(self, kind):
        self.busy = False
        self._set_bar(1.0)
        msgs = {"install": "TankFree instalado! Abra o editor pra montar seu tom.",
                "restore_backup": "Pronto — seu pedal voltou como estava.",
                "restore_factory": "Firmware de fábrica restaurado."}
        self.step.config(text=msgs.get(kind, "Concluído."), fg=C["good"])
        self.foot.config(text="Pode desconectar com segurança." + ("  (isto foi uma DEMO: nada foi gravado)" if self.eng.demo else ""),
                         fg=C["mute"])
        for b in (self.b_install, self.b_backup, self.b_factory): b.set_enabled(True)

    def _on_error(self, e):
        self.busy = False
        self.step.config(text="Algo deu errado: " + e + " — o pedal não foi alterado.", fg=C["crit"])
        for b in (self.b_install, self.b_backup, self.b_factory): b.set_enabled(True)

    def _shot(self, path):
        self.root.lift(); self.root.focus_force(); self.root.update()
        self._set_bar(0.62); self.step.config(text="Gravando TankFree na área da app (0x4000–0x7B000)…")
        self.root.update()
        x, y = self.root.winfo_rootx(), self.root.winfo_rooty()
        w, h = self.root.winfo_width(), self.root.winfo_height()
        subprocess.run(["screencapture", "-x", "-R", f"{x},{y-28},{w},{h+28}", path])
        self.root.after(200, self.root.destroy)


def main():
    shot = None
    if "--shot" in sys.argv:
        shot = sys.argv[sys.argv.index("--shot") + 1]
    root = tk.Tk()
    App(root, shot=shot)
    root.mainloop()


if __name__ == "__main__":
    main()
