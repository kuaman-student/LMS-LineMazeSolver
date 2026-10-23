# ⚡ Part 1 — Current Draw Estimation & Power Budgeting

Detailed current consumption breakdown across idle, nominal, and worst-case operating states for the Techfest Meshmerize robot.

---

## 🧭 Voltage Rail Architecture

The system splits power across three dedicated voltage rails to isolate high-current inductive motor spikes from sensitive mixed-signal logic:

```
[ 7.4V 2S LiPo Battery Pack ]
         │
         ├───► [ 7.4V Raw Motor Rail (VM) ] ──────────► TB6612FNG Driver ──► 2x GA12-N20 Motors
         │
         └───► [ MP1584EN Mini Buck Converter ]
                        │
                        ▼ (5.0V Regulated Rail)
                        ├───► Pololu QTR-8A Sensor Array (8x IR Pairs)
                        │
                        └───► ESP32 Onboard LDO Regulator
                                       │
                                       ▼ (3.3V Clean Logic Rail)
                                       ├───► ESP32 Dual Core (Xtensa LX6)
                                       ├───► TB6612FNG Logic Supply (VCC)
                                       └───► End-Zone Red Indicator LED
```

---

## 📊 Three-Tier Power Budget Table

| Subsystem Component | Supply Rail | Standby / Idle Current ($I_{\text{idle}}$) | Nominal Running Current ($I_{\text{nominal}}$) | Worst-Case Stall Current ($I_{\text{stall}}$) | Datasheet / Test Source |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **2x GA12-N20 Gearmotors (500 RPM)** | $7.4\text{ V}$ | $0.0\text{ mA}$ *(H-Bridge High-Z)* | $360.0\text{ mA}$ *(2 × 180 mA under load)* | $1900.0\text{ mA}$ *(2 × 950 mA full lock)* | Pololu Micro Metal Spec Sheet |
| **ESP32-WROOM-32 Microcontroller** | $3.3\text{ V}$ | $80.0\text{ mA}$ *(Dual-core idle, no RF)* | $140.0\text{ mA}$ *(200 Hz PD + ADC reads)* | $240.0\text{ mA}$ *(Peak dual-core active burst)* | Espressif ESP32 Datasheet §5.2 |
| **Pololu QTR-8A Reflectance Array** | $5.0\text{ V}$ | $100.0\text{ mA}$ *(8x 940nm IR LEDs ON)* | $100.0\text{ mA}$ *(Continuous emitter bank)* | $100.0\text{ mA}$ *(Constant current emitter)* | Pololu QTR-8A User Guide §2 |
| **Toshiba TB6612FNG Dual Driver** | $3.3\text{ V}$ | $1.5\text{ mA}$ *(Logic quiescent current)* | $2.0\text{ mA}$ *(Switching logic gate loss)* | $2.0\text{ mA}$ *(Logic only; motor load on VM)* | Toshiba TB6612FNG Datasheet p. 4 |
| **MP1584EN Step-Down Buck Converter** | $7.4\text{ V}$ | $0.5\text{ mA}$ *(No-load quiescent current)* | $10.0\text{ mA}$ *(Internal switching losses)* | $15.0\text{ mA}$ *(High-duty thermal conversion)* | MPS MP1584EN Datasheet p. 2 |
| **End-Zone Red Marker LED (5mm)** | $3.3\text{ V}$ | $0.0\text{ mA}$ *(GPIO LOW)* | $5.5\text{ mA}$ *(Average flashing/PWM duty)* | $13.2\text{ mA}$ *(Continuous ON through 220Ω)* | Cree LED Forward Characteristic |
| **SUMMED TOTALS** | — | **$182.0\text{ mA}$ ($0.18\text{ A}$)** | **$617.5\text{ mA}$ ($0.62\text{ A}$)** | **$2,270.2\text{ mA}$ ($2.27\text{ A}$)** | **Worst-Case Peak Design Limit** |

---

