# 🚗 Part 2 — Motor Driver Architecture & H-Bridge Physics

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Subsystem:** Actuator Interfacing, Power Electronics & Driver Topology

---

## 📌 Executive Summary

A microcontroller cannot directly power a DC motor. The **Toshiba TB6612FNG** acts as an electromechanical buffer, translating low-voltage $3.3\text{ V}$ logic commands into high-power $7.4\text{ V}$ bidirectional drive currents while isolating the microcontroller from catastrophic inductive flyback spikes.

```text
 [ESP32 GPIO Core]           [TB6612FNG Driver Bridge]          [GA12-N20 Motor]
  3.3V Logic Level   ──────►   Integrated Power MOSFETs  ──────►  7.4V Switched Rail
  ~1 mA Signal Gate            Internal Clamp Diodes              Up to 950 mA Surge
```

---

## ⚡ 1. The Fundamental Role of the Motor Driver

Connecting a DC gearmotor directly to an ESP32 GPIO pin causes instant hardware destruction due to two physical barriers:

### Barrier A: The Current Disparity ($I_{\text{motor}} \gg I_{\text{GPIO}}$)
* **ESP32 Absolute Maximum:** A single GPIO pin can source/sink a maximum of **$\approx 20 - 40\text{ mA}$** (with recommended continuous draw under $12\text{ mA}$).
* **GA12-N20 Motor Draw:** Cruising current is **$\approx 180\text{ mA}$**, and stall current reaches **$950\text{ mA}$**.
* **Result:** Direct connection causes immediate thermal overcurrent burnout of the MCU's internal silicon metallization tracks.

### Barrier B: Inductive Kickback & Lenz's Law ($V = -L \frac{di}{dt}$)
* A DC motor winding is an inductor ($L$). When current switching occurs (e.g., stopping or PWM chopping), the collapsing magnetic field generates a massive reverse voltage spike:
  $$V_{\text{spike}} = -L \frac{di}{dt}$$
* This **flyback kickback can exceed $50 - 100\text{ V}$**, instantly punching through the $3.3\text{ V}$ CMOS gate oxide layer of the ESP32 unless clamped by freewheeling diodes.

---

## ⚖️ 2. Architectural Benchmark: TB6612FNG vs. L298N

The competition selection between the legacy **L298N** and the modern **TB6612FNG** comes down to transistor physics:

```text
             L298N (Legacy BJT)                      TB6612FNG (Modern DMOS)
       ┌────────────────────────────┐             ┌────────────────────────────┐
       │ Bipolar Junction (npn/pnp) │             │ Power MOSFET (N & P Chan)  │
       │ V_CE(sat) Drop: 1.8V - 3.2V│             │ R_DS(on) Resistance: ~0.5Ω │
       │ High Thermal Dissipation   │             │ Efficient, Zero Heat Sink  │
       │ Massive Heat Sink Needed   │             │ Compact Footprint (1.5g)   │
       └────────────────────────────┘             └────────────────────────────┘
```

### Head-to-Head Comparison Table

| Parameter | Legacy L298N Module | Toshiba TB6612FNG Breakout | Physical / Competition Impact |
| :--- | :--- | :--- | :--- |
| **Output Stage Topology** | Bipolar Darlington (BJTs) | DMOS Power MOSFETs | MOSFETs behave as pure low-value resistors when turned ON. |
| **Internal Voltage Drop ($V_{\text{drop}}$)** | **$1.8\text{ V} - 3.2\text{ V}$** ($V_{\text{CE(sat)}}$) | **$\approx 0.35\text{ V} - 0.5\text{ V}$** ($I \cdot R_{\text{DS(on)}}$) | TB6612 delivers over **$93\%$ of LiPo voltage** directly to motors. |
| **Thermal Dissipation ($P_{\text{loss}}$)** | High ($P = V_{\text{drop}} \times I$) $\to$ **$2.5\text{ W} - 4.5\text{ W}$** | Low ($P = I^2 \cdot R_{\text{DS(on)}}$) $\to$ **$\approx 0.4\text{ W}$** | L298N wastes valuable LiPo energy as pure convective heat. |
| **Cooling Hardware** | Requires bulky aluminum heatsink | Passive surface copper heat spreading | Saves chassis space and eliminates chassis payload weight. |
| **PCB Mass & Dimensions** | $\approx 30\text{ g}$ ($43 \times 43 \times 27\text{ mm}$) | **$\approx 1.5\text{ g}$** ($20 \times 20 \times 3\text{ mm}$) | **$95\%$ mass reduction**; keeps robot agile for 500 RPM turns. |
| **Logic Thresholds** | Requires $5.0\text{ V}$ TTL logic | Native $2.7\text{ V} - 5.5\text{ V}$ ($3.3\text{ V}$ native) | Connects directly to ESP32 without level-shifter circuitry. |
| **Continuous Current / Ch** | $2.0\text{ A}$ | $1.2\text{ A}$ ($3.2\text{ A}$ peak pulse) | Fully exceeds N20 stall limit ($0.95\text{ A}$). |
| **Engineering Verdict** | ❌ **REJECTED** | ✅ **SELECTED FOR BOM** | Maximum mechanical efficiency and zero brownout risk. |

