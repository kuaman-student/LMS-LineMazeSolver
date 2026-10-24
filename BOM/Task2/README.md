# 📄 Task 2 — Datasheet Literacy: Toshiba TB6612FNG

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Subsystem:** Actuator Power Electronics & Driver Interface  
**Author:** Aman Kumar 

---

## 📌 Executive Summary

This document presents a structured engineering analysis of the **Toshiba TB6612FNG Dual DC Motor Driver IC** based on its official manufacturer datasheet. It identifies three critical electrical parameters that directly constrain the circuit architecture of our autonomous robot, explains the selection of battery chemistry and logic level connections, and analyzes the root-cause failure physics of inductive kickback with corresponding hardware and software countermeasures.

---

## 🔗 Official Documentation & Sourcing Links

- 📄 **Official Manufacturer Datasheet:** [Toshiba TB6612FNG Technical Datasheet (Rev. 2014-10-01)](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 🌐 **Manufacturer Product Portal:** [Toshiba Brushed DC Motor Driver ICs](https://toshiba.semicon-storage.com/ap-en/semiconductor/product/motor-driver-ics/brushed-dc-motor-driver-ics/detail.TB6612FNG.html)
- 🛒 **Supplier Listing (BOM Sourced):** [Robu.in TB6612FNG Dual H-Bridge Breakout Module](https://robu.in/product/motor-driver-tb6612fng-module-performance-ultra-small-volume-3-pi-matching-performance-ultra-l298n/)
- 🎯 **Target System:** ESP32-WROOM-32 (3.3V Logic) + Orange 2S 7.4V 1500mAh 25C LiPo + 2x GA12-N20 500 RPM Micro Metal Gearmotors

---

## ⚙️ 1. Parameters Directly Constraining Circuit Design

### 🔹 Parameter 1: Motor Supply Operating Range ($V_M$) & PWM Thresholds
- **Datasheet Specifications:**
  * **Absolute Maximum Rating:** $V_M = 15.0\text{ V}$
  * **Operating Supply Voltage Range:** $V_M = 2.5\text{ V}$ to $13.5\text{ V}$ (Typical: $5.0\text{ V}$)
  * **Operational Condition Requirements (Datasheet Remarks):**
    * Full PWM Operation: Requires $V_M \ge 4.5\text{ V}$ ($I_{OUT} = 1.0\text{ A}$ max)
    * Low-Voltage Reduced Operation ($2.5\text{ V} \le V_M < 4.5\text{ V}$): Max $I_{OUT} = 0.4\text{ A}$ and strictly **Without PWM Operation**
- **Circuit Impact & Design Decisions:**
  * **Elimination of 1S LiPo / Li-ion Packs:** A single-cell battery ($3.0\text{ V}$ cutoff to $4.2\text{ V}$ peak) falls below $4.5\text{ V}$. Operating in this sub-$4.5\text{ V}$ region disables high-frequency PWM switching and limits current to $0.4\text{ A}$, which would make dynamic differential steering impossible.
  * **Elimination of 3S LiPo Packs ($11.1\text{ V}$ Nom / $12.6\text{ V}$ Peak):** Although mock hardware guides frequently reference 3S packs for older, lossy drivers like the bipolar L298N, pairing a 3S LiPo directly with the TB6612FNG introduces severe electrical and thermal failure risks:
    1. *Extremely Narrow Voltage Headroom:* A fully charged 3S LiPo reaches **$12.6\text{ V}$**, leaving a razor-thin **$0.9\text{ V}$ safety margin** before breaching the $13.5\text{ V}$ recommended operating ceiling, and only **$2.4\text{ V}$** before hitting the catastrophic $15.0\text{ V}$ Absolute Maximum Rating.
    2. *Vulnerability to Inductive Transients:* Motor inductive back-EMF spikes easily swing $2\text{ V} - 4\text{ V}$ above the rail during high-speed braking or rapid direction changes. On a $12.6\text{ V}$ baseline rail, these spikes inevitably cross the $15.0\text{ V}$ dielectric breakdown threshold, puncturing the driver's internal DMOS gates.
    3. *Severe Overvolting of GA12-N20 Motors:* Micro metal gearmotors are rated nominally for $6.0\text{ V}$. Driving them at $11.1\text{ V} - 12.6\text{ V}$ ($>200\%$ rating) induces severe brush arcing, rapid commutator degradation, and gearbox tooth stripping under stall torque.
    4. *Buck Regulator Thermal Stress:* Stepping down from $12.6\text{ V}$ to $5.0\text{ V}$ ($\Delta V = 7.6\text{ V}$) increases switching dissipation and EMI noise within the MP1584EN step-down regulator compared to a $7.4\text{ V}$ input ($\Delta V = 2.4\text{ V}$).
  * **Selection of 2S LiPo Battery:** An Orange 2S 7.4V LiPo ($7.4\text{ V}$ nominal, $8.4\text{ V}$ maximum charge) is the optimal balance:
    * It sits comfortably above $4.5\text{ V}$ for full $1.0\text{ A}$ PWM capability.
    * At $8.4\text{ V}$ peak, it maintains a massive **$5.1\text{ V}$ operating headroom** ($13.5\text{ V} - 8.4\text{ V}$) and a **$6.6\text{ V}$ destructive barrier** ($15.0\text{ V} - 8.4\text{ V}$), absorbing back-EMF spikes with complete safety.
    * It pairs with $6\text{ V}$ N20 motors safely with negligible thermal wear.

---

### 🔹 Parameter 2: Logic Supply Voltage ($V_{CC}$) and Input Thresholds ($V_{IH} / V_{IL}$)
- **Datasheet Specifications:**
  * **Logic Supply Voltage Operating Range:** $V_{CC} = 2.7\text{ V}$ to $5.5\text{ V}$ (Typical: $3.0\text{ V}$)
  * **Absolute Maximum Ratings:** $V_{CC} = 6.0\text{ V}$, $V_{IN} = -0.2\text{ V}$ to $6.0\text{ V}$
  * **High-Level Input Voltage ($V_{IH}$):** $0.7 \times V_{CC}$ (Minimum)
  * **Low-Level Input Voltage ($V_{IL}$):** $-0.2\text{ V}$ to $0.2 \times V_{CC}$ (Maximum)
- **Circuit Impact & Design Decisions:**
  * **The 5.0V Supply Hazard:** If the driver's logic pin `VCC` were tied to a $5.0\text{ V}$ rail (such as the buck converter output):
    $$V_{IH(\text{min})} = 0.7 \times 5.0\text{ V} = \mathbf{3.5\text{ V}}$$
    Because the ESP32 GPIOs operate at native $3.3\text{ V}$ logic levels, the $3.3\text{ V}$ HIGH output falls below the $3.5\text{ V}$ threshold. This would result in undefined logic states, missing PWM pulses, and erratic motor behavior.
  * **The Direct 3.3V Native Solution:** By connecting `VCC` directly to the ESP32's on-board **$3.3\text{ V}$ regulated logic rail**:
    $$V_{IH(\text{min})} = 0.7 \times 3.3\text{ V} = \mathbf{2.31\text{ V}}$$
    $$V_{IL(\text{max})} = 0.2 \times 3.3\text{ V} = \mathbf{0.66\text{ V}}$$
    The ESP32's $3.3\text{ V}$ logic signal provides a comfortable **$0.99\text{ V}$ noise margin**, ensuring clean switching without requiring external level-shifter ICs.

---

### 🔹 Parameter 3: Continuous & Peak Output Current ($I_{OUT}$)
- **Datasheet Specifications:**
  * **Continuous Operating Current ($V_M \ge 4.5\text{ V}$):** $1.0\text{ A}$ maximum per channel
  * **Absolute Maximum Continuous Rating:** $1.2\text{ A}$ per channel ($T_a = 25^\circ\text{C}$)
  * **Repetitive Peak Pulse:** $2.0\text{ A}$ ($t_w = 20\text{ ms}$, Duty Cycle $\le 20\%$)
  * **Single Peak Pulse:** $3.2\text{ A}$ ($t_w = 10\text{ ms}$)
- **Circuit Impact & Design Decisions:**
  * **Motor Sizing:** The selected GA12-N20 500 RPM micro gearmotors exhibit a no-load cruising current of $\approx 0.18\text{ A}$ and a worst-case stall current of $\approx 0.65\text{ A}$ at $7.4\text{ V}$.
  * **Thermal Margins:** When a motor stalls during an aggressive turn, the $0.65\text{ A}$ draw is $65\%$ of the $1.0\text{ A}$ continuous operating limit, providing a **$35\%$ safety margin**. This permits passive cooling via the breakout PCB's copper ground plane, eliminating the need for an external heatsink and saving $28.5\text{ g}$ compared to an L298N driver module.

---

## ⚠️ 2. Destructive Condition & Engineering Countermeasures

### 💥 The Risk: Inductive Flyback Voltage Spikes ($V_M > 15.0\text{ V}$)
- **Failure Physics:**
  * Brushed DC motor windings act as inductors ($V = -L \frac{di}{dt}$). When the H-bridge transitions rapidly from forward drive ($+255$ PWM) to reverse drive or active dynamic braking, the rapid collapse of the magnetic field induces a high reverse voltage spike (inductive kickback).
  * Without sufficient local filtering, this spike dumps into the motor power rail, causing transients that exceed the **$15.0\text{ V}$ Absolute Maximum Rating** on the `VM` pin.
  * Overvoltage beyond $15\text{ V}$ causes dielectric breakdown of the internal DMOS gate oxides, creating permanent drain-to-source short circuits and burning out driver channels.

```text
Rapid Direction Reversal / Hard Braking
               │
               ▼
   Motor Back-EMF Induction (V = -L * di/dt)
               │
               ▼
Transient Voltage Spike on VM Rail (> 15.0V)
               │
               ▼
Dielectric Breakdown in MOSFET Gate Oxide ──► Permanent Driver Destruction 💥
```

---

### 🛡️ Design Countermeasures (Hardware & Software)

```text
               [ Orange 2S LiPo: 7.4V - 8.4V ]
                              │
                              ▼
                [ SPST Master Power Switch ]
                              │
                              ▼
            ┌──────────────────────────────────────┐
            │        Dual-Stage Decoupling         │
            │  • 470µF Low-ESR Electrolytic Cap    │ <── Absorbs bulk inductive kickback
            │  • 0.1µF Multi-Layer Ceramic Cap     │ <── Clamps high-frequency RF noise
            └──────────────────┬───────────────────┘
                               │
                               ▼
                       TB6612FNG (VM Pin)
                               │
        ┌──────────────────────┴──────────────────────┐
        ▼                                             ▼
[Internal Body Diodes]                       [Firmware Slew Ramp]
Parasitic diodes route inductive             10ms active brake delay
decay currents safely to GND                 enforced before direction reverse
```

1. **Hardware Decoupling:**
   * A **$470\,\mu\text{F}$ 16V low-ESR electrolytic capacitor** is soldered directly across the `VM` and `PGND` pins of the TB6612FNG board to absorb low-frequency inductive surge energy.
   * A **$0.1\,\mu\text{F}$ (104) ceramic capacitor** is placed in parallel to shunt high-frequency RF ringing ($>10\text{ MHz}$) to ground.
2. **Motor-Level Snubbers:**
   * Three $0.1\,\mu\text{F}$ ceramic disc capacitors are soldered directly to each GA12-N20 motor (one across the (+) and (-) terminals, and two from each terminal to the metal gearbox shell) to suppress commutator brush arcing at the source.
3. **Firmware Deceleration Deadbands:**
   * Direct, instantaneous reversals in code (such as $+255$ to $-255$ PWM) are prohibited. The motor driver firmware enforces a mandatory **$10\text{ ms}$ dynamic short-brake ramp** prior to switching bridge direction pins, controlling the current slew rate ($\frac{di}{dt}$).

---

## 📊 Summary Mapping Table

| Parameter / Limit | Datasheet Specified Value | Robot Design Implementation |
| :--- | :---: | :--- |
| **Operating Motor Voltage ($V_M$)** | $2.5\text{ V} - 13.5\text{ V}$ ($V_M \ge 4.5\text{ V}$ for PWM) | 2S LiPo ($7.4\text{ V}$ nominal / $8.4\text{ V}$ peak) direct drive |
| **Logic Threshold ($V_{IH}$)** | $\ge 0.7 \times V_{CC}$ | $V_{CC}$ connected to ESP32 $3.3\text{ V}$ rail ($V_{IH} = 2.31\text{ V}$, zero level shifter) |
| **Operating Current ($I_{OUT}$)** | $1.0\text{ A}$ continuous ($V_M \ge 4.5\text{ V}$) | Sized for GA12-N20 motors ($0.65\text{ A}$ stall, 35% safety margin) |
| **Absolute Maximum ($V_{M\text{,max}}$)** | $15.0\text{ V}$ maximum limit | $470\,\mu\text{F} + 0.1\,\mu\text{F}$ bypass caps + 10ms software brake ramp |

---

## 📚 References & Authority Sources

- 📄 **Toshiba Semiconductor:** [TB6612FNG Technical Datasheet (Rev. 2014-10-01)](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 📄 **Pololu Robotics:** [TB6612FNG Dual Motor Driver Carrier User Guide](https://www.pololu.com/product/713)
- 📄 **Texas Instruments:** [Understanding Motor Driver Voltage Surges & Decoupling Selection](https://www.ti.com/)
- 🛠️ **Author:** Aman Kumar (IIT Mandi)