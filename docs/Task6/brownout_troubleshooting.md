# 🛠️ Part 3 — Fault Troubleshooting: ESP32 Brownout Reset on Motor Engagement

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Fault Investigated:** Microcontroller Resetting When Motors Engage  
**Author:** Aman Kumar

---

## 📌 Executive Summary

The most frequent hardware failure in small autonomous line followers is spontaneous MCU reboot the instant the motors engage from standstill. This guide details the physics of supply rail collapse, an exact diagnostic procedure using an oscilloscope/multimeter, and a multi-layered hardware fix to permanently eliminate brownout resets.

```
 [Dead Stop: Back-EMF = 0V] ──► [Instant Motor Surge: ~2.0A] ──► [Battery Rail Sags < 6V]
                                                                        │
 ┌──────────────────────────────────────────────────────────────────────┘
 ▼
 [Buck / LDO Drops < 2.80V] ──► [ESP32 Comparator Trips] ──► [HARD BROWNOUT RESET] 💥
```

---

## 🔍 1. Symptom & Fault Signature

When the robot detects the start marker or executes an aggressive turn:
1. The motors click or twitch for a fraction of a second.
2. The robot immediately halts and locks up.
3. The ESP32 Serial Monitor outputs the diagnostic abort message:
   ```text
   rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
   configsip: 0, SPIWP:0xee
   clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
   mode:DIO, clock div:2
   load:0x3fff0030,len:1184
   ...
   Brownout detector was triggered
   ```

---

## 🔬 2. Probable Root Causes & Circuit Physics

### Root Cause A: Transient Inrush Current & Battery Rail Sag
* At rest, a DC motor generates **zero Back-EMF** ($E_b = 0$).
* The initial current is governed purely by armature DC coil resistance ($R_{\text{arm}} \approx 3.9\,\Omega$ per motor):
  $$I_{\text{inrush}} = \frac{V_{\text{batt}}}{R_{\text{arm}}} = \frac{7.4\,\text{V}}{3.9\,\Omega} \approx 1.9\,\text{A to } 2.1\,\text{A per pair}$$
* This sudden surge across lead resistance ($R_{\text{wire}}$) and battery internal resistance ($R_{\text{int}}$) causes a steep transient drop ($\Delta V = I \cdot R$).
* If the input to the MP1584EN buck converter drops below its dropout threshold, the clean $3.3\,\text{V}$ logic rail dips below the ESP32 hardware threshold ($V_{\text{brownout}} \approx 2.80\,\text{V}$), tripping the internal comparator reset.

### Root Cause B: Inductive Commutator Arcing & EMI Kickback
* Brushed DC motors create micro-arcs as mechanical brushes cross commutator bars.
* Rapid current changes generate high-frequency inductive kickback ($V = -L \frac{di}{dt}$), injecting $30\,\text{MHz} - 100\,\text{MHz}$ ringing noise onto the shared ground plane.
* This high-frequency noise couples into the ESP32 reset line (`EN` pin / GPIO 0) or resets the crystal oscillator.

---

## 🩺 3. Step-by-Step Diagnostic Protocol

```
 [Step 1: Serial Log Verification] ──► Confirm "Brownout detector was triggered"
                 │
                 ▼
 [Step 2: Voltage Rail Scope Probe] ──► Monitor 3.3V Pin during motor startup
                 ├── Voltage dips below 2.80V? ──► Confirm Power Rail Collapse
                 └── High-frequency RF ringing? ──► Confirm Brush Arcing / EMI
                 │
                 ▼
 [Step 3: Apply Layered Hardware Fixes]
                 │
                 ▼
 [Step 4: Verification Stress Test] ──► 50 Rapid Starts at 100% PWM
```

### First Diagnostic Check (Measurement Method)
1. **Instrument Setup:** Connect a digital storage oscilloscope (DSO) or a multimeter with a fast **Min/Max Peak-Hold** mode across the ESP32 **3.3V** pin and **GND**.
2. **Trigger Configuration:** Set DSO trigger mode to **Single Sweep**, slope **Negative (Falling)**, trigger level at **$3.0\,\text{V}$**, timebase **$500\,\mu\text{s/div}$**.
3. **Execution:** Trigger the motor command in code (`analogWrite(PWM, 255)`).
4. **Observation:**
   * **Confirmed Fault:** The $3.3\,\text{V}$ rail plummets to $\le 2.75\,\text{V}$ for $1-3\,\text{ms}$, coinciding with the exact moment the driver gates open.

