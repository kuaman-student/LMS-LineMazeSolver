# ⚡ Task 5: Power Budget & Battery Selection

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Target Platform:** ESP32-WROOM-32 | 2x GA12-N20 (500 RPM) | Orange 7.4V 2S 1500mAh 25C LiPo  
**Author:** Aman Kumar (IIT Mandi)  
**Deliverable Weight:** 30 Points

---

## 🧭 Executive Dashboard

This directory contains the complete electrical power characterization, runtime endurance calculations, and battery chemistry trade-off analysis for our competition robot.

```text
                            [ docs/Task5/ ]
                                   │
   ┌───────────────────────────────┼───────────────────────────────┐
   ▼                               ▼                               ▼
[ 1_Current_Draw ]        [ 2_Runtime_Calc ]           [ 3_Chemistry_Justify ]
 Three-Tier Summation      Derating Factor (0.75)       LiPo vs. Li-ion 18650
 182mA / 618mA / 2.27A     109.3 Min (~36 Runs)         16.5x Headroom / Anti-Sag
   │                               │                               │
   └───────────────────────────────┼───────────────────────────────┘
                                   ▼
              [ assets/Task5_Power_Budget_and_Battery.xlsx ]
```

---

## 📑 Core Deliverables Index

| Document | Focus Question | Key Equations & Results | Status |
| :--- | :--- | :--- | :---: |
| **[Part 1: Current Draw Estimation](./current_draw_estimation.md)** | **Q1:** Idle, Nominal & Peak Summation | $I_{\text{idle}} = \mathbf{182.0\text{ mA}}$<br>$I_{\text{nominal}} = \mathbf{617.5\text{ mA}}$<br>$I_{\text{stall}} = \mathbf{2,270.2\text{ mA} \ (2.27\text{ A})}$ | ✅ Complete |
| **[Part 2: Runtime Calculations](./runtime_calculation.md)** | **Q2:** Endurance & Derating ($\eta = 0.75$) | $\text{Runtime} = \left(\frac{1500\text{ mAh}}{617.5\text{ mA}}\right) \times 0.75 = \mathbf{109.3\text{ min}}$<br>$\text{Competition Endurance} \approx \mathbf{36\text{ Full Runs}}$ | ✅ Complete |
| **[Part 3: Chemistry Justification](./chemistry_justification.md)** | **Q3:** LiPo Pouch vs. Li-ion 18650 | $I_{\text{max}} = 1.5\text{ Ah} \times 25\text{C} = \mathbf{37.5\text{ A } (16.5\times\text{ margin})}$<br>$\Delta V_{\text{sag(LiPo)}} = \mathbf{0.034\text{ V}}$ vs. $\Delta V_{\text{sag(18650)}} = \mathbf{0.431\text{ V}}$ | ✅ Complete |

---

## 🔬 Key Engineering Takeaways

### 1. Three-Rail Power Topology
- **7.4V Unregulated Rail (VM):** Powers TB6612FNG motor bridges directly from LiPo to minimize dynamic IR losses.
- **5.0V Switching Rail:** Regulated via high-efficiency MP1584EN buck converter to power the Pololu QTR-8A IR LED bank.
- **3.3V Logic Rail:** Powers ESP32 dual-core MCU and TB6612FNG logic ($V_{CC}$), eliminating inductive motor noise crosstalk.

### 2. Runtime & Derating Assumptions ($\eta = 0.75$)
- **80% Depth-of-Discharge (DoD):** Hard cutoff at $3.3\text{ V/cell}$ prevents permanent cell puffing and lithium plating.
- **Peukert & Thermal Losses:** Accounts for internal $I^2R$ battery losses under rapid 500 RPM motor reversals and $\approx 90\%$ buck converter conversion efficiency.
- **Operational Reality:** A typical competition maze run takes **2–3 minutes**. A single charge provides **~36 full runs**, eliminating the need for pit-stop battery swaps.

### 3. Why LiPo Won (and 18650 Was Rejected)
- **Zero Voltage Sag ($\Delta V = 0.034\text{ V}$):** Ultra-low internal resistance ($15\text{ m}\Omega$) prevents motor stall current spikes ($2.27\text{ A}$) from dipping the 7.4V rail, protecting the ESP32 from brownout resets ($V_{\text{reset}} \approx 2.8\text{ V}$).
- **Vibration Immunity:** Factory-welded tabs with locking XT60 connector eliminate the contact chatter seen in 18650 battery holder coil springs.
- **Low Center of Gravity:** An 85g flat pouch mounted flush to the chassis base plate reduces rotational inertia ($J = m \cdot r^2$), preventing body roll during high-speed $90^\circ$ pivot turns.

---

## 📊 Interactive Artifacts

- 📥 **Interactive Excel Workbook:** [`Task5_Power_Budget_and_Battery.xlsx`](https://docs.google.com/spreadsheets/d/1vCYHayypkr-nruddL1RVg_CO_KTvek1Pv8IVOfRQMn0/edit?usp=sharing)  
  *(Contains dynamic `SUM`, `V*I`, `DoD`, and conditional formatting for all load scenarios)*.

---

## 📚 References & Authority Sources

1. **Battery University (Isidor Buchmann):**  
   - [BU-402: What Is C-rate?](https://batteryuniversity.com/article/bu-402-what-is-c-rate)  
   - [BU-205: Types of Lithium-ion](https://batteryuniversity.com/article/bu-205-types-of-lithium-ion)
2. **Espressif Systems:** [ESP32 Hardware Design Guidelines — Power Scheme & Brownout Detection](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/esp-hardware-design-guidelines-en-master-esp32.pdf)
3. **Toshiba Semiconductor:** [TB6612FNG Motor Driver Electrical Characteristics Datasheet](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)