home_automation_hub  -  DESN2000 Home Automation Hub
====================================================

A prototype "Home Automation & Control Hub" for a holiday cottage (Jindabyne, NSW),
built on the Keil QVGA Base Board + DESN2000 Daughter Board (LPC2478, ARM7TDMI).
The code is split into independent modules, one owner per section (see the team
breakdown in ../README.md). This file documents what each module does and where
each graded feature lives.


SECTION 1 - SENSORS & FIXTURES I/O   (owner: Jack)   -- COMPLETE
--------------------------------------------------------------------
Reads the ambient light sensor and drives/reads the mimicked home fixtures
(tricolor blind LEDs + the two buttons) required by ProjectBrief.md Activity 2.

  Feature (grading category)          File . function                    Brief ref
  ----------------------------------  ---------------------------------  ----------
  Read the light sensor (2 pts)       light.c . light_init / light_read  Activity 2
  Blind fixtures - tricolor LEDs (1)  led.c   . led_init  / blind_set     Activity 2
  Button fixtures - S1 plug/S2 bell   button.c. button_init /            Activity 2
                                                button_read / button_edge

  Module summaries:
    light.c   - ambient light sensor via the ADC. light_init() powers and
                configures the converter; light_read() runs one conversion and
                returns a 12-bit value (0 = dark .. 4095 = bright).
    led.c     - the two tricolor "blind" LEDs. led_init() makes the six pins
                outputs; blind_set(which, state) shows a blind's position as a
                colour. ProjectBrief Activity 2: rolled up = red, mid-way =
                green, rolled down = blue.
    button.c  - the two fixture buttons. button_init() sets them as inputs;
                button_read(id) gives the live level; button_edge(id) returns 1
                exactly once per press (rising edge) so a held button is a single
                event.


PINS USED BY SECTION 1
----------------------
  Light sensor -> ADC channel AD0.1  (pin P0.24).
      NOTE: the board's working code selects channel 1 (PINSEL1 bits 17:16 = 01,
      pin P0.24) and reads AD0DR1. Earlier notes said "AD0.2 / P0.25"; the code
      that runs on the real board settles it as AD0.1 / P0.24.
  Blind LED 1 (D9)  -> P3.16 (R) / P3.17 (G) / P3.18 (B), common-cathode, active-high.
  Blind LED 2 (D10) -> P3.19 (R) / P3.20 (G) / P3.21 (B), common-cathode, active-high.
  Button S1 (smart plug override) -> P0.10, active-high, NO hardware debounce.
  Button S2 (doorbell)            -> P0.11, active-high, hardware debounced (Schmitt).


DEBOUNCE PLAN (Section 1)
-------------------------
  button_edge() currently does edge detection only: it fires once per clean 0->1
  transition, which is all S2 needs because S2 is hardware-debounced.

  S1 has no hardware debounce, so a single press can bounce into several rising
  edges. Robust software debounce wants a steady sample rate (e.g. sample every
  1 ms and require the level stable for ~10 ms). That 1 ms tick comes from
  Section 2's Timer1 clock, so the S1 stability filter is intentionally deferred
  and layered into button_edge() once that clock exists. Until then, S1 relies on
  edge detection alone. (See the comment above button_edge in button.c.)


VERIFICATION STATUS
-------------------
  VERIFIED on the real board: the light sensor ADC read path (carried over from
      the working light_sensor_demo).
  SCHEMATIC-ONLY, test early on hardware: the tricolor LED polarity (active-high)
      and the two button pins/levels were taken from
      I.ELEC2142_Daughter_Board_Schematics.pdf and have not yet been exercised on
      a board. First on-board checks to run:
        * blind_set(1, UP/MID/DOWN) cycles Blind 1 red -> green -> blue (repeat for 2)
        * press S1 -> button_edge(BTN_PLUG) returns 1 once per press
        * press S2 -> button_edge(BTN_DOORBELL) returns 1 once per press


OTHER SECTIONS (see ../README.md for owners)
--------------------------------------------
  Section 2 - Sound & Time         : speaker.c/.h (doorbell chime), clock.c/.h  [pending]
  Section 3 - Automation & Assembly: decision.s (ARM assembly), automation.c/.h [pending]
  Section 4 - LCD Touchscreen UI   : lcd/ + touch.c/.h + ui.c/.h                [pending]

  main.c is currently a thin test harness for Section 1. Section 3 owns the final
  main.c integration (the super-loop that ties every module together).


HOW TO BUILD AND RUN
--------------------
  1. Open  home_automation_hub.uvproj  in Keil uVision4.
  2. Target is "QVGA Base Board". Build (F7).
  3. Every .c file (light.c, led.c, button.c, main.c) must be in the "Source Code"
     group, or the linker reports "L6218E: undefined symbol". Headers are included,
     not added.
  4. Load onto the board and run.
