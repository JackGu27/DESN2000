# DESN2000 — Home Automation & Control Hub

ARM (LPC2478 / Keil QVGA Base Board + DESN2000 Daughter Board) code for the holiday-cottage
home automation hub. We build by growing the **working `light_sensor_demo`** into a modular
project called `home_automation_hub`.

**Code deliverable is worth 10 pts.** Each section below maps to a graded category. Everything
reuses a known-good weekly lab, so nobody starts from a blank page.

> ⚠️ **Tutor requirement:** not everything may be in C — at least one module must be **ARM
> assembly**. That is covered by **Section 3** (the automation decision logic).

---

## ⚠️ Post-merge integration fixes (action required)

After merging all four sections, a read-through found **4 integration bugs**. Each module works
on its own, but our sections assumed **different conventions** and never lined up. They all
*compile* (some only warn), but they misbehave on the board. Root cause: `led.h`,
`automation.h`/`decision.s`, and `clock.c` each invented their own numbering/units.
**Fix = one source of truth.**

| # | Bug | Symptom | Fix | Owner |
|---|-----|---------|-----|-------|
| 1 | 🔴 Blind colours reversed | `led.h` uses `UP=0/MID=1/DOWN=2`; `automation.h`+`decision.s` use `UP=2/MID=1/DOWN=0`. Since `blind_set` lights pins in R/G/B order and the brief says up=red/mid=green/down=blue, **led.h is correct**. Currently dark→blue, bright→red (backwards). | `decision.s` returns `UP=0 / MID=1 / DOWN=2` (swap the `MOV R2,#2` and `MOV R2,#0`). | S3 |
| 2 | 🔴 The two blinds are swapped | `automation.h` has `BLIND_1=0/BLIND_2=1`, but `blind_set` expects `which==1` for Blind 1. So Blind 1's logic drives Blind 2's LED and vice-versa. | Agree blind numbering is **1 and 2**: `BLIND_1=1, BLIND_2=2`. | S3 |
| 3 | 🔴 Espresso preheats ~12:15 AM, not 3:30 PM | `clock_now()` returns **seconds** from midnight; `decision.s` treats time as **minutes** (930/990 = 15:30/16:30). Window lands 930–990 s after boot; the LCD would show 00:15 while the "15:30" preheat fires. | Keep units consistent — convert in `automation.c`: `automation_state.time = clock_now() / 60;` (or `clock_now()` returns minutes). | S2/S3 |
| 4 | 🟡 Macro redefinition warning | `main.c` includes both `led.h` and `automation.h`, which both `#define UP/MID/DOWN` to different values → Keil warns "incompatible redefinition". | Disappears once Bugs 1–2 fix is applied (below). | — |

**Single source of truth (fixes 1, 2 & 4 together):** `led.h` becomes the **only** place that
defines `UP/MID/DOWN` and `BLIND_1/BLIND_2` (values: `UP=0, MID=1, DOWN=2`, `BLIND_1=1,
BLIND_2=2`). `automation.h` should `#include "led.h"` and **delete its own copies** of those
defines. *(Jack does the led.h side; S3 updates automation.h + decision.s.)*

**For the demo (not a bug — do not rely on the unit mismatch):** we can't wait until 3:30 PM in
a 10-min slot. `clock.c` already has `clock_set_time()`, so at demo start call
`clock_set_time(15, 29, 50)` — the preheat fires ~10 s in **and** the clock display correctly
reads 15:29→15:30 (logically correct, per brief line 224). If we ever want *compressed* time,
make it an explicit `#define DEMO_MODE` and document it here — never an accidental side effect.

**Who does what:**
- **S1 (Jack):** make `led.h` the single source of truth (`UP=0/MID=1/DOWN=2`, `BLIND_1=1/BLIND_2=2`).
- **S3:** swap UP/DOWN in `decision.s`; `#include "led.h"` in `automation.h` (delete duplicate defines); add `clock_now()/60`.
- **S2:** confirm clock units — `clock_now()` can stay in seconds for the UI as long as automation converts.
- **Everyone:** add `clock_set_time(15,29,50)` at demo start; also use `button_edge(BTN_DOORBELL)` for the doorbell instead of reading `FIO0PIN` directly in `main.c`.

---

## Getting started (everyone, first)

1. Copy `light_sensor_demo/` → `home_automation_hub/` (keep `startup.c`, `LPC2400.s`, `lpc24xx.h`).
2. Open `home_automation_hub.uvproj` in Keil uVision, target **"QVGA Base Board"**, build (**F7**).
   Confirm the light-bar demo still runs on the board.
3. Agree the module header names (function signatures below) so we can work in parallel.
4. Add your new `.c` files to the Keil target: *right-click Source Group → Add Existing Files*.

---

## Work sections — claim one by putting your name in the box

### Section 1 — Sensors & Fixtures I/O  ✅ **Jack**  *(complete)*
- **Files:** `light.c/.h`, `led.c/.h`, `button.c/.h`
- **Does:** read the light sensor (ADC), drive the two tricolor blind LEDs, read the 2 buttons
- **Worth:** 3 pts (light sensor 2 + fixtures 1)
- **Reference:** light-sensor code already exists in the demo; `lab4/blinky` + `lab4/toggle` for GPIO
- **Pins:** ADC AD0.1 / P0.24 · blind LEDs P3.16–21 · buttons S1=P0.10, S2=P0.11

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

- **Light sensor (VERIFIED on board):** ADC AD0.1 on P0.24 (`PINSEL1` bits 17:16 = 01, reads `AD0DR1`)
- **Tricolor LEDs (schematic-only, test early):** common-cathode, **active-high** —
  LED1 R/G/B = P3.16/17/18, LED2 R/G/B = P3.19/20/21. Red = up, Green = mid, Blue = down
- **Buttons (schematic-only, test early):** S2 = P0.11 (doorbell, HW-debounced), S1 = P0.10 (smart plug, needs software debounce)
- **Speaker:** DAC AOUT = P0.26
