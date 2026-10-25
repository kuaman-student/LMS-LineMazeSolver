# 🛰️ Part 2 — Telemetry Pipeline & Bump Rejection Architecture

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Subsystem:** Real-Time Data Flow, Outlier Rejection Heuristic & Visualization  
**Author:** Aman Kumar

---

## 📌 Executive Summary

When racing over uneven vinyl arena tiles, mechanical bumps cause phototransistors to lose focus, and track vibrations cause loose jumper pins to flicker. This document details the **end-to-end telemetry architecture** that identifies faulty sensors in real time without stopping the robot, distinguishing mechanical bump disturbances from legitimate trajectory shifts.

```text
 ┌───────────────┐     ┌───────────────────────┐     ┌────────────────────────┐
 │ 8x QTR-8A     │────►│ ESP32 Core 0: ADC1    │────►│ Spatial & Temporal     │
 │ Sensor Array  │     │ 100 Hz Hardware Scan  │     │ Bump-Rejection Engine  │
 └───────────────┘     └───────────────────────┘     └───────────┬────────────┘
                                                                 │
 ┌───────────────┐     ┌───────────────────────┐                 │
 │ Phone/PC GUI  │◄────│ Classic Bluetooth SPP │◄────────────────┘
 │ Live Waveform │     │ RFCOMM Stream (115kB) │  Formatted CSV Telemetry Frame
 └───────────────┘     └───────────────────────┘
```

---

## 🔬 1. Physics & Heuristic: Distinguishing Bumps from True Line Shifts

A line tracker must differentiate between a robot drifting across a line and an intermittent hardware/surface fault.

```text
A. LEGITIMATE LINE DRIFT (Spatial Continuity)
   Sensor Index:   [ S1 ]    [ S2 ]    [ S3 ]    [ S4 ]    [ S5 ]    [ S6 ]    [ S7 ]    [ S8 ]
   Reflectance:      50       120       480       980       520       110        60        40
                                         ▲         ▲         ▲
                                         └── Smooth Gaussian ┘

B. MECHANICAL BUMP / LOOSE PIN (Spatial Outlier)
   Sensor Index:   [ S1 ]    [ S2 ]    [ S3 ]    [ S4 ]    [ S5 ]    [ S6 ]    [ S7 ]    [ S8 ]
   Reflectance:      50       110       490       000!      510       115        55        40
                                                   ▲
                                     Single-channel drop to 0!
                                     Neighbors remain high (S3=490, S5=510)
```

### The Analytical Rules

1. **Legitimate Line Shift (Spatial Smoothness):**
   * The white line ($19\text{ mm}$ width) spans across at least 2–3 adjacent phototransistors ($9.525\text{ mm}$ pitch).
   * As the robot steers, analog intensity transfers smoothly:
     $$S_i \text{ decreases} \implies S_{i+1} \text{ increases}$$
   * The spatial gradient between neighbors remains bounded:
     $$|S_i - S_{i \pm 1}| < \Delta_{\text{spatial, max}}$$

2. **Intermittent Fault / Loose Wire (Spatial Outlier):**
   * A mechanical vibration or loose pin causes a single channel to drop to $0$ (open-circuit pull-down) or spike to $4095$ (floating rail) in $< 5\text{ ms}$.
   * **The Diagnostic Invariant:** If channel $S_i$ abruptly plummets while both immediate neighbors $S_{i-1}$ and $S_{i+1}$ remain illuminated ($> 300$), **channel $S_i$ is flagged as mechanically/electrically faulty**:
     $$\text{Fault Flag } F_i = 1 \iff \left( |S_i[k] - S_i[k-1]| > 400 \right) \land \left( \frac{S_{i-1} + S_{i+1}}{2} > 300 \right)$$

---

## ⚡ 2. Five-Stage Telemetry Pipeline

```text
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ STAGE 1: Multi-Channel ADC1 Acquisition (ESP32 Core 0)                       │
 │ • Reads 8 analog reflectance channels sequentially on ADC1                   │
 │ • Normalizes 12-bit ADC raw counts (0-4095) to calibrated scale (0-1000)     │
 └──────────────────────────────────────┬───────────────────────────────────────┘
                                        │
                                        ▼
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ STAGE 2: Real-Time Diagnostic Engine (Edge Detection)                        │
 │ • Computes temporal gradient (rate of change dS/dt)                          │
 │ • Evaluates spatial neighbor coherence                                       │
 │ • Encodes 8-bit bitmask: Fault_Byte = [F8 F7 F6 F5 F4 F3 F2 F1]              │
 └──────────────────────────────────────┬───────────────────────────────────────┘
                                        │
                                        ▼
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ STAGE 3: Serialization & Lightweight Frame Encoding                          │
 │ • Packs timestamp, 8 channels, and fault byte into bounded CSV string:       │
 │   "$TLM,<time_ms>,<S1>,<S2>,<S3>,<S4>,<S5>,<S6>,<S7>,<S8>,<Fault_Mask>\n"    │
 │ • Frame payload size: ~48 bytes (deterministic transmission budget)          │
 └──────────────────────────────────────┬───────────────────────────────────────┘
                                        │
                                        ▼
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ STAGE 4: Wireless Transport (Classic Bluetooth SPP / RFCOMM)                 │
 │ • Transmitted via 2.4 GHz FHSS at 115,200 baud equivalent                    │
 │ • Point-to-point link directly to ground station terminal without router     │
 └──────────────────────────────────────┬───────────────────────────────────────┘
                                        │
                                        ▼
 ┌──────────────────────────────────────────────────────────────────────────────┐
 │ STAGE 5: Ground Station Visualisation & Blackbox Logging                     │
 │ • Android Smartphone: "Serial Bluetooth Terminal" logs CSV to flash storage  │
 │ • Laptop / PC: Arduino IDE 2.0 Serial Plotter renders live multi-color trace │
 └──────────────────────────────────────────────────────────────────────────────┘
```

