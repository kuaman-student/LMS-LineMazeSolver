# 📡 Part 1 — Wireless Protocol Selection & Trade-Off Analysis

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Subsystem:** Remote Telemetry & In-Motion Diagnostics  
**Author:** Aman Kumar 

---

## 📌 Executive Summary

To diagnose intermittent infrared sensor dropouts caused by arena floor bumps and jumper vibrations in real time—without halting the robot or touching hardware—we evaluated the three wireless protocols native to the **ESP32-WROOM-32**: **Wi-Fi (802.11 b/g/n)**, **Bluetooth Low Energy (BLE 4.2)**, and **Classic Bluetooth (BR/EDR with Serial Port Profile - SPP)**.

```text
                           ESP32 Wireless Protocols
                                      │
         ┌────────────────────────────┼────────────────────────────┐
         ▼                            ▼                            ▼
  [ Wi-Fi 802.11 b/g/n ]         [ BLE 4.2 GATT ]          [ Classic Bluetooth SPP ]
  ❌ 240mA RF current surges    ⚠️ GATT MTU packet slicing   ✅ Low-latency UART pipe
  ❌ Disables ADC2 hardware      ⚠️ Higher stack overhead   ✅ Zero router dependency
  ❌ Heavy connection latency   ✅ Low power (30–80mA)      ✅ Native Serial Plotter support
```

> 🏆 **Selection Verdict:** **Classic Bluetooth (SPP / RFCOMM)** is selected as the primary telemetry protocol. It behaves as a transparent, high-throughput wireless serial cable requiring zero network infrastructure, zero GATT packet chopping, and zero hardware interference with analog-to-digital conversions.

---

## ⚖️ Quantitative Trade-Off Matrix

| Evaluation Parameter | Wi-Fi (UDP / WebSockets) | Bluetooth Low Energy (BLE 4.2) | Classic Bluetooth (SPP / RFCOMM) | Winning Advantage |
| :--- | :--- | :--- | :--- | :--- |
| **End-to-End Latency** | $10 - 25\text{ ms}$ (Jitter prone in crowded halls) | $20 - 50\text{ ms}$ (Bound by Connection Interval) | **$12 - 18\text{ ms}$** (Consistent continuous stream) | **Classic BT:** Deterministic real-time frame rates. |
| **Peak RF Current Draw** | **$180 - 240\text{ mA}$** (Dangerous transient spikes) | **$30 - 80\text{ mA}$** (Pulsed duty cycle) | **$100 - 130\text{ mA}$** (Steady carrier wave) | **BLE:** Lowest power; **Classic BT:** Safe within battery budget. |
| **ESP32 Silicon Resource Conflict** | ⚠️ **Locks ADC2 Controller** (Disables GPIOs 0, 2, 4, 12–15, 25–27) | ✅ **No ADC Conflicts** (Permits simultaneous sampling) | ✅ **No ADC Conflicts** (Operates independently of SAR ADC) | **Classic BT & BLE:** Clean analog line acquisition. |
| **Network Infrastructure** | Requires external 2.4 GHz AP / Router or SoftAP mode | Peer-to-Peer (Direct Client-Server) | **Peer-to-Peer (Direct Point-to-Point)** | **Classic BT & BLE:** Works anywhere without Wi-Fi routers. |
| **Packet Formatting Overhead** | High (TCP/IP / WebSocket packet headers) | Moderate (GATT Service / Characteristic UUIDs) | **Zero (Raw ASCII Comma-Separated Byte Stream)** | **Classic BT:** Drop-in replacement for `Serial.printf()`. |
| **Host Tool Compatibility** | Requires web browser or socket listener | Requires custom BLE app (nRF Connect / Web Bluetooth) | **Universal:** Android *Serial Bluetooth Terminal*, PC *PuTTY*, Arduino Serial Plotter | **Classic BT:** Instant plotting and CSV logging. |
| **Engineering Decision** | ❌ **REJECTED** | 🥈 **VIABLE ALTERNATIVE** | 🏆 **APPROVED & SELECTED** | **Optimal balance of ease, speed, and safety.** |

---

## 🔬 In-Depth Engineering Justification

