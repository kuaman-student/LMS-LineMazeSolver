# ⚡ Task 6: Electronics, Hardware & Troubleshooting Architecture

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Target Platform:** ESP32-WROOM-32 | Toshiba TB6612FNG Driver | Pololu QTR-8A Array | GA12-N20 Motors  
**Author:** Aman Kumar 

---

## 🧭 Executive Overview

This directory contains the electrical subsystem architecture, complete pin-to-pin wiring maps, power rail isolation schemas, common-ground physics, motor driver solid-state comparative analysis, and an in-depth troubleshooting diagnostic protocol for controller brownout resets.

```text
                                [ docs/Task6/ ]
                                        │
        ┌───────────────────────────────┼───────────────────────────────┐
        ▼                               ▼                               ▼
 [ 1_Wiring_Diagram ]         [ 2_Driver_Architecture ]     [ 3_Troubleshooting_Brownout ]
  • 3 Isolated Power Rails     • Low-Drop MOSFET vs BJT      • Dual Root Causes (Sag + EMI)
  • Pin-to-Pin Interconnects   • TB6612FNG vs L298N Matrix   • DSO 3.3V Trigger Protocol
  • Single Star Ground (0V)    • H-Bridge Physics & Braking  • 3-Tier Hardware Decoupling
```

---

## 📑 Directory Index & Core Findings

| Sub-Document | Primary Focus | Key Takeaways & Engineering Specs | Direct Link |
| :--- | :--- | :--- | :---: |
| **Part 1: Electrical Wiring & Power Rails** | Interconnections & Common-Ground | • **3 Voltage Rails:** 7.4V Motor Raw, 5.0V Buck-Regulated (Sensors/MCU), 3.3V Logic.<br>• **Star Ground:** Common 0V reference eliminates floating MOSFET gates.<br>• **ADC1 Pin Isolation:** All 8 sensor channels route strictly to ADC1. | [Open Part 1](./wiring_diagram.md) |
| **Part 2: Motor Driver Topology & Physics** | TB6612FNG vs. L298N Benchmark | • **$V_{\text{drop}}$ Comparison:** TB6612FNG drops $\approx 0.35\text{V}$ (DMOS) vs L298N dropping $1.8\text{V}-3.2\text{V}$ (BJT).<br>• **Thermal/Weight:** Zero heatsink needed, saving $28.5\text{g}$ ($95\%$ mass cut).<br>• **Active Dynamic Brake:** Instant magnetic stop in $<15\text{ms}$. | [Open Part 2](./motor_driver_architecture.md) |
| **Part 3: Brownout Fault Troubleshooting** | Hardware Diagnostics & Fixes | • **Failure Mode:** Inrush surge ($~2.0\text{A}$) sags battery rail; logic dips below $2.80\text{V}$, tripping ESP32 brownout detector.<br>• **1st Diagnostic:** DSO single-sweep trigger on 3.3V falling edge at $3.0\text{V}$.<br>• **3-Tier Fix:** $470\,\mu\text{F}$ bulk cap at VM + $10\,\mu\text{F}/0.1\,\mu\text{F}$ local + Pololu 3-cap snubber. | [Open Part 3](./brownout_troubleshooting.md) |

---

## ⚡ Key Architectural Specifications

```text
       BATTERY RAIL (7.4V RAW)              LOGIC / SENSOR RAIL (5.0V BUCK)          MCU INTERNAL RAIL (3.3V LDO)
   ┌──────────────────────────────┐        ┌──────────────────────────────┐        ┌──────────────────────────────┐
   │ • Orange 2S 1500mAh 25C LiPo │        │ • MP1584EN High-Freq Buck    │        │ • Low-Dropout 3.3V Regulator │
   │ • TB6612FNG VM Motor Supply  │        │ • Pololu QTR-8A VCC (Clean)  │        │ • ESP32 Core Logic & ADC     │
   │ • 470µF Low-ESR Bulk Bypass  │        │ • ESP32 VIN Power Supply     │        │ • TB6612FNG VCC & STBY Pins  │
   └──────────────────────────────┘        └──────────────────────────────┘        └──────────────────────────────┘
                                          ▲                                       ▲
                                          └────────── COMMON STAR GROUND ─────────┘
                                                       (Single Low-Z 0V Ref)
```

---

## 🛠️ Verification Checklist for Judges

- [x] **Supply Voltage Separation:** Complete isolation between noisy $7.4\text{V}$ motor current paths and sensitive $5.0\text{V}/3.3\text{V}$ logic buses.
- [x] **Common-Ground Proof:** Physical star-point grounding eliminates parasitic ground bounce and false gate triggering.
- [x] **Motor Driver Justification:** Quantitative efficiency comparison proving why TB6612FNG was selected over L298N.
- [x] **Oscilloscope-Verified Troubleshooting:** Clear diagnostic trigger setup and multi-layer passive filter validation.

---

## 📚 References & Authority Sources

1. **Toshiba Semiconductor:** [TB6612FNG IC Datasheet & Application Notes](https://toshiba.semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG)
2. **Pololu Robotics:** [Application Note 9: Dealing with Motor Noise & Snubbers](https://www.pololu.com/docs/0J15/9)
3. **Espressif Systems:** [ESP32 Hardware Design Guidelines — Power & Brownout Schemes](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/esp-hardware-design-guidelines-en-master-esp32.pdf)