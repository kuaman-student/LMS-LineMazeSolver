# ⏱️ Part 2 — Battery Runtime Calculations & Derating Analysis

Engineering estimation of robot operating endurance under typical maze navigation and worst-case continuous stall loading.

---

## 📌 TL;DR Summary

$$\text{Runtime (Hours)} = \left( \frac{\text{Battery Capacity (mAh)}}{\text{Average System Current (mA)}} \right) \times \eta \quad (\eta = 0.75)$$

```text
[ Nominal 1500 mAh Pack ] ──► [ Usable Margin: 80% DoD ] ──► [ Buck Losses & Peukert (0.75) ]
                                                                       │
┌─────────────────────────────────┬────────────────────────────────────┘
▼                                 ▼
Nominal Maze Runtime:             Worst-Case Continuous Stall:
1.82 Hours (~109.3 Minutes)       0.495 Hours (~29.7 Minutes)
~36 Full Competition Runs         Continuous Mechanical Jam Headroom
```

---

## 🧮 Core Governing Formula & Derivation

Standard theoretical runtime ($T = \frac{C}{I}$) assumes ideal discharge down to absolute zero under lab-bench static currents. In dynamic robotics, a **Derating Factor ($\eta = 0.75$)** is mandatory:

$$\text{Runtime (Hours)} = \left( \frac{C_{\text{nominal}}}{I_{\text{system}}} \right) \times \eta_{\text{derate}}$$

$$\text{Runtime (Minutes)} = \text{Runtime (Hours)} \times 60$$

### Key Battery Specifications (Orange 2S LiPo)
* **Nominal Pack Voltage ($V_{\text{nom}}$):** $7.4\text{ V}$ ($3.7\text{ V / cell}$)
* **Fully Charged Terminal Voltage ($V_{\text{max}}$):** $8.4\text{ V}$ ($4.2\text{ V / cell}$)
* **Nameplate Capacity ($C_{\text{nominal}}$):** $1500\text{ mAh } (1.50\text{ Ah})$
* **Total Stored Energy:**
  $$E_{\text{total}} = 1.50\text{ Ah} \times 7.4\text{ V} = \mathbf{11.10\text{ Watt-hours (Wh)}}$$

---

## ⚙️ Stated Engineering Assumptions ($\eta = 0.75$)

The combined multiplier $\eta = 0.75$ accounts for three critical real-world phenomena:

```text
             Derating Factor Breakdown (η = 0.75)
                             │
     ┌───────────────────────┼───────────────────────┐
     ▼                       ▼                       ▼
[ 80% DoD Reserve ]     [ Peukert's Effect ]    [ Buck Converter Losses ]
  Cutoff at 3.3V/cell     Capacity drops under    ~90% efficiency on
  Prevents pack puffing   dynamic high-current    5.0V step-down rail
```

### 1. 🔹 80% Depth-of-Discharge (DoD) Safety Reserve
Discharging a lithium polymer cell below $3.0\text{ V}$ initiates irreversible chemical degradation:
* Copper dendrites dissolve and short across the separator.
* Delamination of active cathode material leads to gas generation (**pack puffing**).
* To guarantee pack health and multi-cycle endurance, our firmware enforces a cutoff floor at $3.3\text{ V / cell}$ ($6.6\text{ V}$ pack), utilizing only **$80\%$ usable capacity** ($1,200\text{ mAh}$).

### 2. 🔹 Peukert’s Law & Dynamic Load Inefficiencies
Manufacturer capacity ratings ($1,500\text{ mAh}$) are tested at low $0.2\text{C}$ rates ($300\text{ mA}$). Rapid motor accelerations, sharp $90^\circ$ turns, and high-frequency PWM switching increase internal resistive $I^2 R$ heat losses, reducing effective delivered milliamp-hours by approximately **$5–8\%$**.

### 3. 🔹 Switching Regulator Thermal Dissipation
The MP1584EN step-down buck converter powering the 5V and 3.3V rails operates at approximately **$90\%$ efficiency**. Stepping down from $7.4\text{ V}$ introduces thermal switching losses across the inductor and diode.

