# cLOCK-IN

A 4-key alarm clock built on a Seeed XIAO ESP32-C3 with a 284x76 ST7789 display and a buzzer. An alarm clock to wake you up to LOCK-IN!!

## Hardware
- XIAO ESP32-C3
- ST7789 284x76 display (8-pin: GND, 3V3, SCL, SDA, RST, DC, CS, BL)
- 4 push switches
- Buzzer

> **Note:** This ReadME is writtern by Syed Ihteshamuddin (human ofc), So if some mistakes are occurred please highlight.

## Overview 
---
I have spent over 8+ hours on this project. And I have learned alot while doing this project!

<img width="776" height="473" alt="cLOCK-IN pic" src="https://github.com/user-attachments/assets/5cc1883a-a56b-402e-8333-4d2315b5eb6c" />

---

## Project Gallery

### SCHEMATIC

<img width="1617" height="822" alt="cLOCK-IN Schematic" src="https://github.com/user-attachments/assets/cadcff59-ecb6-4b49-b388-dbd7e844914e" />

Schematic was the easiest thing to do in this entire project, i had done it under an hour! 

---

### PCB LAYOUT

<img width="1318" height="857" alt="cLOCK-IN 3D PCB VIEW" src="https://github.com/user-attachments/assets/9ea3f099-c410-4424-a479-8d69198131c9" />


<img width="793" height="825" alt="cLOCK-IN PCB VIEW" src="https://github.com/user-attachments/assets/b828ad69-cd69-45fc-b0e2-97f2ef2c62a6" />


I have manually routed the PCB. The PCB has 0 errors (DRC) except silkscreen warnings. 

---

### CAD Designing 

I'm new to CAD Designing So, it was not easy for me. I have designed this case in Onshape. This design is a "SANDWITCH STYLE AlARM-CLOCK" where it consists of two plates; Top plate, Bottom Case and PCB gets sandwiched between them. They are all screwed up with M3 Screws. All the design was done in +0.04 tolerance for better results!


#### Assembly View

<img width="877" height="520" alt="cLOCK-IN headbanner photo" src="https://github.com/user-attachments/assets/2bfa2c01-2628-4e67-ba34-930e7a07f5b4" />

#### Top Plate

<img width="1060" height="780" alt="cLOCK-IN Top Plate" src="https://github.com/user-attachments/assets/4fca0231-1058-4be0-9676-1607a27be42f" />


#### Bottom Plate

<img width="1169" height="676" alt="cLOCK-IN bottom case" src="https://github.com/user-attachments/assets/a48017ba-ff46-45b7-b129-2328466e4c14" />


---

## BOM - Bill of Materials


| # | Component | Qty | Source |
|---:|---|---:|---|
| 1 | Seeed Studio XIAO ESP32-C3 | 1 | Hack Club Kit |
| 2 |  MX Switches | 4 | Hack Club Kit |
| 3 |2.25in TFT display | 1 | Hack Club Kit |
| 4 |  3.3V Buzzer | 1 | Hack Club Kit |
| 5 | 3D-Printed Case (Base + Top Plate) | 1 set | Self-fabricated |


## BOM - Bill of Materials

| # | Component | Qty | Source |
|---|---|---:|---|
| 1 | Seeed Studio XIAO ESP32C3 | 1 | BLARE Kit |
| 2 | MX-Style Yellow Switches | 4 | BLARE Kit |
| 3 | White Blank DSA Keycaps | 4 | BLARE Kit |
| 4 | 2.25-inch TFT Display | 1 |BLARE Kit |
| 5 | 3.3V Piezo Buzzer | 1 | BLARE Kit |
| 6 | 2.54mm 8-Pin Male Header | 1 | BLARE Kit |
| 7 | 20cm Female-Female Jumper Wires | 8 | BLARE Kit |
| 8 | M3 × 5 × 4mm Heat-set Inserts | 4 | BLARE Kit |
| 9 | M3 × 8mm Screws | 4 | BLARE Kit |
| 10 | M3 × 16mm Screws | 4 | BLARE Kit |


## Notes

- **Controller:** Seeed Studio XIAO ESP32-C3.
- **Input:** 4 MX-style mechanical switches.
- **Display:** 2.25in TFT display.
- **Sound:** 3.3V Buzzer.
- **Enclosure:** 3D-printed base and top plate.


---

## Function

### Input

| Part | Pin |
|---|---|
| SW4 (UP) | D0 |
| SW3 (DOWN) | D1 |
| SW2 (ENTER) | D2 |
| SW1 (BACK) | D3 |
| Display DC / CS / BL | D4 / D5 / D6 |
| Buzzer | D7 |
| Display RST / SCL / SDA | D8 / D9 / D10 |

### Controls

| Screen | UP | DOWN | ENTER | BACK |
|---|---|---|---|---|
| Home | alarm on/off | set time | set alarm | - |
| Set alarm / time | increase | decrease | next field / save | cancel |
| Ringing | - | - | stop | snooze (5 min) |



> **Note:** The alarm time and on/off state are saved and survive power loss. The clock itself has no battery, so without WiFi you must set the time after each power-up.


---
## AI Usage

 I used AI for assistance in Firmware. Rest all work is done by me! [ For Proof : You can check Lapse recording ] 

---

## Credits

- **HackClub:** - for organizing the program
- **BLARE Team:** - for tutorials and resources

---

## License

OPEN SOURCE, enjoy :) 

( I aint responsible if it turns out to be a time bomb).

















