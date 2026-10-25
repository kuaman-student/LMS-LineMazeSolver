# 📋 Task 4: Bill of Materials & Component Selection

Autonomous line-maze robot architecture for **Techfest Meshmerize (IIT Bombay)**.

---

## 🔗 Quick Links

- 📊 **Google Sheets BOM:** [BOM Sheet](https://docs.google.com/spreadsheets/d/1ojaPZ_JZtjW4_C1dWolM55V-_D-JKaqS8tzH1k0BkUU/edit?gid=0#gid=0)
- 📜 **Competition Rules:** Size $\le 220 \times 220 \times 220\text{ mm}$ | Budget $\le ₹10,000$ | Single start switch | End-zone Red LED

---

## 💰 Budget Snapshot

| Item | Cost (₹) | Details |
| :--- | :---: | :--- |
| **Components (16 items)** | ₹8,056.10 | Core setup + pit spares (dual ESP32, dual LiPo, extra driver) |
| **Shipping** | ₹400.00 | Priority Air to IIT Mandi (PIN 175005) |
| **Contingency (10%)** | ₹805.61 | Test track tape (30mm vinyl), spacers, emergency buffer |
| **Total Project Cost** | **₹9,261.71** | **₹738.28 safely under the ₹10,000 limit** ✅ |

---

## ⚡ Major Choice: TB6612FNG vs. L298N

We selected the **TB6612FNG** over the older **L298N** dual H-bridge driver.

### 📊 Quick Comparison

| Feature | Selected: TB6612FNG | Alternative: L298N | Why It Matters |
| :--- | :---: | :---: | :--- |
| **Voltage Drop** | **~0.2V** (MOSFET) | **~2.0V** (BJT) | Motors receive >7.2V vs only 5.4V on 2S LiPo. |
| **Logic Level** | **3.3V & 5V native** | **Needs 5V TTL** | Direct connection to ESP32 GPIO; no level-shifters. |
| **Size & Weight** | **20 × 20 mm (~5g)** | **43 × 43 mm (~30g)** | Saves space and weight inside the 220mm box constraint. |
| **Thermal Loss** | Runs cool (no heatsink) | Requires heavy aluminum heatsink | Prevents thermal throttling during the 5.5-min run. |
| **Braking** | **Active short brake** | Coast to stop | Prevents overshooting sharp 90° intersections. |

### 🛑 Example: Reaching a 90° Turn

```text
Robot at 500 RPM ──► Line sensors detect 90° junction
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
 [ With L298N ]           [ With TB6612FNG ]
 Coasts to a stop         Active dynamic brake stops instantly
 Slides 2-3 cm past line  Locks precisely on the junction
 ❌ Over-shoots turn       ✅ Executes clean 90° pivot
```

> **Verdict:** The L298N is too bulky and inefficient for battery-powered mini-bots. The TB6612FNG provides maximum motor voltage, cool operation, and precise stops.

---

## 🔌 Power & Signal Architecture

- ⚡ **Interactive Power Diagram:** [Draw.io Live Power Block Diagram](https://drive.google.com/file/d/1fKkfYH_CU_g1Yqtz-ct_hp4HrwZmPFUk/view?usp=sharing)

```text
[ 2S LiPo Battery: 7.4V Nom / 8.4V Peak ]
                   │
                   ▼
       [ SPST Master Rocker Switch ] (Rule 2 Compliance)
                   │
         ┌─────────┴───────────────────────────────────────────────────────┐
         ▼ (VBAT: 7.4V - 8.4V Unregulated)                                 ▼
┌──────────────────────────────────────┐                ┌──────────────────────────────────────┐
│       TB6612FNG Motor Driver         │                │     Mini MP1584EN Buck Converter     │
│             (VM Pin)                 │                │       (Step-Down to 5.0V Reg.)       │
└──────────────────┬───────────────────┘                └──────────────────┬───────────────────┘
                   │                                                       │
                   ▼ (Motor Outputs)                                       │ (5.0V Regulated Rail)
┌──────────────────────────────────────┐                   ┌───────────────┴───────────────┐
│       Left & Right N20 Motors        │                   ▼                               ▼
└──────────────────────────────────────┘        ┌─────────────────────┐         ┌─────────────────────┐
                                                │   ESP32 DevKit V1   │         │    Pololu QTR-8A    │
                                                │      (VIN Pin)      │         │   8-Ch IR Array     │
                                                └──────────┬──────────┘         └──────────┬──────────┘
                                                           │                               │
                                  ┌────────────────────────┴─────────┐                     │ (Analog/GPIO)
                                  ▼ (3.3V Logic PWM / DIR)           ▼ (GPIO Output)       ▼
                       ┌──────────────────────┐           ┌────────────────────┐   ┌───────────────┐
                       │ TB6612FNG Logic Ctrl │           │ 220Ω + Red LED     │◄──┤ ESP32 ADC Pins│
                       │ (PWMA, PWMB, IN1-4)  │           │ (Rule 3 End-Zone)  │   └───────────────┘
                       └──────────────────────┘           └────────────────────┘

================================ COMMON GROUND (GND) ================================
* All modules (Battery, Driver, Motors, Buck Converter, ESP32, Sensors, LED) share GND.
---

## 📓 Handwritten Notes

| Undestanding Basics of BOM |
|:---:|
| ![Note 1](../../docs/Handwritten%20Notes/Task4/image1.png) |

---

## 🤖 Credits

- 🛠️ **Hardware Architecture & BOM:** Aman Kumar
- 📊 **Spreadsheet:** [Live Google Sheet](https://docs.google.com/spreadsheets/d/1ojaPZ_JZtjW4_C1dWolM55V-_D-JKaqS8tzH1k0BkUU/edit?gid=0#gid=0)
- ⚡ **Interactive Power Diagram:** [Draw.io Live Power Block Diagram](https://drive.google.com/file/d/1fKkfYH_CU_g1Yqtz-ct_hp4HrwZmPFUk/view?usp=sharing)
- 📝 **Documentation:** Prepared with assistance from Gemini
- 🔍 **Verification:** Verified against Toshiba TB6612FNG, ST L298N, and Espressif datasheets