$$\eta_{\text{total}} \approx 0.80 \times 0.95 \times 0.98 \approx \mathbf{0.75}$$

---

## 🔢 Numerical Calculations

### Scenario A: Nominal Maze-Solving Operation
Under active autonomous navigation at 500 RPM, the system draws nominal current $I_{\text{nominal}} = \mathbf{617.5\text{ mA}}$:

$$\text{Runtime}_{\text{nominal}} = \left( \frac{1500\text{ mAh}}{617.5\text{ mA}} \right) \times 0.75 = 2.429 \times 0.75 \approx \mathbf{1.82\text{ Hours}}$$

$$\text{Runtime}_{\text{nominal}} = 1.822 \times 60 \approx \mathbf{109.3\text{ Minutes}}$$

* **Usable Energy Delivered:** $11.10\text{ Wh} \times 0.75 \approx \mathbf{8.33\text{ Wh}}$

---

### Scenario B: Worst-Case Continuous Dual-Motor Stall
If both GA12-N20 drive wheels become locked against an arena obstacle, current jumps to $I_{\text{stall}} = \mathbf{2,270.2\text{ mA}}$:

$$\text{Runtime}_{\text{stall}} = \left( \frac{1500\text{ mAh}}{2270.2\text{ mA}} \right) \times 0.75 = 0.6607 \times 0.75 \approx \mathbf{0.495\text{ Hours}}$$

$$\text{Runtime}_{\text{stall}} = 0.495 \times 60 \approx \mathbf{29.7\text{ Minutes}}$$

> **Safety Note:** Although the battery can sustain this for nearly 30 minutes, firmware watchdog limits motor stall duration to **1.2 seconds** to prevent motor winding insulation breakdown.

---

## 🏁 Practical Techfest Meshmerize Operational Context

In competitive maze-solving, match rounds are short, high-intensity sprints:

| Competition Metric | Value | Operational Implication |
| :--- | :---: | :--- |
| **Typical Maze Run Duration** | $2.0 - 3.0\text{ Minutes}$ | Single round run time |
| **Full Competition Runs / Charge** | **$\approx 36\text{ Runs}$** | ($\approx 109.3\text{ min} / 3.0\text{ min}$) |
| **Pit-Stop Battery Swaps Needed** | **Zero** | Entire heat / tournament run on single charge |
| **Recharge Cycle Time (IMAX B3)** | $\approx 90\text{ Minutes}$ | Quick turnaround using 2nd spare battery in BOM |

---

## 📊 Operational Comparison Matrix

| Load Condition | System Current | Raw Theoretical Time | Derated Time ($\eta = 0.75$) | Equivalent Maze Runs |
| :--- | :---: | :---: | :---: | :---: |
| **Standby / Idle** | $182.0\text{ mA}$ | $8.24\text{ Hours}$ | **$6.18\text{ Hours}$** ($370.9\text{ min}$) | N/A (Waiting in Pits) |
| **Nominal Maze Run** | $617.5\text{ mA}$ | $2.43\text{ Hours}$ | **$1.82\text{ Hours}$** ($109.3\text{ min}$) | **~36 Full Runs** |
| **Worst-Case Stall** | $2,270.2\text{ mA}$ | $0.66\text{ Hours}$ | **$0.49\text{ Hours}$** ($29.7\text{ min}$) | Extreme Fault State |

---

## 🔗 Related Documentation

* ⚡ **Previous Section:** [Part 1: Current Draw Estimation & Power Budget](./current_draw_estimation.md)
* 🔋 **Next Section:** [Part 3: Cell Chemistry Justification (LiPo vs. Li-ion)](./chemistry_justification.md)
* 📊 **Spreadsheet Model:** - [`Power_Budget.xlsx`](https://docs.google.com/spreadsheets/d/1vCYHayypkr-nruddL1RVg_CO_KTvek1Pv8IVOfRQMn0/edit?usp=sharing)