# ⚡ Techfest Meshmerize — Autonomous Line-Maze Solver (LMS)

Production-grade firmware, control physics, hardware architecture, and graph-traversal documentation for an autonomous Line-Maze Solver built around the **ESP32-WROOM-32**.

The whole design is engineered strictly within a **₹10,000 budget**, with delivery to **IIT Mandi (PIN 175005)**.

---

## 📂 Repository Directory Layout

```text
├── BOM/
│   ├── BOM.md                          # Itemized ₹10k budget table with vendor links
│   └── Task5/
│       ├── power_budget.md             # Peak (2.27 A) & idle current calculations
│       └── chemistry_justification.md  # 2S LiPo (25C) vs 18650 Li-ion comparison
├── docs/
│   ├── Task2/datasheet_literacy.md     # TB6612FNG electrical limits & thermal metrics
│   ├── Task3/
│   │   ├── sensor_to_motor_pipeline.md # Centroid derivation & PWM allocation
│   │   └── pid_math_and_cases.md       # Damped PD formulation & lost-line recovery
│   ├── Task6/wiring_and_schematic.md   # Star-ground topology & noise mitigation
│   ├── Task7/system_architecture.md    # Spatial bump rejection & BT telemetry frame
│   ├── Task8/
│   │   ├── algorithm_selection.md      # LSRB vs DFS in loop-free tree mazes
│   │   └── graph_modelling_and_example.md # Node/edge mapping & path reduction
│   └── Task9/pid_tuning_and_validation.md # Deterministic 200 Hz scheduling & test cases
└── src/
    ├── task1_basic_coding/             # Centroid function & motor clamping tests
    └── task9_pid_controller.ino        # Full ESP32 firmware with simulation test harness
```

**How to navigate:** `BOM/` holds the budget and power analysis, `docs/` holds the theory and derivations for each task, and `src/` holds the actual code. Use the Task Compliance Matrix at the bottom to jump straight from a task number to its deliverable.

---

## ⚙️ Hardware Specifications & Budget Rationale

| Subsystem | Component | Specifications & Engineering Rationale |
| --- | --- | --- |
| **Compute Engine** | ESP32-WROOM-32 | Dual-core 240 MHz; the PID control loop and wireless diagnostics run on separate cores, so Bluetooth traffic never delays steering. |
| **Sensor Array** | 5-Ch Analog Array | Wired only to ADC1 pins (`GPIO 34, 35, 32, 33, 25`), because ADC2 becomes unavailable while Wi-Fi/Bluetooth is active. |
| **Motor Driver** | L298N (Appendix A) / TB6612FNG | Hardware LEDC PWM at 5 kHz, low voltage drop, and active short-braking (with the TB6612FNG). |
| **Actuation** | 2x N20 6V 500 RPM | Micro metal gearmotors with high stall torque; combined peak draw of 2.27 A. |
| **Power Supply** | 7.4V 2S 1500mAh 25C LiPo | Only 15 mΩ internal resistance, 16.5× current headroom (37.5 A max), and negligible voltage sag (0.034 V). |
| **Grounding** | Star-Ground Point | Keeps the high-current inductive return separate from logic ground, preventing ESP32 brownouts. |

**Power sanity check:**

- Peak draw is 2.27 A and the battery's maximum is 25C × 1.5 Ah = 37.5 A, so headroom is 37.5 / 2.27 ≈ **16.5×**.
- Voltage sag is I × R = 2.27 A × 15 mΩ ≈ **0.034 V**, which is practically negligible.

---

## 🔬 Core Engineering Highlights

### 1. Critically Damped PD Steering (ζ = 1.0)

The robot estimates where the line is from the sensor readings, then a PD controller turns that position error into left/right motor speed corrections.

- **Weighted Centroid:** The line position is computed as

  $$p = \frac{\sum w_i S_i}{\sum S_i}, \quad w = [-2, -1, 0, 1, 2]$$

  A small ε is added to the denominator to guard against division by zero when no sensor sees the line.
