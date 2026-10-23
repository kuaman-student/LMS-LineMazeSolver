# ⚡ Part 1 — Labelled Wiring Diagram & Power Architecture

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Subsystem:** Electrical Interconnects, Power Distribution, and Common-Ground Infrastructure  

---

## 📌 Power Distribution Architecture (3 Voltage Rails)

To prevent inductive kickback and transient current sag from resetting the digital core, the robot segregates incoming battery power into three electrically isolated functional rails:

```text
 [Orange 7.4V 2S 1500mAh 25C LiPo] 
                 │
           [SPST Switch]
                 │
   ┌─────────────┴─────────────────────────────────────────────┐
   ▼                                                           ▼
[Motor Rail: 7.4V RAW]                                [MP1584EN Buck Converter]
   │                                                           │ (Steps down 7.4V -> 5.0V)
   ├──► TB6612FNG (VM Pin)                                     ▼
   │                                             [Sensor & MCU Subsystem: 5.0V]
   │                                                   ├──► Pololu QTR-8A (VCC)
   │                                                   └──► ESP32 DevKit (VIN / 5V Pin)
   │                                                             │ (Internal LDO 5V -> 3.3V)
   │                                                             ▼
   │                                                  [Logic Reference: 3.3V]
   │                                                        ├──► TB6612FNG (VCC & STBY)
   │                                                        └──► Cree Red Status LED (via 220Ω)
   ▼
[COMMON STAR GROUND] ◄── Shorted: Battery(-) = Buck GND = ESP32 GND = TB6612 GND
```

---

## ⚡ Voltage Rails Breakdown

| Rail Name | Source Provider | Voltage Level | Fed Components | Functional Role & Isolation Rationale |
| :--- | :--- | :---: | :--- | :--- |
| **High-Current Motor Rail** | 7.4V LiPo via Rocker Switch | **7.4V Nominal (8.4V Peak)** | TB6612FNG `VM` pin $\to$ GA12-N20 Motors | Unregulated direct power for maximum torque and 500 RPM dynamic response. Motor inductive spikes are absorbed locally by bulk bypass capacitors. |
| **Regulated Sensor/MCU Rail** | MP1584EN Step-Down Buck Converter | **5.0V Clean ($\pm1\%$)** | ESP32 `VIN` pin, Pololu QTR-8A `VCC` | Steps down dirty 7.4V to smooth 5.0V. Eliminates IR emitter flicker and powers the ESP32 internal low-dropout regulator without thermal stress. |
| **Microcontroller Logic Rail** | ESP32 On-board LDO (AMS1117 / ME6211) | **3.3V Regulated** | TB6612FNG `VCC` & `STBY`, Cree Red Goal LED | Supplies low-noise 3.3V reference to match ESP32 native GPIO voltage levels. Protects 3.3V-only CMOS gates from 5V overvoltage destruction. |

---

## 🔌 Complete Pin-to-Pin Interconnection Matrix

### 1. TB6612FNG Dual H-Bridge Interface

| Driver Pin | Connected Component | Logic / Supply Level | Functional Purpose |
| :---: | :---: | :---: | :--- |
| **VM** | 7.4V LiPo (+) via Switch | 7.4V RAW (Power) | High-current motor supply rail ($2.27\text{ A}$ peak capacity). |
| **VCC** | ESP32 3.3V Pin | 3.3V Logic | Driver core IC logic supply ($I_{CC} \approx 1.5\text{ mA}$). |
| **GND (x2)** | Common Star Ground | 0V Reference | Power ground and logic signal ground reference. |
| **STBY** | ESP32 3.3V Pin (or GPIO 4) | 3.3V (HIGH) | Hardware Standby. Pulled HIGH to take driver out of shutdown mode. |
| **PWMA** | ESP32 **GPIO 14** | 3.3V PWM (LEDC) | Hardware PWM timer channel (20 kHz) for Left Motor speed control. |
| **AIN1** | ESP32 **GPIO 27** | 3.3V Digital | Direction control phase 1 (Left Motor H-bridge branch). |
| **AIN2** | ESP32 **GPIO 26** | 3.3V Digital | Direction control phase 2 (Left Motor H-bridge branch). |
| **PWMB** | ESP32 **GPIO 13** | 3.3V PWM (LEDC) | Hardware PWM timer channel (20 kHz) for Right Motor speed control. |
| **BIN1** | ESP32 **GPIO 12** | 3.3V Digital | Direction control phase 1 (Right Motor H-bridge branch). |
| **BIN2** | ESP32 **GPIO 11** | 3.3V Digital | Direction control phase 2 (Right Motor H-bridge branch). |
| **A01 / A02** | Left GA12-N20 Motor Leads | Bi-directional $\pm$7.4V | Differential drive output to Left Motor with bypass snubber. |
| **B01 / B02** | Right GA12-N20 Motor Leads | Bi-directional $\pm$7.4V | Differential drive output to Right Motor with bypass snubber. |

