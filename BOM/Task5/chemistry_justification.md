# 🔋 Part 3 — Battery Chemistry Justification (LiPo vs. Li-ion 18650)

Comparative evaluation and trade-off analysis justifying the selection of a **7.4V 2S 1500mAh 25C Lithium Polymer (LiPo)** pack over a **2S 18650 Lithium-Ion (Li-ion)** configuration for Techfest Meshmerize.

---

## 🎯 Benchmark Summary

```text
                      Battery Architecture Decision
                                    │
        ┌───────────────────────────┴───────────────────────────┐
        ▼                                                       ▼
[ 7.4V 2S 1500mAh 25C LiPo ]                         [ 2x 18650 Li-ion in Series ]
✅ Ultra-low internal resistance (15 mΩ)             ❌ High internal resistance (~190 mΩ)
✅ Zero voltage sag (ΔV = 0.034 V)                   ❌ Severe voltage sag (ΔV = 0.431 V)
✅ 16.5x Stall discharge headroom (37.5 A)           ❌ Borderline C-rate margin (3.0–5.0 A)
✅ Lightweight (85 g), low-CG pouch profile          ❌ Bulky (120 g), top-heavy cylindrical pack
```

---

## 📊 Comprehensive Engineering Comparison Matrix

| Evaluation Parameter | Orange 7.4V 2S LiPo (Selected) | 2x 18650 Li-ion Series (Evaluated Alternative) | Impact on Competition Robot |
| :--- | :--- | :--- | :--- |
| **Cell Construction** | Laminated aluminum-polymer foil pouch | Rigid steel cylindrical cans ($18\times65\text{ mm}$) | Pouch has flat profile; cylinders force tall stacking. |
| **Nominal Voltage / Cap.** | $7.4\text{ V}$ ($2\text{S}$) / $1500\text{ mAh}$ | $7.4\text{ V}$ ($2\text{S}$) / $2200 - 2600\text{ mAh}$ | Li-ion offers more raw mAh, but excess is unneeded. |
| **Discharge C-Rating** | **$25\text{C}$ continuous ($50\text{C}$ burst)** | **$1\text{C} - 2\text{C}$ (Standard budget cells)** | LiPo delivers massive burst headroom without stress. |
| **Max Continuous Current** | **$37.5\text{ A}$** ($1.5\text{ Ah} \times 25\text{C}$) | **$3.0\text{ A} - 5.0\text{ A}$** ($2.5\text{ Ah} \times 2\text{C}$) | Li-ion runs near its thermal limit during dual stalls. |
| **Internal Resistance ($R_{\text{int}}$)** | **$\approx 15\text{ m}\Omega$** ($0.015\,\Omega$) total | **$\approx 190\text{ m}\Omega$** ($0.190\,\Omega$ cells + sled springs) | $12.6\times$ higher resistance in Li-ion setup. |
| **Voltage Sag at Stall ($2.27\text{ A}$)** | **$\mathbf{0.034\text{ V}}$** (Imperceptible) | **$\mathbf{0.431\text{ V}}$** (Critical transient dip) | Li-ion sag triggers ESP32 brownout detector. |
| **Subsystem Weight** | **$\approx 85\text{ g}$** (Pack + XT60 pigtail) | **$\approx 120\text{ g}$** ($95\text{ g}$ cells + $25\text{ g}$ dual sled) | Li-ion adds $41\%$ extra mass to chassis payload. |
| **Center of Gravity (CG)** | Ultra-low flat mount on base plate | Elevated center of gravity | Li-ion induces body-roll during 500 RPM turns. |
| **Vibration Reliability** | Welded tabs + soldered XT60 lead | Mechanical coil contact springs in sled | Sled springs bounce over tile seams $\to$ MCU reset! |
| **Cost** | ₹1,239 (High discharge pack) | ₹650–₹800 (Cells + sled + BMS) | LiPo is well within ₹10,000 budget ceiling. |
| **Verdict** | **✅ APPROVED & SELECTED** | **❌ REJECTED** | Proven anti-brownout reliability. |

---

## ⚡ Mathematical Proofs & Physics Rationale

### 1. 🔹 C-Rating & Stall Headroom Proof

The C-rating defines safe continuous discharge relative to capacity:

$$I_{\text{max}} = C_{\text{capacity}} \times C_{\text{rating}}$$

For our selected Orange $1500\text{ mAh}$ $25\text{C}$ LiPo:

$$I_{\text{max}} = 1.50\text{ Ah} \times 25\text{C} = \mathbf{37.5\text{ A}}$$

Comparing against our worst-case dual-motor stall current ($I_{\text{stall}} = 2.27\text{ A}$):