## 🔍 Detailed Operating State Analysis

```
                      System Current Draw Tiers
                                  │
      ┌───────────────────────────┼───────────────────────────┐
      ▼                           ▼                           ▼
[ Standby / Idle ]       [ Nominal Cruise ]       [ Worst-Case Stall ]
  182.0 mA (0.18 A)        617.5 mA (0.62 A)        2,270.2 mA (2.27 A)
  Wait at Start Line       Line Tracking @ 500 RPM   Dual Rotor Lockout
```

### 1. 🔹 Standby / Idle State ($I_{\text{idle}} = 182.0\text{ mA}$)
* **Operational Condition:** Robot placed in the start box awaiting course activation.
* **Component Dynamics:**
  * Motors are completely unpowered ($0\text{ mA}$) with TB6612FNG driver inputs pulled LOW (high-impedance standby mode).
  * ESP32 runs its primary scheduling loop without RF transmission ($~80\text{ mA}$).
  * QTR-8A array draws constant emitter bias ($~100\text{ mA}$) to maintain thermal equilibrium on phototransistors.

### 2. 🔹 Nominal Running State ($I_{\text{nominal}} = 617.5\text{ mA}$)
* **Operational Condition:** Autonomous line-following at 500 RPM on flat vinyl tile surfaces.
* **Component Dynamics:**
  * Motors overcome rolling resistance and dynamic turning inertia, pulling an average of $180\text{ mA}$ per channel ($360\text{ mA}$ total).
  * ESP32 runs continuous ADC sampling and discrete-time PD math at $200\text{ Hz}$ ($140\text{ mA}$).
  * MP1584EN buck converter dissipates switching losses operating at $\sim 90\%$ efficiency ($10\text{ mA}$).
  * Red LED activates intermittently during intersection detection ($~5.5\text{ mA}$ average).

### 3. 🔹 Worst-Case Stall State ($I_{\text{stall}} = 2,270.2\text{ mA} \approx 2.27\text{ A}$)
* **Operational Condition:** Dual drive wheels physically stalled against an obstacle or executing a violent counter-rotating pivot at a $90^\circ$ dead end.
* **Component Dynamics:**
  * Both GA12-N20 micro gearmotors enter full rotor-lock, drawing their maximum stall rating ($2 \times 950\text{ mA} = 1900\text{ mA}$).
  * ESP32 cores experience peak execution load while handling recovery watchdog routines ($240\text{ mA}$).
  * Red LED illuminates continuously at full forward drive ($13.2\text{ mA}$).
* **Actuator Margin Check:** The TB6612FNG driver is rated for **$3.2\text{ A}$ peak pulse current per channel** and **$1.2\text{ A}$ continuous**, comfortably operating above the $0.95\text{ A}$ stall limit of the GA12-N20 motor.

---

## 🧮 Summary Metrics

* **Idle Power Consumption:** $P_{\text{idle}} = 7.4\text{ V} \times 0.182\text{ A} \approx \mathbf{1.35\text{ W}}$
* **Nominal Cruise Power:** $P_{\text{nominal}} = 7.4\text{ V} \times 0.618\text{ A} \approx \mathbf{4.57\text{ W}}$
* **Peak Stall Power:** $P_{\text{peak}} = 7.4\text{ V} \times 2.270\text{ A} \approx \mathbf{16.80\text{ W}}$

---

## 🔗 Related Documentation
- 📊 **Downloadable Spreadsheet Model:** [`Power_Budget.xlsx`](https://docs.google.com/spreadsheets/d/1vCYHayypkr-nruddL1RVg_CO_KTvek1Pv8IVOfRQMn0/edit?usp=sharing)
- ⏱️ **Next Section:** [Part 2: Runtime Calculations & Derating Assumptions](./runtime_calculation.md)
- 🔋 **Battery Sizing:** [Part 3: Cell Chemistry Justification (LiPo vs. Li-ion)](./chemistry_justification.md)