---

### 2. Pololu QTR-8A Reflectance Sensor Array Interface

> **Critical Design Constraint:** All 8 analog signals route exclusively to **ADC1** pins on the ESP32. ADC2 pins are internally shared with the Wi-Fi/BT baseband and return corrupted readings when any RF routines or system ticks run.

| Sensor Pin | ESP32 Target Pin | ADC Channel | Functional Position & Tracking Role |
| :---: | :---: | :---: | :--- |
| **VCC** | MP1584EN Buck (+5.0V) | — | Powers 8 high-output 940nm IR emitter LEDs in constant-current pairs. |
| **GND** | Common Star Ground | — | Sensor ground reference and LED emitter return path. |
| **OUT 1** | **GPIO 36 (VP)** | ADC1_CH0 | Far-Left outer sensor: 90° intersection and T-junction leading detector. |
| **OUT 2** | **GPIO 39 (VN)** | ADC1_CH3 | Mid-Left guidance sensor: sharp curve trajectory acquisition. |
| **OUT 3** | **GPIO 34** | ADC1_CH6 | Inner-Left tracking sensor: weighted centroid dynamic tracking. |
| **OUT 4** | **GPIO 35** | ADC1_CH7 | Center-Left tracking sensor: primary steady-state straight line reference. |
| **OUT 5** | **GPIO 32** | ADC1_CH4 | Center-Right tracking sensor: primary steady-state straight line reference. |
| **OUT 6** | **GPIO 33** | ADC1_CH5 | Inner-Right tracking sensor: weighted centroid dynamic tracking. |
| **OUT 7** | **GPIO 25** | ADC1_CH8 | Mid-Right guidance sensor: sharp curve trajectory acquisition. |
| **OUT 8** | **GPIO 26 / 15** | ADC1_CH2 | Far-Right outer sensor: 90° intersection and T-junction leading detector. |

---

### 3. Auxiliaries & Competition Status LED

| Component | Connected Pins | In-Line Component | Functional Purpose |
| :--- | :---: | :---: | :--- |
| **Cree 5mm Red LED** | **GPIO 2** to Ground | **220Ω Resistor** | Goal pad arrival indicator: holds lit for $\ge 3\text{ seconds}$ on 400×400mm stop zone (+5 bonus pts). |
| **Left Encoder (A/B)** | **GPIO 16, GPIO 17** | Direct Logic Input | Dual-channel magnetic quadrature pulses for dead-reckoning odometry during 90° blind pivot turns. |
| **Right Encoder (A/B)** | **GPIO 18, GPIO 19** | Direct Logic Input | Dual-channel magnetic quadrature pulses for dead-reckoning odometry during 90° blind pivot turns. |

---

## 📐 End-to-End System Schematic (ASCII Circuit Map)

