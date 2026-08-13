# DESN2000 — Home Automation & Control Hub

ARM (LPC2478 / Keil QVGA Base Board + DESN2000 Daughter Board) code for the holiday-cottage
home automation hub. We build by growing the **working `light_sensor_demo`** into a modular
project called `home_automation_hub`.

**Code deliverable is worth 10 pts.** Each section below maps to a graded category. Everything
reuses a known-good weekly lab, so nobody starts from a blank page.

> ⚠️ **Tutor requirement:** not everything may be in C — at least one module must be **ARM
> assembly**. That is covered by **Section 3** (the automation decision logic).

---

## Getting started (everyone, first)

1. Copy `light_sensor_demo/` → `home_automation_hub/` (keep `startup.c`, `LPC2400.s`, `lpc24xx.h`).
2. Open `home_automation_hub.uvproj` in Keil uVision, target **"QVGA Base Board"**, build (**F7**).
   Confirm the light-bar demo still runs on the board.
3. Agree the module header names (function signatures below) so we can work in parallel.
4. Add your new `.c` files to the Keil target: *right-click Source Group → Add Existing Files*.

---

## Work sections — claim one by putting your name in the box

### Section 1 — Sensors & Fixtures I/O  ✅ **Jack**
- **Files:** `light.c/.h`, `fixtures.c/.h`, `buttons.c/.h`
- **Does:** read the light sensor (ADC), drive the tricolor blind LEDs + ladder LEDs, read the 2 buttons
- **Worth:** 3 pts (light sensor 2 + fixtures 1)
- **Reference:** light-sensor code already exists in the demo; `lab4/blinky` + `lab4/toggle` for GPIO
- **Pins:** ADC AD0.2 / P0.25 · blind LEDs P3.16–21 · ladder P2.1–8 (enable P0.22) · buttons S1=P0.10, S2=P0.11

### Section 2 — Sound & Time  → **[ song ]**
- **Files:** `speaker.c/.h`, `clock.c/.h`
- **Does:** doorbell chime (DAC) + a software clock for time-of-day automation
- **Worth:** 1 pt (doorbell) + timekeeping the automation needs
- **Reference:** copy `lab5/play_song` (`play_tone.c`, `songs.c`). Speaker = Timer0, clock = Timer1
- **Pins:** DAC AOUT = P0.26

### Section 3 — Automation & Assembly  → **[ Zhengxi ]**  *(hardest; owns `main.c` integration)*
- **Files:** `decision.s` **(ARM assembly)**, `automation.c/.h`
- **Does:** the "smarts" — decide blinds/plug from light + time. Decision logic in assembly (satisfies tutor)
- **Worth:** 2 pts (automation) + fulfils the assembly requirement
- **Reference:** `lab5/max_of_5/max_of_5.s` for the C-calls-assembly (AAPCS) pattern

### Section 4 — LCD Touchscreen UI  → **Declan**  *(biggest, but mostly reuse)*
- **Files:** copy `lcd/` folder + `touch.c/.h` from `lab6/bubblepop`; write `ui.c/.h`
- **Does:** touchscreen dashboard — show light/time/blind+plug state, touch buttons to control fixtures
- **Worth:** 2 pts
- **Reference:** `lab6/bubblepop` is a complete LCD + touch example to copy from

---

## How the sections connect (interfaces to agree on Day 1)

| Owner | Exposes (called by others) |
|-------|----------------------------|
| S1 (Jack) | `light_read()`, `blind_set(which, UP/MID/DOWN)`, `button_edge(id)` |
| S2 | `chime()`, `clock_now()` |
| S3 | calls all of the above from `automation.c` + the `main.c` loop |
| S4 | reads the same state to draw the UI |

**Everyone:** comment your own files and write your part of this README's feature map — the tutor
grades comments and needs to find where each feature lives (2 pts).

---

## Confirmed pin map (verified where noted)

- **Light sensor (VERIFIED on board):** ADC AD0.2 on P0.25 (`PINSEL1` bits 19:18 = 01)
- **Ladder LEDs (VERIFIED on board):** GPIO P2.1–P2.8, enable by driving **P0.22 HIGH**
- **Tricolor LEDs (schematic-only, test early):** common-cathode, **active-high** —
  LED1 R/G/B = P3.16/17/18, LED2 R/G/B = P3.19/20/21. Red = up, Green = mid, Blue = down
- **Buttons (schematic-only, test early):** S2 = P0.11 (doorbell, HW-debounced), S1 = P0.10 (smart plug, needs software debounce)
- **Speaker:** DAC AOUT = P0.26