---

## 🛡️ 4. Multi-Layered Permanent Hardware Fix

To resolve both DC voltage sag and RF inductive spikes, implement this 3-tier countermeasure:

```
                          [7.4V Battery Lead]
                                  │
    ┌─────────────────────────────┴─────────────────────────────┐
    ▼                                                           ▼
[Bulk Reservoir Cap]                                  [MP1584EN Buck Converter]
 470µF 16V Low-ESR                                              │ (Step-down 5.0V)
 Placed at TB6612 VM & GND                                      ▼
    │                                                 [ESP32 Logic Rail 3.3V]
    ▼                                                           │
[TB6612FNG Driver]                                    [Local Decoupling]
    │                                                  10µF Tantalum + 0.1µF Ceramic
    ▼                                                  Directly on 3.3V & GND pins
[2x GA12-N20 Motors]
    │
[Motor Snubbers] ──► 0.1µF Ceramic across (+) and (-) leads
                     0.1µF Ceramic from each lead to motor metal case
```

### Layer 1: Bulk Energy Reservoir (Solves Sag)
* **Component:** Solder a **$470\,\mu\text{F}$ 16V Low-ESR Electrolytic Capacitor** directly across the TB6612FNG **VM** and **GND** pins.
* **Physics:** Acts as an immediate local energy reservoir. During the first $2\,\text{ms}$ of motor startup, current is supplied by this capacitor rather than travelling the length of the battery harness, keeping the main rail stable.

### Layer 2: High-Frequency LC Filtering (Solves Noise)
* **Component:** Place a **$10\,\mu\text{F}$ Tantalum Capacitor** in parallel with a **$0.1\,\mu\text{F}$ (104) Multi-Layer Ceramic Capacitor (MLCC)** right at the ESP32 power pins.
* **Physics:** The $0.1\,\mu\text{F}$ capacitor has low equivalent series inductance (ESL), absorbing high-frequency transients ($>10\,\text{MHz}$) that large electrolytic capacitors cannot react to.

### Layer 3: Pololu 3-Capacitor Motor Snubber Network (Solves EMI)
* Solder three **$0.1\,\mu\text{F}$ ceramic disc capacitors** directly to each GA12-N20 motor:
  1. **$C_1$:** Across motor terminal (+) and motor terminal (-).
  2. **$C_2$:** From motor terminal (+) to the motor's metal gearbox shell.
  3. **$C_3$:** From motor terminal (-) to the motor's metal gearbox shell.
* **Physics:** Commutator spark energy is shunted straight to the chassis body at the physical source before it radiates into nearby sensor and logic traces.

---

## ✅ 5. Confirmation & Verification Protocol

After soldering the decoupling capacitors and star-ground connections, perform the following verification steps:

| Verification Phase | Test Action | Expected Passing Benchmark | Status |
| :--- | :--- | :--- | :---: |
| **Phase 1: Oscilloscope Probe** | Re-run single sweep DSO capture during 0 $\to$ 100% PWM step | $3.3\,\text{V}$ rail transient dip is $\le 60\,\text{mV}$ (stays above $3.24\,\text{V}$) | **PASS** |
| **Phase 2: Stress Bench Cycling** | Run test sketch toggling rapid Forward-to-Reverse reversals for 50 cycles | Zero reboots; Serial monitor displays uninterrupted 200 Hz loop heartbeats | **PASS** |
| **Phase 3: Arena Dynamic Test** | Full-speed 500 RPM run with aggressive $90^\circ$ pivot skid turns | Robot tracks smoothly through turns without stutter or lockup | **PASS** |

---

## 📚 References & Authority Sources
- 📄 **Universal Solder:** [ESP32 Brownout Detector Architecture & Reset Reasons](https://www.universal-solder.ca/troubleshooting-esp32-brownout/)
- 📄 **Pololu Application Note:** [Dealing with Motor Noise and Decoupling](https://www.pololu.com/docs/0J15/9)
- 📄 **Texas Instruments:** [Understanding Power Supply Decoupling and Capacitor Selection](https://www.ti.com/content/dam/videos/external-videos/en-us/9/3816841626001/6313253251112.mp4/subassets/notes-decoupling_capacitors.pdf)