```text
                           +-------------------------------------+
                           |   Orange 7.4V 2S 1500mAh 25C LiPo   |
                           +------------------+------------------+
                                  (+)         |         (-)
                                   |          |          |
                              [SPST Switch]   |          |
                                   |          |          |
           +-----------------------+          |          |
           |                                  |          |
           | (7.4V RAW)                       |          |
           +------------------+               |          |
           |                  |               |          |
           ▼                  ▼               |          |
    +--------------+   +--------------+       |          |
    |  TB6612FNG   |   |   MP1584EN   |       |          |
    |  Driver (VM) |   |   Buck IN(+) |       |          |
    +-------+------+   +-------+------+       |          |
            |                  | (5.0V REG)   |          |
            |                  +---------+    |          |
            |                            |    |          |
            |                            ▼    ▼          |
            |                    +------------------+    |
            |                    |  Pololu QTR-8A   |    |
            |                    |  VCC        GND  |    |
            |                    +---+----------+---+    |
            |                        |          |        |
            |                        | (Analog) |        |
            |                        ▼          |        |
            |                  +----------------+---+    |
            |                  |  ESP32-WROOM-32    |    |
            |                  |  VIN (5V)      GND |    |
            |                  |  3.3V (OUT)    ADC1|    |
            |                  +---+-------------+--+    |
            |                      |             |       |
            |                      | (3.3V Logic)|       |
            |                      ▼             |       |
            |              +---------------+     |       |
            +------------->| TB6612FNG VCC |     |       |
                           | STBY, IN1-IN4 |<----+       |
                           +-------+-------+             |
                                   |                     |
                                   ▼                     ▼
                             [COMMON STAR GROUND TERMINAL BLOCK]
                                 (0V Absolute Shared Reference)
```

---

## 🌐 Common-Ground Topology: Physical Necessity

In mixed-signal electromechanical systems, grounding is the most common point of circuit failure. The common-ground configuration is designed around three electrical principles:

### 1. The Shared Voltage Reference ($V = V_{\text{signal}} - V_{\text{ref}}$)
Voltage is not an absolute quantity; it is a potential difference between two distinct points. When the ESP32 drives `AIN1` to $3.3\text{ V}$, that voltage is referenced solely to the ESP32's internal ground plane. If the motor driver’s ground terminal floats with respect to the ESP32, the driver's gate logic detects an undefined, floating input level, resulting in random motor triggers or failure to switch.

### 2. Eliminating Floating Gate Susceptibility
The input stages of the TB6612FNG consist of high-impedance CMOS gates. Without a common ground reference, high-impedance inputs act as miniature antennae, picking up ambient electromagnetic radiation (EMI) from brushed motor commutator arcing and inductive rail collapse, triggering spurious bridge firings.

### 3. Star-Ground Topology vs. Ground Loops
Ground connections must never be daisy-chained from one module to the next (e.g., `Battery (-) -> Motors -> ESP32 -> Sensors`). Brushed motors draw current surges up to $2.27\text{ A}$, which generate transient voltage drops across wire resistances ($V = I \cdot R_{\text{wire}}$). A daisy-chained layout shifts the ESP32 reference ground above 0V (ground bounce), causing ADC read errors and hard chip brownouts.

* **Engineering Solution:** A dedicated **Star-Ground Junction** where the LiPo negative terminal, Buck Converter output ground, ESP32 ground pin, and TB6612FNG ground pins meet at one single, low-impedance solder point. High-frequency motor return currents circulate back to the battery without traversing logic return paths.

---

## 🛡️ Circuit Protection & Decoupling Strategy

1. **Bulk Power Rail Decoupling:** A $470\,\mu\text{F}$ 16V low-ESR electrolytic capacitor is soldered directly across the TB6612FNG `VM` and `GND` pins to supply immediate inrush current during dead-stop motor accelerations.
2. **High-Frequency RF Suppression:** Each GA12-N20 motor has a $0.1\,\mu\text{F}$ (104) ceramic capacitor soldered directly across its positive and negative solder lugs to short inductive brush-arcing spikes at the source.
3. **Logic Rail Filtering:** A $10\,\mu\text{F}$ tantalum capacitor paired with a $0.1\,\mu\text{F}$ ceramic capacitor sits adjacent to the ESP32 `VIN` terminal to filter residual high-frequency switching ripple generated by the MP1584EN buck regulator.

---

## 📚 References & Authority Sources
- 📄 **Toshiba Semiconductor:** [TB6612FNG Dual H-Bridge Driver Datasheet](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 📄 **Pololu Robotics:** [QTR-8A Reflectance Sensor Array Technical User Guide](https://www.pololu.com/docs/pdf/0j12/qtr-8x.pdf)
- 📄 **Espressif Systems:** [ESP32 Hardware Design Guidelines — Power Scheme & Pinout Considerations](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/esp-hardware-design-guidelines-en-master-esp32.pdf)
- 🛠️ **Author:** Aman Kumar