---

## 🔬 3. MOSFET H-Bridge Physics & Switching Topologies

An H-Bridge consists of four solid-state switches arranged around a central motor load:

```text
              +VM (7.4V Battery Rail)
                     │
         ┌───────────┴───────────┐
         ▼                       ▼
      [ Q1: P-FET ]           [ Q3: P-FET ]
         │                       │
         ├──────[ GA12-N20 ]─────┤
         │        MOTOR          │
         ▼                       ▼
      [ Q2: N-FET ]           [ Q4: N-FET ]
         │                       │
         └───────────┬───────────┘
                     ▼
             GND (Star Reference)
```

### A. Forward Rotation Mode (Clockwise)
* **Switches Active:** **`Q1` is closed (ON)**, **`Q4` is closed (ON)**. `Q2` and `Q3` are open (OFF).
* **Current Path:** $+\text{VM} \to \text{Q1} \to \text{Motor (+) to (-)} \to \text{Q4} \to \text{GND}$.
* **Result:** Left wheel spins forward.

### B. Reverse Rotation Mode (Counter-Clockwise)
* **Switches Active:** **`Q3` is closed (ON)**, **`Q2` is closed (ON)**. `Q1` and `Q4` are open (OFF).
* **Current Path:** $+\text{VM} \to \text{Q3} \to \text{Motor (-) to (+)} \to \text{Q2} \to \text{GND}$.
* **Result:** Left wheel spins backward. Crucial for zero-radius pivot turns.

### C. Active Short Brake Mode (Dynamic Braking)
* **Switches Active:** **`Q2` and `Q4` are both closed (ON)**; `Q1` and `Q3` are open.
* **Working Principle:** Both motor terminals are shorted to GND. The spinning rotor acts as a generator, converting kinetic energy into electrical energy that circulates through the low-resistance bottom FETs.
* **Result:** The generated counter-torque creates instant **magnetic braking**, locking the wheels within **$< 15\text{ ms}$** without coasting past lines.

### D. Coast / High-Impedance Mode (Free Running)
* **Switches Active:** All four FETs (`Q1`, `Q2`, `Q3`, `Q4`) are open (OFF).
* **Result:** Motor freewheels with zero active resistive torque.

---

## 🛡️ 4. Shoot-Through Prevention & Dead-Time Insertion

* **The Disaster Scenario (Shoot-Through):** If `Q1` and `Q2` (or `Q3` and `Q4`) were to turn ON simultaneously, raw battery voltage ($\text{VM}$) would be shorted straight to $\text{GND}$. This creates a zero-resistance path that vaporizes the silicon switches within microseconds.
* **TB6612FNG Internal Dead-Time Circuit:** The IC incorporates integrated hardware logic delays:
  ```text
  Turn OFF Top FET (Q1) ──► [ Hardware Dead-Time Delay: ~500ns ] ──► Turn ON Bottom FET (Q2)
  ```
  This internal interlock guarantees that one switch is fully quenched before the complementary switch begins conduction.

---

## 📋 5. TB6612FNG Control Logic Truth Table

| `IN1` | `IN2` | `PWM` | `STBY` | Output `OUT1` | Output `OUT2` | Operational Mode | Robot Dynamic Action |
| :---: | :---: | :---: | :---: | :---: | :---: | :--- | :--- |
| **H** | **L** | **H** | **H** | **H** | **L** | **Forward Drive** | Accelerates forward on track |
| **L** | **H** | **H** | **H** | **L** | **H** | **Reverse Drive** | Reverses direction / pivot counter-turn |
| **H** | **H** | **H/L**| **H** | **L** | **L** | **Short Brake** | Instant magnetic braking at $90^\circ$ turns |
| **L** | **L** | **H** | **H** | **OFF (Hi-Z)**| **OFF (Hi-Z)**| **Coast / Float** | Freewheels (sluggish settling) |
| **X** | **X** | **X** | **L** | **OFF (Hi-Z)**| **OFF (Hi-Z)**| **Standby** | Ultra-low power sleep ($I_{CC} < 1\,\mu\text{A}$) |

*Note: In our implementation, PWM speed chopping is conducted via high-frequency $20\text{ kHz}$ ESP32 LEDC channels applied directly to `PWMA` and `PWMB`.*

---

## 📚 References & Authority Sources
- 📄 **Toshiba Semiconductor:** [TB6612FNG IC Technical Datasheet & Block Diagram](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
- 📄 **DroneBot Workshop:** [TB6612FNG Dual H-Bridge Tutorial & Architecture Comparison](https://dronebotworkshop.com/tb6612fng-h-bridge/)
- 📄 **STMicroelectronics:** [L298 Dual Full-Bridge Driver Datasheet](https://www.st.com/en/motor-drivers/l298.html)
- 🛠️ **Author:** Aman Kumar