---

## 📦 3. Telemetry Frame Specification

To guarantee minimal radio payload and instantaneous parsing in standard terminal monitors, data is packaged into human-readable ASCII frames:

### Frame Structure

```text
$TLM,<TIMESTAMP_MS>,<S1>,<S2>,<S3>,<S4>,<S5>,<S6>,<S7>,<S8>,<FAULT_MASK>*<CHECKSUM>\n
```

### Sample Real-Time Data Stream

| Sample Transmitted String | Decoded Meaning |
| :--- | :--- |
| `$TLM,10420,45,110,480,950,510,120,50,40,0x00*4A` | **Normal Operation:** S4 centered on line. Fault mask = `0x00` (all healthy). |
| `$TLM,10440,40,105,475,000,505,115,48,38,0x08*2C` | **FAULT DETECTED:** S4 dropped to 0 while S3 & S5 are high! Fault mask = `0x08` (Bit 3 = 1 $\implies$ **Sensor 4 Faulty**). |
| `$TLM,10460,42,108,470,940,512,118,52,41,0x00*5F` | **Self-Recovery:** S4 bounced back. Diagnostic log flags S4 as intermittent. |

---

## 📱 4. Ground Station Setup & Field Visualization

No custom app development is required; standard diagnostic tools provide immediate out-of-the-box operation:

```text
                      [ ESP32 Robot Transmitting Over Air ]
                                       │
                      ┌────────────────┴────────────────┐
                      ▼                                 ▼
           [ Android Smartphone ]              [ Pit-Lane Laptop ]
        Serial Bluetooth Terminal App          Arduino Serial Plotter
        ┌───────────────────────────┐      ┌───────────────────────────┐
        │ 10440: S4 DROP -> FAULT!  │      │ 1000 ─┐  S4 (Red Line)    │
        │ Auto-logged to SD Card:   │      │       │     /\     /\     │
        │ /storage/telemetry.csv    │      │    0 ─┴────\  /───\  /────│
        │ Real-time timestamped     │      │         Spike down to 0   │
        └───────────────────────────┘      └───────────────────────────┘
```

### Option A: Mobile Phone Blackbox (Android)
- **Application:** *Serial Bluetooth Terminal* (by Kai Morich).
- **Operation:** Connects via Bluetooth RFCOMM. 
- **Logging:** Writes raw CSV frames with millisecond precision directly to internal storage (`telemetry_run1.csv`) for post-run analysis.

### Option B: Pit-Lane Laptop (Windows / Linux / macOS)
- **Application:** *Arduino IDE 2.0 Serial Plotter* or *PuTTY*.
- **Operation:** Pairs as standard Bluetooth serial link (`COMx` or `/dev/rfcomm0`).
- **Visual Graphing:** Comma-separated channel values automatically render as eight distinct color-coded curves. A faulty sensor instantly appears as a dramatic vertical drop down to zero, confirming the culprit pin in real time.

---

## 🏁 Diagnostic Verification Summary

| Verification Step | Target Parameter | Passing Criterion |
| :--- | :--- | :--- |
| **Sampling Latency** | ADC1 + Spatial Filter Loop | Execution completes in $< 3.5\text{ ms}$ on Core 0. |
| **Telemetry Cadence** | Transmission Interval | Deterministic $50\text{ Hz}$ ($20\text{ ms}$ update window). |
| **False-Positive Immunity**| Sharp $90^\circ$ Line Exits | Smooth neighbor transitions do **not** trigger fault bitmask. |
| **Fault Isolation** | Simulated Jumper Pin Disconnect | Fault bitmask flags target channel within $\le 1$ frame ($20\text{ ms}$). |

---

## 🔗 Related Documentation
- 📡 **Part 1:** [Wireless Protocol Selection & Trade-Offs](./protocol_selection.md)
- 🗂️ **Master Dashboard:** [Task 7 Master README](./README.md)