### 1. ⚡ Latency & Real-Time Responsiveness ($f_s \ge 50\text{ Hz}$)
* **The Diagnostic Constraint:** The robot cruises at $500\text{ RPM}$ ($\approx 1.2\text{ m/s}$). A mechanical bump over an arena vinyl seam lasts between $10 - 30\text{ ms}$. To catch a single-sensor dip during this transient window, telemetry must stream at $\ge 50\text{ Hz}$ ($20\text{ ms}$ update rate).
* **Why Classic BT SPP Excels:** Classic Bluetooth operates over an established RFCOMM transport layer that emulates an RS-232 serial port. It streams continuous 8-channel sensor frames at $115,200\text{ baud}$ without packet acknowledgment round-trip stalls, delivering updates in under $15\text{ ms}$.
* **Why BLE Stalls:** BLE throughput is governed by the negotiated **Connection Interval** ($7.5\text{ ms} - 4000\text{ ms}$) and slave latency. Standard consumer smartphones typically negotiate a $30 - 50\text{ ms}$ interval, which introduces lag and risks dropping transient bump spikes.

### 2. 🔋 Power Consumption & Battery Protection
* **The Brownout Threat:** As proven in Task 5 and Task 6, sudden current draws can sag the system supply rail.
* **Why Wi-Fi Was Rejected:** The ESP32's internal RF power amplifier draws transient bursts up to **$240\text{ mA}$** when broadcasting Wi-Fi beacons. In an arena with hundreds of spectator smartphones competing on the 2.4 GHz band, Wi-Fi channel retries cause prolonged radio transmit windows, heating the ESP32 and pushing peak current dangerously close to regulator dropout.
* **Why Classic BT Fits the Budget:** Classic Bluetooth draws a stable, predictable **$\approx 110\text{ mA}$**, well within the Orange 1500mAh 25C LiPo's 37.5A continuous discharge capability ($16.5\times$ safety margin).

### 3. 🚫 Hardware Conflict: The ADC2 Silicon Lockout
The most decisive hardware reason for rejecting Wi-Fi is the silicon architecture of the ESP32:
* The ESP32 contains two Successive Approximation Register (SAR) ADCs. **ADC2 is multiplexed with the Wi-Fi baseband.**
* The moment `WiFi.begin()` or `WiFi.softAP()` is invoked, the ESP-IDF Wi-Fi driver takes exclusive ownership of ADC2. Any attempt to read analog sensors connected to ADC2 pins returns `ESP_ERR_TIMEOUT` or garbage values ($0$ or $4095$).
* Classic Bluetooth and BLE run over the Bluetooth baseband, which **does not interfere with the ADC system**, leaving analog peripherals unaffected.

### 4. 🛠️ Implementation Simplicity & Field Usability
* **Classic Bluetooth SPP:** Emulates a standard serial port via `BluetoothSerial.h`. Firmware implementation requires zero protocol stack code:
  ```cpp
  // Native replacement for hardware UART
  SerialBT.printf("%lu,%d,%d,%d,%d,%d,%d,%d,%d\n", 
                  millis(), s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7]);
  ```
* **Host Receiver:** Connects immediately to:
  * **Android Devices:** Using the free *Serial Bluetooth Terminal* app to view live text and log directly to `.csv` in phone storage.
  * **Laptops (PC / Mac / Linux):** Pairs as a virtual Bluetooth COM port (`COMx` or `/dev/rfcomm0`), enabling plug-and-play multi-colored line rendering in the **Arduino IDE 2.0 Serial Plotter**.

---

## 🏁 Final Protocol Verdict

| Protocol | Final Status | Primary Engineering Rationale |
| :--- | :---: | :--- |
| **Wi-Fi (802.11 b/g/n)** | ❌ **Rejected** | Locks ADC2 pins, draws $240\text{ mA}$ RF spikes, and suffers channel interference in crowded halls. |
| **Bluetooth Low Energy (BLE)** | 🥈 **Alternative** | Low power, but restricted by GATT 23-byte MTU slicing and connection interval latency. |
| **Classic Bluetooth (SPP)** | 🏆 **Selected** | **Zero ADC conflicts, deterministic $<18\text{ ms}$ stream, zero router dependence, and instant multi-channel plotting.** |

---

## 🔗 Related Documentation
- 📊 **Next Section:** [Part 2: End-to-End System Architecture & Fault-Detection Data Flow](./system_architecture.md)
- 🗂️ **Master Dashboard:** [Task 7 Master README](./README.md)