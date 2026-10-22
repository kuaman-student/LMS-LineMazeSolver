# 📄 Task 2 — Datasheet Literacy: Toshiba TB6612FNG

A clean, structured, and interactive engineering report analyzing the critical operational parameters and failure boundaries of the Toshiba TB6612FNG Dual DC Motor Driver IC.

---

## 🔗 Official Documentation & Sourcing Links

- 🌐 **Official Product Page:** [Toshiba Semiconductor TB6612FNG Product Detail](https://toshiba.semicon-storage.com/ap-en/semiconductor/product/motor-driver-ics/brushed-dc-motor-driver-ics/detail.TB6612FNG.html)
- 📄 **Official Manufacturer Datasheet:** [Toshiba TB6612FNG Datasheet (PDF direct link)](http://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 🛒 **Supplier Listing (BOM Sourced):** [Robu.in TB6612FNG Dual Motor Driver Carrier](https://robu.in/product/motor-driver-tb6612fng-module-performance-ultra-small-volume-3-pi-matching-performance-ultra-l298n/)
- 🎯 **Target System:** Differential-drive maze robot powered by ESP32-WROOM-32 (3.3V logic) and a 2S LiPo battery pack (7.4V nominal / 8.4V peak).

---

## ⚙️ 1. Parameters Directly Constraining Circuit Design

### 🔹 Parameter 1: Motor Supply Operating Voltage ($V_M$)
* **Datasheet Specifications:**
  * **Recommended Operating Range:** $4.5\text{ V}$ to $13.5\text{ V}$
  * **Absolute Maximum Rating:** $15.0\text{ V}$
* **Circuit Impact & Design Choices:**
  * **Battery Pack Sizing:** A 1S Li-ion/LiPo ($3.7\text{ V}$ nominal, $3.0\text{ V}$ cutoff) cannot be used because it falls below the minimum $4.5\text{ V}$ required for stable H-bridge gate drive. Conversely, a 4S LiPo ($16.8\text{ V}$ peak) would exceed the $15.0\text{ V}$ absolute maximum limit.
  * **Direct Supply Wiring:** A **2S LiPo battery** ($7.4\text{ V}$ nominal, $8.4\text{ V}$ maximum charge) sits dead-center within the safe $4.5\text{ V} - 13.5\text{ V}$ operating window. This allows the battery positive terminal (after the master switch) to connect directly to the driver's `VM` pin without going through a bulky voltage regulator, eliminating power conversion loss and thermal stress.

---

### 🔹 Parameter 2: Logic Supply Voltage ($V_{CC}$) and Input Thresholds ($V_{IH} / V_{IL}$)
* **Datasheet Specifications:**
  * **Logic Supply Voltage Range ($V_{CC}$):** $2.7\text{ V}$ to $5.5\text{ V}$
  * **High-Level Input Voltage ($V_{IH}$):** $0.7 \times V_{CC}$ (minimum)
  * **Low-Level Input Voltage ($V_{IL}$):** $-0.2\text{ V}$ to $0.2 \times V_{CC}$ (maximum)
* **Circuit Impact & Design Choices:**
  * **The 5V Trap:** If the driver's logic pin `VCC` were tied to a standard $5.0\text{ V}$ rail (e.g., buck converter output), the minimum high voltage threshold would be:
    $$V_{IH(\text{min})} = 0.7 \times 5.0\text{ V} = \mathbf{3.5\text{ V}}$$
    Because the ESP32 GPIOs operate at **$3.3\text{ V}$**, a $3.3\text{ V}$ signal would sit below the $3.5\text{ V}$ threshold, leading to dropped PWM cycles, erratic motor jitter, or complete failure to switch.
  * **The 3.3V Solution:** By connecting `VCC` directly to the ESP32's regulated **$3.3\text{ V}$ output rail**:
    $$V_{IH(\text{min})} = 0.7 \times 3.3\text{ V} = \mathbf{2.31\text{ V}}$$
    The ESP32's $3.3\text{ V}$ high signal provides an abundant **$0.99\text{ V}$ noise margin**, guaranteeing clean, reliable digital logic switching without requiring bi-directional level-shifter ICs.

---

### 🔹 Parameter 3: Continuous & Peak Output Current ($I_{OUT}$)
* **Datasheet Specifications:**
  * **Continuous Average Current:** $1.2\text{ A}$ per channel (at $T_a = 25^\circ\text{C}$)
  * **Peak Output Current (Single Pulse):** $3.2\text{ A}$ ($t_w = 10\text{ ms}$)
  * **Peak Output Current (Repetitive):** $2.0\text{ A}$ ($t_w = 20\text{ ms}$, duty cycle $\le 20\%$)
* **Circuit Impact & Design Choices:**
  * **Motor Pairing:** Our chosen GA12-N20 500 RPM gear motors have a free-run current of $\approx 0.15\text{ A} - 0.20\text{ A}$ and a worst-case stall current of $\approx 0.65\text{ A}$ at $6\text{V} - 7.4\text{V}$.
  * **Heatsink Elimination:** Even if both wheels become completely locked against a maze wall, the $0.65\text{ A}$ stall draw is only **54% of the 1.2A continuous rating**. This wide thermal headroom means the TB6612FNG module can run safely without adding heavy heatsinks or external cooling fans, keeping total robot weight minimal.

---

## ⚠️ 2. Destructive Condition & Engineering Countermeasures

### 💥 The Risk: Inductive Flyback Voltage Spikes Exceeding Absolute Maximum ($V_M > 15\text{ V}$)
* **Failure Mechanism:**
  * DC motors are inductive loads ($V = L \frac{di}{dt}$). When the H-bridge rapidly reverses motor direction or switches from high-speed driving to active short-braking, the collapsing magnetic field in the motor coils generates an instantaneous reverse voltage spike (back-EMF).
  * If this inductive kickback energy dumps directly into the `VM` trace without adequate local decoupling, it causes voltage transients that exceed the **$15.0\text{ V}$ Absolute Maximum Rating**.
  * Spikes above $15\text{ V}$ cause dielectric breakdown of the internal power MOSFET gate oxides, resulting in permanent drain-to-source short circuits and burnt driver channels.

```text
Rapid Direction Reverse / Hard Braking
               │
               ▼
   Motor Back-EMF Induction (V = L * di/dt)
               │
               ▼
Transient Voltage Spike on VM Rail (> 15.0V)
               │
               ▼
Gate-Oxide Dielectric Breakdown in MOSFETs ──► Permanent Driver Destruction 💥
```

---

### 🛡️ Design Countermeasures (Hardware & Software)

```text
               [ 2S LiPo Battery: 7.4V - 8.4V ]
                              │
                              ▼
                [ SPST Master Power Switch ]
                              │
                              ▼
           ┌──────────────────────────────────────┐
           │   Dual-Stage Bypass Decoupling       │
           │  • 47µF Low-ESR Electrolytic Cap     │ <── Absorbs bulk inductive surge
           │  • 0.1µF Multi-Layer Ceramic Cap     │ <── Clamps fast high-frequency spikes
           └──────────────────┬───────────────────┘
                              │
                              ▼
                     TB6612FNG (VM Pin)
                              │
        ┌─────────────────────┴─────────────────────┐
        ▼                                           ▼
[Internal Flyback Diodes]                [Firmware Slew Ramping]
Integrated body diodes route             10ms deadband deceleration
discharge currents safely to GND         before direction reversals
```

1. **Hardware Decoupling (Capacitor Filter):**
   * Placed a **$47\,\mu\text{F}$ low-ESR electrolytic capacitor** in parallel with a **$0.1\,\mu\text{F}$ high-frequency ceramic capacitor** directly across the breakout board's `VM` and `PGND` pins with minimum lead length to absorb transient voltage surges.
2. **Internal Freewheeling Diodes:**
   * Utilized the TB6612FNG's integrated internal parasitic body diodes across each MOSFET switch, which provide safe recirculation loops for inductive energy back into ground during decay cycles.
3. **Firmware Deceleration Deadbands:**
   * In the motor drive firmware, instant hard reversals (e.g., transitioning straight from $+255$ forward PWM to $-255$ reverse PWM) are strictly forbidden. The software enforces a **$10\text{ ms}$ zero-PWM / dynamic short-brake ramp** before reversing direction, preventing high $\frac{di}{dt}$ transients.

---

## 📊 Summary Mapping Table

| Parameter / Limit | Datasheet Specified Value | Robot Design Implementation |
| :--- | :---: | :--- |
| **Supply Voltage ($V_M$)** | $4.5\text{ V} - 13.5\text{ V}$ operating | 2S LiPo ($7.4\text{ V}$ nom / $8.4\text{ V}$ peak) direct drive |
| **Logic Threshold ($V_{IH}$)** | $\ge 0.7 \times V_{CC}$ | $V_{CC}$ tied to ESP32 $3.3\text{ V}$ rail ($V_{IH} = 2.31\text{V}$, no level shifter) |
| **Continuous Current ($I_{OUT}$)** | $1.2\text{ A}$ continuous per channel | Sized for N20 motors ($0.65\text{ A}$ stall, 84% margin) |
| **Absolute Maximum ($V_{M\text{,max}}$)** | $15.0\text{ V}$ maximum limit | $47\,\mu\text{F} + 0.1\,\mu\text{F}$ bypass caps + 10ms software brake ramp |

---

## 🤖 Credits & References

- 📄 **Official Datasheet:** [Toshiba Semiconductor TB6612FNG Datasheet (Rev. 2014-10-01)](http://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 🌐 **Manufacturer Portal:** [Toshiba Brushed DC Motor Drivers](https://toshiba.semicon-storage.com/ap-en/semiconductor/product/motor-driver-ics/brushed-dc-motor-driver-ics/detail.TB6612FNG.html)
- 🛒 **Supplier Page:** [Robu.in TB6612FNG Module](https://robu.in/product/motor-driver-tb6612fng-module-performance-ultra-small-volume-3-pi-matching-performance-ultra-l298n/)
- 🛠️ **Author:** Aman Kumar, verified by claude(Anthropic)
- 📝 **Evaluation Task:** Techfest Meshmerize Task 2 Deliverable (18 pts)