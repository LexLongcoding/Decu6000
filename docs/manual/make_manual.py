#!/usr/bin/env python3
# Decu6000 - Copyright (C) 2026 LexLongcoding
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Builds docs/Decu6000-User-Manual.pdf.

Control ranges and defaults are read straight from Source/Parameters.h, so the
manual stays in step with the plugin. Requires: pip install reportlab
Run from anywhere:  python3 docs/manual/make_manual.py
"""
import os, re
from reportlab.lib.pagesizes import letter
from reportlab.lib.units import inch
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Table,
                                TableStyle, Image, PageBreak, KeepTogether, NextPageTemplate, CondPageBreak)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "docs", "Decu6000-User-Manual.pdf")
IMG = os.path.join(ROOT, "docs", "images")
VERSION = re.search(r"project\(Decu6000 VERSION ([0-9.]+)\)", open(os.path.join(ROOT, "CMakeLists.txt")).read()).group(1)

# ---------------------------------------------------------------- fonts & colours
FONTS = os.path.join(ROOT, "Resources", "Fonts")
pdfmetrics.registerFont(TTFont("DecuDisplay", os.path.join(FONTS, "DecuDisplay-Bold.ttf")))
pdfmetrics.registerFont(TTFont("DecuBlack", os.path.join(FONTS, "DecuDisplay-Black.ttf")))
pdfmetrics.registerFont(TTFont("DecuMono", os.path.join(FONTS, "ShareTechMono-Regular.ttf")))

BG     = colors.HexColor("#12102B")
PANEL  = colors.HexColor("#2A2150")
ORANGE = colors.HexColor("#FF7A1A")
PINK   = colors.HexColor("#FF2E93")
CYAN   = colors.HexColor("#3DE8FF")
INK    = colors.HexColor("#1D1838")
SOFT   = colors.HexColor("#F3F1FB")
RULE   = colors.HexColor("#D8D2FF")
MUTED  = colors.HexColor("#5E5885")

S = {
    "h1":   ParagraphStyle("h1", fontName="DecuDisplay", fontSize=20, leading=24, textColor=INK, spaceBefore=4, spaceAfter=10),
    "h2":   ParagraphStyle("h2", fontName="DecuDisplay", fontSize=13, leading=17, textColor=PINK, spaceBefore=12, spaceAfter=5),
    "h3":   ParagraphStyle("h3", fontName="Helvetica-Bold", fontSize=10.5, leading=14, textColor=INK, spaceBefore=8, spaceAfter=3),
    "body": ParagraphStyle("body", fontName="Helvetica", fontSize=9.6, leading=13.6, textColor=INK, spaceAfter=6),
    "bul":  ParagraphStyle("bul", fontName="Helvetica", fontSize=9.6, leading=13.4, textColor=INK, leftIndent=14,
                           bulletIndent=3, spaceAfter=2.5),
    "cell": ParagraphStyle("cell", fontName="Helvetica", fontSize=8.6, leading=11, textColor=INK),
    "cellb":ParagraphStyle("cellb", fontName="Helvetica-Bold", fontSize=8.6, leading=11, textColor=INK),
    "head": ParagraphStyle("head", fontName="DecuMono", fontSize=8.6, leading=11, textColor=colors.white),
    "cap":  ParagraphStyle("cap", fontName="Helvetica-Oblique", fontSize=8.4, leading=11, textColor=MUTED,
                           alignment=TA_CENTER, spaceBefore=3, spaceAfter=10),
    "note": ParagraphStyle("note", fontName="Helvetica", fontSize=9, leading=12.6, textColor=INK),
    "toc":  ParagraphStyle("toc", fontName="Helvetica", fontSize=10.5, leading=18, textColor=INK),
}

def P(t, s="body"): return Paragraph(t, S[s])
def bullets(items): return [Paragraph(i, S["bul"], bulletText="\u2022") for i in items]

def note(text, colour=CYAN):
    t = Table([[Paragraph(text, S["note"])]], colWidths=[6.5 * inch])
    t.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), SOFT), ("LINEBEFORE", (0, 0), (0, -1), 3, colour),
                           ("LEFTPADDING", (0, 0), (-1, -1), 10), ("RIGHTPADDING", (0, 0), (-1, -1), 10),
                           ("TOPPADDING", (0, 0), (-1, -1), 7), ("BOTTOMPADDING", (0, 0), (-1, -1), 7)]))
    return t

def table(rows, widths, header=True, accent=PANEL):
    data = []
    for r, row in enumerate(rows):
        style = "head" if (header and r == 0) else "cell"
        data.append([c if not isinstance(c, str) else Paragraph(c, S[style]) for c in row])
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    st = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.4, RULE),
          ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
          ("LEFTPADDING", (0, 0), (-1, -1), 5), ("RIGHTPADDING", (0, 0), (-1, -1), 5)]
    if header:
        st += [("BACKGROUND", (0, 0), (-1, 0), accent)]
    for r in range(1 if header else 0, len(rows)):
        if r % 2 == 0:
            st.append(("BACKGROUND", (0, r), (-1, r), SOFT))
    t.setStyle(TableStyle(st))
    return t

def shot(name, width=6.5 * inch, caption=None):
    path = os.path.join(IMG, name)
    from reportlab.lib.utils import ImageReader
    iw, ih = ImageReader(path).getSize()
    img = Image(path, width=width, height=width * ih / iw)
    out = [img]
    if caption: out.append(P(caption, "cap"))
    return KeepTogether(out)

# ---------------------------------------------------------------- parameters from source
SPEC_RE = re.compile(r'\{\s*"(\w+)",\s*"([^"]+)",\s*"([^"]+)",\s*([-0-9.]+)f,\s*([-0-9.]+)f,\s*([-0-9.]+)f,\s*([-0-9.]+)f,\s*U::(\w+)\s*\}')
SPECS = {m[0]: dict(id=m[0], name=m[1], knob=m[2], lo=float(m[3]), hi=float(m[4]), default=float(m[6]), unit=m[7])
         for m in SPEC_RE.findall(open(os.path.join(ROOT, "Source", "Parameters.h")).read())}

def fmt(v, unit):
    if unit == "Percent":   return f"{round(v * 100)}%"
    if unit == "Hz":        return f"{v:.1f} Hz" if v < 20 else f"{round(v)} Hz"
    if unit == "Time":      return f"{round(v * 1000)} ms" if v < 1 else f"{v:.2f} s".replace(".00 s", " s")
    if unit == "Semitones": st = round(v * 24 - 12); return f"{'+' if st > 0 else ''}{st} st"
    if unit == "Ratio":     return f"{v:.2f}:1"
    if unit == "Integer":   return str(round(v))
    return str(v)

def rng(s): return f"{fmt(s['lo'], s['unit'])} to {fmt(s['hi'], s['unit'])}"

def controls(prefix, desc, accent=PANEL):
    rows = [["CONTROL", "WHAT IT DOES", "RANGE", "DEFAULT"]]
    for pid, s in SPECS.items():
        if pid.split("_")[0] != prefix: continue
        suffix = pid.split("_", 1)[1]
        rows.append([f"<b>{s['knob']}</b>", desc.get(suffix, ""), rng(s), fmt(s["default"], s["unit"])])
    return table(rows, [0.95 * inch, 3.45 * inch, 1.3 * inch, 0.8 * inch], accent=accent)

COMMON = {
    "pitch": "Tuning of the voice.", "tune": "Tuning of the voice.",
    "decay": "How long the sound rings (time to fade by 60 dB).",
    "level": "Volume of this voice.",
    "drive": "Saturation - warmer and louder at first, gritty and distorted when pushed.",
    "crush": "Lo-fi sample-rate and bit reduction, like an old cheap sampler.",
    "send":  "How much of this voice goes to the gated reverb.",
}
def D(**extra):
    d = dict(COMMON); d.update(extra); return d

# ---------------------------------------------------------------- page decoration
def on_cover(c, doc):
    w, h = letter
    c.setFillColor(BG); c.rect(0, 0, w, h, stroke=0, fill=1)
    c.setFillColor(colors.Color(0.24, 0.91, 1.0, alpha=0.035))
    for y in range(0, int(h), 3): c.rect(0, y, w, 1, stroke=0, fill=1)
    bw = w - 1.5 * inch
    c.setFillColor(ORANGE); c.rect(0.75 * inch, h - 3.05 * inch, bw * 0.5, 3, stroke=0, fill=1)
    c.setFillColor(PINK);   c.rect(0.75 * inch + bw * 0.5, h - 3.05 * inch, bw * 0.333, 3, stroke=0, fill=1)
    c.setFillColor(CYAN);   c.rect(0.75 * inch + bw * 0.833, h - 3.05 * inch, bw * 0.167, 3, stroke=0, fill=1)
    c.setFont("DecuBlack", 54)
    c.setFillColor(PINK);   c.drawString(0.75 * inch, h - 2.2 * inch, "DECU")
    c.setFillColor(ORANGE); c.drawString(0.75 * inch + pdfmetrics.stringWidth("DECU", "DecuBlack", 54), h - 2.2 * inch, "6000")
    c.setFillColor(colors.HexColor("#D8D2FF")); c.setFont("DecuMono", 13)
    c.drawString(0.78 * inch, h - 2.65 * inch, "BIO-NEURAL PERCUSSION SYNTHESIZER")
    c.setFont("DecuDisplay", 26); c.setFillColor(colors.white)
    c.drawString(0.75 * inch, h - 3.75 * inch, "USER MANUAL")
    c.setFont("DecuMono", 11); c.setFillColor(CYAN)
    c.drawString(0.78 * inch, h - 4.1 * inch, f"VERSION {VERSION}  //  VST3 + STANDALONE  //  WINDOWS + LINUX")
    img = os.path.join(IMG, "rack_view.png")
    iw = w - 1.5 * inch; ih = iw * 820 / 1440
    c.drawImage(img, 0.75 * inch, 1.55 * inch, iw, ih)
    c.setStrokeColor(colors.HexColor("#3A3170")); c.setLineWidth(1); c.rect(0.75 * inch, 1.55 * inch, iw, ih)
    c.setFont("DecuMono", 9); c.setFillColor(colors.HexColor("#8A83B8"))
    c.drawString(0.78 * inch, 0.9 * inch, "FREE & OPEN SOURCE  //  GNU AGPL v3.0 OR LATER  //  GITHUB.COM/LEXLONGCODING/DECU6000")

def on_page(c, doc):
    w, h = letter
    c.setFillColor(BG); c.rect(0, h - 0.42 * inch, w, 0.42 * inch, stroke=0, fill=1)
    c.setFont("DecuBlack", 11); c.setFillColor(PINK); c.drawString(0.75 * inch, h - 0.28 * inch, "DECU")
    c.setFillColor(ORANGE); c.drawString(0.75 * inch + pdfmetrics.stringWidth("DECU", "DecuBlack", 11), h - 0.28 * inch, "6000")
    c.setFont("DecuMono", 8.5); c.setFillColor(colors.HexColor("#D8D2FF"))
    c.drawRightString(w - 0.75 * inch, h - 0.28 * inch, "USER MANUAL  //  v" + VERSION)
    bw = w; c.setFillColor(ORANGE); c.rect(0, h - 0.45 * inch, bw * 0.5, 2, stroke=0, fill=1)
    c.setFillColor(PINK); c.rect(bw * 0.5, h - 0.45 * inch, bw * 0.333, 2, stroke=0, fill=1)
    c.setFillColor(CYAN); c.rect(bw * 0.833, h - 0.45 * inch, bw * 0.167, 2, stroke=0, fill=1)
    c.setFont("DecuMono", 8.5); c.setFillColor(MUTED)
    c.drawCentredString(w / 2, 0.45 * inch, str(doc.page))

# ---------------------------------------------------------------- content
story = [NextPageTemplate("body"), PageBreak()]

# contents
story += [P("Contents", "h1")]
toc = ["1  Welcome", "2  Installing", "3  Quick start", "4  The editor", "5  Pads, keyboards and MIDI learn",
       "6  The voices", "7  Master and gated reverb", "8  The Bio-Neural Engine", "9  Patches",
       "10  Sound design tips", "11  Troubleshooting", "12  Credits and license"]
story += [P(t, "toc") for t in toc]
story += [Spacer(1, 14), note("<b>New to plug-ins?</b> Read sections 2 and 3 first - they get you from download to "
                              "hitting pads in a few minutes. Everything else is reference you can dip into.", ORANGE),
          PageBreak()]

# 1 welcome
story += [P("1  Welcome", "h1"),
          P("Decu6000 is a drum synthesizer inspired by mid-90s cyber-industrial, glitch and trip-hop production. "
            "Every sound is synthesized in real time - there are no samples - so each of the 16 voices can be "
            "bent far beyond a normal drum kit."),
          P("It is designed around a <b>4x4 MIDI pad controller</b> such as the Akai MPD18, and works equally well "
            "with a MIDI keyboard or a DAW's piano roll."),
          P("What's inside", "h2")]
story += bullets([
    "<b>11 machine voices</b> - kick, snare, closed and open hi-hats, crash, ride, three toms and two FM percussion voices.",
    "<b>5 bio-neural voices</b> - Crystal Impact, Liquid Metal, Synthetic Breath, Digital Perc and Bio Burst: "
    "glassy, liquid, vocal, glitchy and granular sounds you won't find in a classic drum machine.",
    "<b>Gated reverb</b> on the master, fed by the bio-neural voices' SEND knobs.",
    "<b>Bio-Neural Engine</b> - a global instability layer that \"awakens\" the longer you play: glitch stutters, "
    "crushing, tape-style wow, timing drift and dropped hits.",
    "<b>Two views</b> - a rack of every voice, or a pad grid laid out like your controller.",
    "<b>MIDI learn</b> for pads and knobs, pad layout presets for pads, keyboards and General MIDI.",
    "<b>Patches</b> - 7 factory patches plus your own, shared between the plug-in and the standalone app.",
])
story += [PageBreak()]

# 2 installing
story += [P("2  Installing", "h1"),
          P("Decu6000 comes in two forms: a <b>VST3 plug-in</b> you load into a DAW (Reaper, Bitwig, Ableton Live, "
            "Cubase, FL Studio, Ardour...) and a <b>Standalone</b> app you can play without a DAW."),
          P("Windows (64-bit)", "h2")]
story += bullets([
    "<b>VST3:</b> copy the whole <font face='DecuMono'>Decu6000.vst3</font> folder to "
    "<font face='DecuMono'>C:\\Program Files\\Common Files\\VST3\\</font> (Windows asks for administrator "
    "permission). Rescan plug-ins in your DAW - Decu6000 appears as an instrument.",
    "<b>Standalone:</b> double-click <font face='DecuMono'>Decu6000.exe</font>. No installation needed.",
    "The first time, Windows may show <i>\"Windows protected your PC\"</i> because the app isn't signed with a paid "
    "certificate. Click <b>More info</b>, then <b>Run anyway</b>.",
])
story += [P("Linux (x86_64)", "h2")]
story += bullets([
    "<b>VST3:</b> copy <font face='DecuMono'>Decu6000.vst3</font> to <font face='DecuMono'>~/.vst3/</font> and rescan "
    "in your DAW (Bitwig, Reaper, Ardour, Carla...).",
    "<b>Standalone:</b> run <font face='DecuMono'>./Decu6000</font> (if needed, first "
    "<font face='DecuMono'>chmod +x Decu6000</font>). It supports ALSA and JACK; with PipeWire, "
    "<font face='DecuMono'>pw-jack ./Decu6000</font> also works.",
])
story += [P("Setting up the Standalone", "h2"),
          P("Click <b>Options</b> (top-left of the window), then <b>Audio/MIDI Settings...</b>"),
          ]
story += bullets([
    "<b>Output:</b> choose your audio interface. On Windows, <i>Windows Audio (Exclusive Mode)</i> gives the lowest "
    "latency. A buffer of 128-256 samples is a good start.",
    "<b>MIDI input:</b> tick your MPD18 or keyboard under <i>Active MIDI inputs</i>.",
])
story += [Spacer(1, 6), note("<b>Building it yourself:</b> Decu6000 is open source. The README explains how to build it "
                             "with Visual Studio or on Linux, or how to let GitHub build Windows and Linux versions "
                             "for you automatically."), PageBreak()]

# 3 quick start
story += [P("3  Quick start", "h1")]
story += bullets([
    "<b>1. Load it.</b> Add Decu6000 to an instrument track in your DAW, or open the Standalone.",
    "<b>2. Hear it.</b> Click any trigger pad in the window - each one plays its voice.",
    "<b>3. Play it.</b> Hit the pads on your controller. An MPD18 (and most 4x4 pad controllers) works straight away: "
    "pad 1, bottom-left, is the kick.",
    "<b>4. Using a keyboard?</b> Open <b>PAD LAYOUT...</b> in the header and choose <i>Keyboard: from C2</i> or "
    "<i>from middle C</i> - or <i>General MIDI drum map</i> if your DAW's drum lanes use GM.",
    "<b>5. Try patches.</b> Click the arrows either side of the LCD to step through the factory patches.",
    "<b>6. Make it yours.</b> Turn knobs (the LCD shows the value), then click the LCD and choose <b>Save As...</b>",
])
story += [Spacer(1, 4), P("Default pad layout (MPD18 bank A)", "h2"),
          P("Pads are numbered from the bottom-left, left to right, then upward - like the hardware.")]
grid = [["Crystal Impact<br/>48 C2", "Liquid Metal<br/>49 C#2", "Synthetic Breath<br/>50 D2", "Digital Perc<br/>51 D#2"],
        ["Ride<br/>44 G#1", "Perc 1<br/>45 A1", "Perc 2<br/>46 A#1", "Bio Burst<br/>47 B1"],
        ["Lo Tom<br/>40 E1", "Mid Tom<br/>41 F1", "Hi Tom<br/>42 F#1", "Crash<br/>43 G1"],
        ["Kick<br/>36 C1", "Snare<br/>37 C#1", "Closed HH<br/>38 D1", "Open HH<br/>39 D#1"]]
gt = Table([[Paragraph(c, ParagraphStyle("g", parent=S["cellb"], alignment=TA_CENTER, leading=13)) for c in r] for r in grid],
           colWidths=[1.55 * inch] * 4, rowHeights=[0.62 * inch] * 4)
gs = [("GRID", (0, 0), (-1, -1), 1.2, colors.white), ("VALIGN", (0, 0), (-1, -1), "MIDDLE")]
row_col = [colors.HexColor("#D8F8FC"), colors.HexColor("#FFD9EA"), colors.HexColor("#FFE6D2"), colors.HexColor("#FFD3B3")]
for r in range(4): gs.append(("BACKGROUND", (0, r), (-1, r), row_col[r]))
gs.append(("BACKGROUND", (3, 1), (3, 1), row_col[0]))
gt.setStyle(TableStyle(gs))
story += [gt, P("Note numbers with names (middle C = C3, the Akai / Ableton convention).", "cap"), PageBreak()]

# 4 editor
story += [P("4  The editor", "h1"),
          P("The header is the same in both views:")]
story += bullets([
    "<b>RACK VIEW / PAD VIEW</b> - switch between the two layouts (remembered with your project).",
    "<b>LCD</b> - shows the current patch, e.g. <font face='DecuMono'>PATCH 03 // RUST BELT</font>. A "
    "<font face='DecuMono'>*</font> means you've changed it since loading. While you turn a knob it shows the "
    "value. Click it for the patch menu; the arrows either side step through patches.",
    "<b>PAD LAYOUT...</b> - presets for which MIDI notes play which pads (section 5).",
    "<b>MIDI LEARN</b> - assign keys, pads and controller knobs (section 5).",
])
story += [shot("rack_view.png", caption="Rack view: machine voices on top; bio-neural voices, the Bio-Neural Engine "
                                         "and the master section below."),
          P("Rack view", "h2"),
          P("Every voice is a column with its knobs and a trigger pad. The pad shows the MIDI note that plays it and "
            "flashes when it's hit. Click it to hear the voice.")]
story += [CondPageBreak(3.5 * inch),
          shot("pad_view.png", caption="Pad view: the 4x4 grid, the selected voice's knobs, master and engine."),
          P("Pad view", "h2"),
          P("The 4x4 grid matches your controller. <b>Click a pad</b> to hear it and to edit it: its knobs appear "
            "large on the right. Master and Bio-Neural Engine controls sit underneath.")]
story += [P("Handy everywhere", "h2")]
story += bullets([
    "<b>Double-click</b> a knob to reset it to its default.",
    "<b>Right-click</b> a knob to MIDI-learn it, clear its controller, or reset it.",
    "<b>Hover</b> over a knob to see its full name.",
    "<b>Resize</b> the window by dragging its bottom-right corner; it keeps its shape and remembers its size.",
])
story += [PageBreak()]

# 5 MIDI
story += [P("5  Pads, keyboards and MIDI learn", "h1"),
          P("Every pad on a controller sends a MIDI <i>note number</i>. Decu6000 keeps a map of which note plays "
            "which voice. A voice can answer to several notes (in the General MIDI map, the kick plays on 35 and 36)."),
          P("Pad layout presets", "h2")]
story += [table([["PRESET", "NOTES", "USE IT FOR"],
                 ["Pads: notes 36-51", "36 to 51 in a row", "MPD18 and most 4x4 pad controllers (default)"],
                 ["Keyboard: from C2", "48 to 63", "Playing from a MIDI keyboard, lower octave"],
                 ["Keyboard: from middle C", "60 to 75", "Playing from a MIDI keyboard around middle C"],
                 ["General MIDI drum map", "GM standard", "DAW drum lanes, e-kits, GM drum MIDI files"]],
                [1.9 * inch, 1.5 * inch, 3.1 * inch])]
story += [P("General MIDI note assignments", "h3")]
gm = [["VOICE", "NOTES", "VOICE", "NOTES"],
      ["Kick", "35, 36", "Ride", "51, 59"], ["Snare", "38, 40", "Perc 1 (side stick)", "37"],
      ["Closed HH", "42, 44", "Perc 2 (clap)", "39"], ["Open HH", "46", "Crystal Impact", "52"],
      ["Lo Tom", "41, 43", "Liquid Metal", "53"], ["Mid Tom", "45, 47", "Bio Burst", "54"],
      ["Hi Tom", "48, 50", "Synthetic Breath", "55"], ["Crash", "49, 57", "Digital Perc", "56"]]
story += [table(gm, [1.5 * inch, 1.1 * inch, 1.9 * inch, 2.0 * inch])]
story += [P("MIDI learn: pads and keys", "h2")]
story += bullets([
    "Click <b>MIDI LEARN</b>. Every pad and knob gets a yellow outline with its current note "
    "(<font face='DecuMono'>N 36</font>) or controller (<font face='DecuMono'>CC 21</font>).",
    "Click a <b>pad</b>, then hit any key or pad on your controller. That note now plays the voice. It replaces "
    "the pad's previous notes and is taken away from any other pad that used it.",
    "Click the next pad to keep going. <b>Esc</b> cancels; click MIDI LEARN again when you're done.",
])
story += [P("MIDI learn: knobs (CC)", "h2")]
story += bullets([
    "In MIDI LEARN mode, click a <b>knob</b>, then move a knob or fader on your controller.",
    "Each knob listens to one controller (CC); moving a CC to a new knob frees the old one.",
    "Outside learn mode you can also right-click any knob and choose <b>Learn MIDI CC...</b>",
    "In learn mode, right-click a pad or knob to clear its mapping.",
])
story += [shot("learn_mode.png", caption="MIDI LEARN mode: every mapping shown at a glance; the armed knob waits for a controller."),
          note("Note and CC mappings are saved with your <b>project</b>, not with patches - they describe your "
               "controller, so they stay put when you change sounds.", ORANGE), PageBreak()]

# 6 voices
story += [P("6  The voices", "h1"),
          P("All voices respond to velocity (how hard you hit). Several also get brighter when hit harder. "
            "Ranges and defaults below are exactly what the knobs show.")]
V = [
    ("Kick", "kick", "Pad 1 - note 36",
     "A sine kick with a pitch sweep. Drive comes before the decay envelope, so the tail stays distorted; Drone feeds "
     "the tail back into itself until it can hold like a drone.",
     D(pitch="Fundamental pitch the sweep settles on.", drive="Saturation. Grows stronger with longer Decay settings.",
       drone="Tuned feedback on the tail - from a normal kick to a sustained, growling drone.")),
    ("Snare", "snare", "Pad 2 - note 37",
     "Two tuned tones for the drum body plus filtered noise for the snares.",
     D(tune="Pitch of the drum body.", snappy="Amount (and length) of the noise 'snares'.",
       tone="Brightness of the snares.")),
    ("Closed / Open Hi-Hat", "hhc", "Pads 3 and 4 - notes 38, 39",
     "808-style metallic hats: six detuned square waves blended with noise. The closed hat cuts off (chokes) a "
     "ringing open hat. Both hats have the same controls; the table shows the closed hat's defaults "
     "(open hat decay: 100 ms to 2 s, default 600 ms).",
     D(tune="Pitch of the metallic oscillators (in semitones).", tone="Brightness (cut-off of the high-pass filter).",
       metal="Blend from pure noise (0%) to metallic clang (100%).")),
    ("Crash / Ride", "crash", "Pads 8 and 9 - notes 43, 44",
     "Ring-modulated cymbals: pairs of square waves multiplied together for dense, clangy overtones, plus a noise wash "
     "and a strike burst. The Ride is the same engine with a longer, brighter, more metallic default.",
     D(tune="Pitch of the metallic oscillators (in semitones).", tone="Brightness.",
       ring="Blend from noise wash (0%) to ring-modulated clang (100%).")),
    ("Lo / Mid / Hi Tom", "lotom", "Pads 5, 6, 7 - notes 40, 41, 42",
     "Electronic toms with an adjustable pitch 'zap'. Each tom has its own full set of controls; the table shows the "
     "Lo Tom (Mid: 60-240 Hz, default 123 Hz; Hi: 90-360 Hz, default 184 Hz).",
     D(tune="Pitch the tom settles on after the bend.",
       bend="Pitch drop at the start: flat (0%) to a long Simmons-style zap sweeping down from about 3 octaves up (100%).",
       noise="Short stick-click at the start of the hit.")),
    ("Perc 1 / Perc 2", "perc1", "Pads 10 and 11 - notes 45, 46",
     "Two-operator FM percussion. Perc 1 starts as a woody click, Perc 2 as a metallic blip; both have the same "
     "controls (Perc 2 defaults: 540 Hz, 300 ms, ratio 1.41:1, index 55%).",
     D(pitch="Pitch of the carrier tone.",
       ratio="Modulator-to-carrier frequency ratio. Whole numbers (1:1, 2:1, 3:1) sound woody and tonal; in-between "
             "values (1.41:1, 2.76:1) sound like bells, cowbells and metal.",
       index="FM depth: how buzzy and bright the attack is. Harder hits are brighter.")),
    ("Crystal Impact", "crystal", "Pad 13 - note 48",
     "Modal synthesis of struck glass: eight ringing partials, the higher ones dying faster, plus a tiny glass 'glint'.",
     D(inharm="Partial spacing: evenly spaced and bell-like (0%) to the stretched spacing of a glass bar (100%).",
       shimmer="Adds slightly detuned twins of every partial; they beat against each other so the tail glitters.")),
    ("Liquid Metal", "liquid", "Pad 14 - note 49",
     "A metallic FM tone that drips and wobbles.",
     D(drip="Rising pitch sweep at the start - the 'bloop' of a falling drop - up to two octaves.",
       modrate="Speed of the warble in pitch and brightness that makes the tail move like molten metal.")),
    ("Synthetic Breath", "breath", "Pad 15 - note 50",
     "Formant synthesis - a robotic throat. A buzzy source and breath noise are shaped into vowel sounds; each hit is "
     "a short exhale.",
     D(pitch="Pitch of the buzzy 'voice'.", vowel="Sweeps the vowel: U - O - A - E - I (dark to bright).",
       breath="Blend from buzzy robot voice (0%) to whispered gasp (100%).")),
    ("Digital Perc", "digital", "Pad 16 - note 51",
     "A short blip pushed through a wavefolder, with a chaotic digital source mixed in.",
     D(chaos="Blends in a chaotic number generator and makes the pitch jump between octaves and fifths - from clean "
             "to broken machine.",
       fold="Wavefolding: from a clean tick (0%) to harsh, buzzy metal (100%). Harder hits fold more.")),
    ("Bio Burst", "burst", "Pad 12 - note 47",
     "A swarm of tiny sound grains released by each hit.",
     D(pitch="Centre pitch of the swarm.", decay="How long grains keep spawning.",
       swarm="How many grains and how widely their pitches scatter: sparse chirps (0%) to a dense insect swarm (100%).",
       grain="Grain length: clicky static (0%) to buzzing, tonal chirps (100%).")),
]
for i, (title, prefix, where, blurb, desc) in enumerate(V):
    accent = PANEL if i < 6 else colors.HexColor("#1F6F86")
    block = [P(f"{title}  <font face='DecuMono' size='9' color='#5E5885'>// {where}</font>", "h2"), P(blurb),
             controls(prefix, desc, accent)]
    story += [CondPageBreak(2.6 * inch)] + block
story += [PageBreak()]

# 7 master
story += [P("7  Master and gated reverb", "h1"),
          P("The master section sets the overall level and runs the <b>gated reverb</b> - the classic 80s/90s drum "
            "verb that opens on a hit, holds, then slams shut instead of fading. The bio-neural voices feed it through "
            "their SEND knobs."),
          controls("master", {
              "level": "Overall output level. A soft limiter after it keeps loud, stacked hits from clipping harshly.",
              "reverb": "Level of the reverb you hear (return).",
              "gate": "How long the reverb stays open after a hit before it's cut off.",
              "predelay": "Gap before the reverb starts - pushes the reverb behind the dry hit."}, colors.HexColor("#8E1D57")),
          Spacer(1, 8),
          P("The two meters show the output level. The master soft limiter only acts above about -2 dBFS, so normal "
            "playing is untouched.")]

# 8 engine
story += [PageBreak(), P("8  The Bio-Neural Engine", "h1"),
          P("A global instability layer over the whole kit. It \"awakens\": the longer and busier you play, the more "
            "unstable things get; stop playing and it relaxes. The screen on the engine panel draws this live, with a "
            "pink line for each glitch stutter."),
          note("<b>Off means off.</b> With LUCID / UNSTABLE at 0 or BLEND at 0 the engine is completely bypassed and "
               "your sound is untouched. INIT starts with LUCID at 0.", PINK),
          Spacer(1, 6),
          P("LUCID versus BLEND", "h2"),
          P("Think of a distortion pedal. <b>LUCID / UNSTABLE</b> is the <i>drive</i>: how hard the engine works - "
            "how often it glitches, how deep it crushes, how far the timing drifts. <b>BLEND</b> is the <i>mix</i>: how "
            "much of the engine you hear next to your clean kit. High LUCID with low BLEND gives wild glitching tucked "
            "behind a solid beat; low LUCID with high BLEND gives a subtly worn, lo-fi kit."),
          controls("engine", {
              "blend": "How much of the engine you hear. All the way left = off; all the way right = full effect. "
                       "Crush, bits, wow and stutters mix in parallel with the dry kit; PROB and MICRO-T scale down with it.",
              "lucid": "How unstable the engine gets, and how strongly it awakens as you play.",
              "grain": "Chance of glitch stutters - a short slice of the output repeats 2 to 7 times.",
              "crush": "Sample-rate reduction.",
              "bits": "Bit depth, in whole steps from 16 bits down to 3.",
              "rate": "Speed of a drifting, tape-style pitch wow (a random wander, not a regular wobble).",
              "microt": "Random timing push on each hit - human feel at small settings, drunken at large ones.",
              "prob": "Chance that each hit plays at all. 100% = every hit.",
              "seed": "Starting point of the randomness. Click the SEED display for a new one; the same seed repeats the "
                      "same glitches, so patches sound the same every time."}, colors.HexColor("#1F6F86")),
          PageBreak()]

# 9 patches
story += [P("9  Patches", "h1"),
          P("A patch stores every sound setting - all voices, the master section and the Bio-Neural Engine.")]
story += bullets([
    "<b>Browse:</b> click the arrows beside the LCD, or click the LCD for the full list.",
    "<b>Save:</b> click the LCD and choose <b>Save As...</b> to name a new patch, or <b>Save</b> to update the "
    "user patch you're on. <b>Delete</b> removes a user patch (the sound stays loaded).",
    "<b>Your DAW's preset menu</b> lists the same patches.",
    "Your project remembers which patch was loaded, including unsaved changes.",
    "Pad layouts and MIDI CC assignments are <i>not</i> part of a patch.",
])
story += [P("Where patches are stored", "h3"),
          table([["SYSTEM", "FOLDER"],
                 ["Windows", "%APPDATA%\\Decu6000\\Patches   (paste into the Explorer address bar)"],
                 ["macOS", "~/Library/Decu6000/Patches"],
                 ["Linux", "~/.config/Decu6000/Patches"]], [1.2 * inch, 5.3 * inch]),
          P("Patches are small <font face='DecuMono'>.decupatch</font> files: copy them between computers or share "
            "them. The plug-in and Standalone share the same folder. Older patches load fine in newer versions - "
            "controls added later simply start at their defaults."),
          P("Factory patches", "h2"),
          table([["PATCH", "CHARACTER"],
                 ["INIT", "Every control at its default. Engine asleep. A clean starting point."],
                 ["HOLOGRAM DIVA", "Bright and glassy: shimmering Crystal Impact, metallic hats, generous gated reverb, "
                                   "a light touch of the engine."],
                 ["RUST BELT", "Industrial: overdriven droning kick, crushed snare and hats, zapping toms, "
                               "clanging cymbals, crunchy engine."],
                 ["TRIP HOP 94", "Dusty and laid back: low, soft snare, darker hats, gentle toms, lo-fi sampler "
                                 "wobble and a longer reverb."],
                 ["GLITCH RITUAL", "Broken machines: heavy crush, extreme FM percs, full-bend toms, and a strongly "
                                   "awake engine with stutters and dropped hits."],
                 ["SUBMERGED", "Deep and wet: sub kick drone, dark tones, dripping Liquid Metal, long open reverb."],
                 ["AWAKENING", "The engine at full tilt - play for a while and listen to it come alive."]],
                [1.5 * inch, 5.0 * inch]),
          PageBreak()]

# 10 tips
story += [P("10  Sound design tips", "h1")]
tips = [
    ("Industrial kick", "Kick: Pitch 40 Hz, Decay 1.2 s, Drive 80%, Drone 30-40%. Add Engine CRUSH for grit."),
    ("Trip-hop snare", "Snare: Pitch 150 Hz, Snap 40%, Tone 30%, Crush 20-30%, Decay 350 ms."),
    ("Simmons tom fill", "Toms: Bend 70-100%, Decay 600 ms+, a little Drive. Play Hi-Mid-Lo quickly."),
    ("Cowbell from Perc 2", "Pitch about 560 Hz, Ratio 1.41:1, Index 50%, Decay 250 ms."),
    ("Woodblock / clave", "Perc 1: Ratio 1:1 or 2:1, Index 10-25%, Decay 30-70 ms, Pitch 700 Hz-1.5 kHz."),
    ("Gated 80s snare", "Send Crystal or Breath to the reverb, Gate T 150-250 ms, Reverb 60%+."),
    ("Robot vocal hits", "Synthetic Breath: Breath 0%, Vowel swept per hit with a MIDI-learned knob, Pitch 100-200 Hz."),
    ("Data-corruption hi-hat", "Closed HH Crush 60%+, Digital Perc with Chaos 80% layered underneath at low Level."),
    ("Humanise, don't glitch", "Engine: LUCID 30%, BLEND 100%, GRAIN 0, CRUSH 0, BITS 0, MICRO-T 8-12 ms, "
                               "PROB 100% - just a loose, human feel."),
]
story += [table([["IDEA", "TRY THIS"]] + [[f"<b>{a}</b>", b] for a, b in tips], [1.7 * inch, 4.8 * inch])]
story += [Spacer(1, 8), note("<b>Keep a knob under your hand:</b> MIDI-learn the engine's LUCID or a voice's Decay to a "
                             "controller knob and ride it while you play.", ORANGE)]

# 11 troubleshooting
story += [P("11  Troubleshooting", "h1")]
trouble = [
    ("No sound from the Standalone", "Options > Audio/MIDI Settings: check the output device is selected and not "
                                     "muted. On Linux, try another ALSA device, or start JACK / use pw-jack."),
    ("Pads trigger the wrong sounds", "Your controller sends different notes. Choose a preset under PAD LAYOUT, or "
                                      "use MIDI LEARN to assign each pad."),
    ("Only some keys make sound", "With the default layout only notes 36-51 are mapped. Choose a Keyboard layout or "
                                  "learn the keys you want."),
    ("Hits sometimes don't play", "The Bio-Neural Engine's PROB is below 100% with LUCID up. Raise PROB or lower BLEND."),
    ("Timing feels loose", "Engine MICRO-T is pushing hits late. Turn it down (or BLEND down)."),
    ("Unexpected crunch or stutters", "The engine is awake. Set LUCID or BLEND to 0 to switch it off."),
    ("DAW doesn't list Decu6000", "Check the .vst3 folder is in your VST3 folder (section 2) and rescan plug-ins."),
    ("Linux build won't start", "The ready-made build needs a 2023-or-newer distro (glibc 2.38+). Use the build from "
                                "the GitHub workflow, or build from source."),
    ("Windows says the app is unsafe", "Click More info > Run anyway. The app isn't signed with a paid certificate."),
]
story += [table([["PROBLEM", "WHAT TO DO"]] + [[f"<b>{a}</b>", b] for a, b in trouble], [2.0 * inch, 4.5 * inch])]
story += [PageBreak()]

# 12 credits
story += [P("12  Credits and license", "h1"),
          P("Decu6000 is free software: you can redistribute it and/or modify it under the terms of the "
            "<b>GNU Affero General Public License</b>, version 3 or (at your option) any later version. It is "
            "distributed in the hope that it will be useful, but <b>without any warranty</b>; see the LICENSE file "
            "for details."),
          P("In short: you can use, study, change and share Decu6000. If you distribute a modified version, you must "
            "make its source code available under the same license."),
          P("Source code: <font face='DecuMono'>https://github.com/LexLongcoding/Decu6000</font>"),
          P("Built with", "h2")]
story += bullets([
    "<b>JUCE 8</b> (juce.com), used under the AGPLv3.",
    "<b>VST 3 SDK</b> by Steinberg Media Technologies GmbH. VST is a trademark of Steinberg Media Technologies GmbH; "
    "Decu6000 is not affiliated with Steinberg.",
    "<b>Orbitron</b> by The League of Moveable Type (instanced and renamed \"Decu Display\") and <b>Share Tech Mono</b> "
    "by Carrois Type Design - both under the SIL Open Font License 1.1.",
])
story += [Spacer(1, 10), P(f"Decu6000 - Copyright (C) 2026 LexLongcoding. Manual for version {VERSION}.", "cap")]

# ---------------------------------------------------------------- build
doc = BaseDocTemplate(OUT, pagesize=letter, leftMargin=inch, rightMargin=inch, topMargin=0.85 * inch,
                      bottomMargin=0.8 * inch, title="Decu6000 User Manual", author="LexLongcoding",
                      subject=f"Decu6000 {VERSION} - bio-neural percussion synthesizer", creator="docs/manual/make_manual.py")
frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")
doc.addPageTemplates([PageTemplate(id="cover", frames=[frame], onPage=on_cover),
                      PageTemplate(id="body", frames=[frame], onPage=on_page)])
doc.build(story)
print("wrote", OUT, "-", len(SPECS), "parameters documented")