- **Integral Elimination (Kᵢ = 0):** An integral term causes windup and phase lag during sharp 90° turns, so it is removed entirely.
- **PD Values:** Kₚ = 35.0 and K_d = 1.45, running on a strictly deterministic **200 Hz (5 ms)** non-blocking timer loop. The fixed sample time keeps the derivative term stable.

### 2. Spatial Outlier Rejection & Live Telemetry

- **Spatial Invariant:** A 19 mm line always illuminates 2–3 adjacent sensors. If one sensor suddenly drops to 0 while its neighbors stay high, it is a loose jumper pin, not the line. The algorithm flags the fault and does not steer off-track:

  $$F_i = 1 \iff \left( |S_i[k] - S_i[k-1]| > 400 \right) \land \left( \frac{S_{i-1} + S_{i+1}}{2} > 300 \right)$$

- **Telemetry:** Data is streamed as ASCII frames over Classic Bluetooth SPP / RFCOMM at 115,200 baud for real-time pit-lane analysis:

  ```text
  $TLM,<time>,<S1..S5>,<pos>,<err>,<L>,<R>\n
  ```

### 3. Maze Graph Reduction (LSRB vs Dijkstra)

- **Topological Invariant:** A simply connected line-maze is a cycle-free tree, so |E| = |V| − 1.
- **Equivalence:** The maze is explored with the Left-Hand Rule, then the recorded path string is reduced:

  | Pattern | Reduces to |
  | --- | --- |
  | `LBL` | `S` |
  | `LBS` | `R` |
  | `RBL` | `B` |

  After removing the dead-end detours in a tree, the remaining path equals the optimal path found by offline Dijkstra:

  $$\text{Path}_{\text{LSRB, Reduced}} \equiv \text{Path}_{\text{Dijkstra, Optimal}}$$

  The benefit: no floating-point coordinates are needed, and there is no risk of odometer drift from wheel slip.

---

## 🚀 How to Build & Run

### A. Verification Mode (No Hardware Needed)

1. Open `src/task9_pid_controller.ino` in the Arduino IDE or VS Code PlatformIO.
2. Keep `#define SIMULATION_MODE` **uncommented**.
3. Select the **ESP32 Dev Module** board and flash over USB.
4. Open the Serial Monitor at **115200 baud**. The automated test runner evaluates these vectors: Centered, Left-Drift, Right-Drift, 90° Turn, and Lost-Line recovery.

### B. Hardware Deployment (Competition Arena)

1. Comment out `#define SIMULATION_MODE` in `task9_pid_controller.ino`.
2. Upload the sketch to the ESP32.
3. Place the robot over the line on the arena floor. After boot, sweep it across the line for 3 seconds to auto-calibrate the baseline reflectance.

---

## 📋 Task Compliance Matrix

| Task | Topic | Points | Key Deliverables |
| --- | --- | --- | --- |
| **Task 1** | Basic Coding | 15 | `src/task1_basic_coding/` (Centroid math & motor clamping) |
| **Task 2** | Datasheet Literacy | 18 | `docs/Task2/datasheet_literacy.md` (Electrical ratings & SOA) |
| **Task 3** | PID Control | 24 | `docs/Task3/pid_math_and_cases.md` (PD tuning & recovery) |
| **Task 4** | BOM Preparation | 27 | `BOM/BOM.md` (₹10,000 budget & power-block diagram) |
| **Task 5** | Power & Battery | 30 | `BOM/Task5/` (Current budget & LiPo vs Li-ion analysis) |
| **Task 6** | Hardware & Wiring | 36 | `docs/Task6/wiring_and_schematic.md` (Star grounding & debug) |
| **Task 7** | Wireless Telemetry | 39 | `docs/Task7/system_architecture.md` (Spatial bump rejection) |
| **Task 8** | Maze Algorithms | 45 | `docs/Task8/` (Graph mapping, LSRB vs Dijkstra, worked trace) |
| **Task 9** | Production System | 66 | `src/task9_pid_controller.ino` & `docs/Task9/` (Firmware & tests) |
| **Total** |  | **300** | **Fully Documented & Verified** |