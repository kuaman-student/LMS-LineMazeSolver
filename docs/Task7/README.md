# 📡 Task 7: Wireless Sensor Diagnostics & Telemetry

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Target Platform:** ESP32-WROOM-32 | Pololu QTR-8A Array | Classic Bluetooth SPP  
**Author:** Aman Kumar

---

## 🧭 Executive Overview

This directory contains the protocol evaluation and telemetry architecture designed to isolate intermittent IR sensor failures in real time while navigating uneven maze surfaces. The solution requires **zero stops**, **zero physical contact**, and **zero custom client application development**.

```text
                                [ docs/Task7/ ]
                                       │
        ┌──────────────────────────────┴──────────────────────────────┐
        ▼                                                             ▼
 [ 1_Protocol_Selection ]                                   [ 2_System_Architecture ]
  • Wi-Fi vs BLE vs Classic BT SPP                           • 5-Stage Telemetry Pipeline
  • ADC2 Silicon Lockout Avoidance                           • Spatial Continuity vs Bump Heuristic
  • Latency, Power & Ease Justification                      • Live Mobile / Laptop Visualization
```

---

## 📑 Core Deliverables Index

| Document | Primary Focus | Key Engineering Findings | Status |
| :--- | :--- | :--- | :---: |
| **[Part 1: Protocol Selection](./protocol_selection.md)** | Wireless Trade-Offs (Latency, Power, Complexity) | • **Selected:** Classic Bluetooth (SPP / RFCOMM).<br>• **Why Wi-Fi Rejected:** Locks ADC2 pins and pulls $240\text{ mA}$ RF surges.<br>• **Why BLE Ranked 2nd:** GATT MTU packet slicing and variable connection intervals. | ✅ Complete |
| **[Part 2: System Architecture](./system_architecture.md)** | Data Flow & Outlier Rejection Heuristic | • **Bump vs Drift:** Line drift is spatially smooth; loose pins drop single channels while neighbors stay high.<br>• **Format:** Lightweight ASCII CSV frame ($TLM,time,s1..s8,mask*CS).<br>• **Tooling:** Plug-and-play with Android Bluetooth Terminal & Arduino Plotter. | ✅ Complete |

---

## 🔬 Core Engineering Highlights

### 1. The Protocol Decision: Classic Bluetooth SPP
* **Zero ADC Interference:** Wi-Fi requires exclusive control over SAR ADC2, rendering half the ESP32 analog pins useless. Classic Bluetooth operates over the radio baseband without locking analog peripherals.
* **Deterministic Streaming:** Provides an unbroken virtual serial link at $115,200\text{ baud}$, streaming at $50\text{ Hz}$ ($< 18\text{ ms}$ latency) to capture transient mechanical bounces.
* **Zero Network Dependency:** Operates peer-to-peer without needing Wi-Fi routers or mobile hotspots in crowded competition arenas.

### 2. Spatial Outlier Bump Rejection Heuristic
* **The Physics Invariant:** A legitimate $19\text{ mm}$ line always spans across 2–3 phototransistors on a $9.525\text{ mm}$ pitch array.
* **Outlier Detection:** If a single channel drops to zero while both adjacent neighbors report high reflection, the ESP32 flags that channel as an intermittent hardware fault and broadcasts its bitmask without corrupting the motor PD controller.

### 3. Immediate Out-of-the-Box Visualization
* **Phone Telemetry:** Android *Serial Bluetooth Terminal* connects directly to store continuous millisecond-stamped logs (`.csv`) for post-run audits.
* **Laptop Plotting:** Connects as a virtual serial port to render real-time color traces in the *Arduino IDE Serial Plotter*, immediately pinpointing loose wiring under race conditions.

---

## 📚 References & Authority Sources
- 📄 **Espressif Systems:** [ESP32 Bluetooth Architecture & ADC2 Silicon Limitations](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html)
- 📄 **Pololu Robotics:** [QTR-8A Reflectance Sensor Array Application Note](https://www.pololu.com/docs/0J19)