$$\text{Discharge Safety Margin} = \frac{I_{\text{max}}}{I_{\text{stall}}} = \frac{37.5\text{ A}}{2.27\text{ A}} \approx \mathbf{16.51\times \text{ Headroom}}$$

> **Engineering Takeaway:** The LiPo operates at barely **$6\%$ of its rated continuous discharge capacity** even during extreme rotor-lock events, ensuring zero thermal stress, zero cell swelling, and prolonged cycle life.

---

### 2. 🔹 Voltage Sag ($\Delta V_{\text{sag}}$) & Anti-Brownout Protection

When motors draw transient current surges, terminal voltage sags across the battery's internal resistance ($R_{\text{int}}$):

$$V_{\text{terminal}} = V_{\text{OC}} - (I_{\text{load}} \cdot R_{\text{int}})$$

$$\Delta V_{\text{sag}} = I_{\text{load}} \cdot R_{\text{int}}$$

#### Under Worst-Case Dual Stall ($I_{\text{stall}} = 2.270\text{ A}$):

- **LiPo Configuration:**
  $$\Delta V_{\text{sag(LiPo)}} = 2.270\text{ A} \times 0.015\,\Omega = \mathbf{0.034\text{ V}}$$
  * Terminal voltage dips from $7.400\text{ V} \to 7.366\text{ V}$.
  * The MP1584EN step-down buck converter sees an uninterrupted input rail, maintaining a rock-solid $5.0\text{ V}$ and $3.3\text{ V}$ supply.

- **18650 Li-ion Sled Configuration:**
  $$\Delta V_{\text{sag(Li-ion)}} = 2.270\text{ A} \times 0.190\,\Omega = \mathbf{0.431\text{ V}}$$
  * Terminal voltage plummets from $7.400\text{ V} \to 6.969\text{ V}$.
  * Combined with high-frequency $L \cdot \frac{di}{dt}$ inductive motor noise, this massive transient drop induces ripple that passes through low-cost regulators, triggering the **ESP32 Internal Brownout Detector ($V_{\text{threshold}} \approx 2.8\text{ V}$)** and rebooting the robot mid-run.

---

### 3. 🔹 Weight, Rotational Inertia & Chassis Center of Gravity

Maze solving at 500 RPM requires rapid angular acceleration during $90^\circ$ turns. Rotational inertia depends directly on mass and distance from the pivot axis:

$$J = m \cdot r^2$$

- **LiPo Pouch ($\approx 85\text{ g}$):** 
  Flat $14\text{ mm}$ thickness mounts directly flush to the bottom acrylic plate between the drive wheels. This achieves an **ultra-low Center of Gravity (CG)**, eliminating chassis body roll and tire unweighting.
- **18650 Pack ($\approx 120\text{ g}$):** 
  Dual cylindrical cells ($18\text{ mm}$ diameter $\times 65\text{ mm}$ length) plus plastic carrier sled require elevated mounting. This raises the center of mass, increasing rotational momentum ($J$) and causing the robot to skid or overturn during high-speed pivot turns.

---

### 4. 🔹 Mechanical Failure Mode: Contact Bounce

- **18650 Sled Risk:** Typical battery holders use coil springs to hold cells in place. When the robot hits floor seams or arena joints at high speed, spring vibration momentarily breaks physical contact for $1 - 5\text{ ms}$ (**contact chatter**), causing instant system power-down.
- **LiPo Reliability:** The LiPo features factory spot-welded cell tabs terminated in a locking **high-current XT60 connector**, guaranteeing zero intermittent disconnects under severe vibration.

---

## 🏁 Final Engineering Justification Verdict

| Decision Aspect | Summary Justification |
| :--- | :--- |
| **Why LiPo Was Selected** | **$16.5\times$ current headroom**, negligible voltage sag ($\mathbf{0.034\text{ V}}$), flat low-CG profile, and vibration-proof XT60 connection eliminate brownouts and maximize cornering stability. |
| **Why Li-ion Was Rejected** | Excessive internal resistance creates a dangerous **$0.431\text{ V}$ sag** during stalls, battery sled springs risk vibration disconnects, and $41\%$ higher mass degrades turn agility. |

---

## 🔗 Related Documentation

- ⚡ **Part 1:** [Current Draw Estimation & Power Budget](./current_draw_estimation.md)
- ⏱️ **Part 2:** [Battery Runtime Calculations & Derating Analysis](./runtime_calculation.md)
- 📊 **Spreadsheet Model:** [`Power_Budget.xlsx`](https://docs.google.com/spreadsheets/d/1vCYHayypkr-nruddL1RVg_CO_KTvek1Pv8IVOfRQMn0/edit?usp=sharing)