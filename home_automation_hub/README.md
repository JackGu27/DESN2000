home_automation_hub  -  DESN2000 Home Automation Hub
====================================================

WHAT IT DOES
------------
Live proof that the ambient light sensor works:
  * The brighter the light on the sensor, the MORE LEDs light up (a bar graph).
  * A "dark" threshold fires the automation logic (our "cottage lights ON"
    decision) - showing the sensor drives the hub, not just reacts.
  * Optional: the brighter the light, the HIGHER the speaker pitch
    (set USE_SPEAKER to 1 in main.c).

WHERE THE FEATURES ARE
----------------------
  src/main.c      All of the demo code, grouped into small commented functions:
                    light_init() / light_read()   - ADC on the light sensor
                    leds_init()  / leds_bar()      - LED bar graph
                    speaker_init/udelay/chirp()    - optional speaker pitch
                    main()                         - read -> display -> decide
  src/LPC2400.s   Board start-up (from the lab template; do not edit).
  src/lpc24xx.h   LPC2478 register definitions (from the lab template).

HARDWARE / PINS (Keil QVGA Base Board + DESN2000 Daughter Board, LPC2478)
------------------------------------------------------------------------
  Light sensor -> ADC channel AD0.2  (pin P0.25).
      Confirmed from the board's shipped demo (lab1 LEDCycle.c) and the
      LPC2478 Pin Connect Block (PINSEL1 bits 19:18 = 01 -> AD0.2).
  LEDs         -> the daughter board's 8-LED vertical ladder on GPIO Port 2,
      pins P2.1..P2.8 (schematic nets LLAD1..LLAD8). The bar grows bottom to
      top as the light increases. Confirmed from the daughter board schematic
      (I.ELEC2142_Daughter_Board_Schematics.pdf).
      Ladder enable: P0.22 (LLAD_EN) drives a 74HC2G14 inverter that sinks the
      common-cathode current; the code drives P0.22 HIGH to enable it.
      (The two tri-colour LEDs are P3.16-18 = LED1 R/G/B and P3.19-21 = LED2
      R/G/B, if you want a colour status indicator instead.)
  Speaker      -> DAC output AOUT (pin P0.26), Timer0 for timing (as in lab5).

HOW TO BUILD AND RUN
--------------------
  1. Open  home_automation_hub.uvproj  in Keil uVision4.
  2. Target is "QVGA Base Board". Build (F7).
  3. Load onto the board and run. Cover the sensor -> fewer LEDs; shine a
     phone torch on it -> more LEDs.
  4. To find/tune DARK_THRESHOLD: run in Debug mode, cover vs light the sensor,
     and watch the value returned by light_read() (or the AD0DR2 register).

NOTES
-----
  * This project was not compiled outside Keil (no ARM toolchain here), so do a
    build in uVision before the presentation. The register setup mirrors the
    working lab examples (LEDCycle.c for the ADC, lab5 for the DAC).
  * The ADC gives a 12-bit value (0 = dark .. 4095 = bright).
