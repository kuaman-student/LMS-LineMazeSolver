# 🚨 Part 4 — Failure Modes: Oscillation & Lost-Line Recovery

Engineering strategies for mitigating high-speed lateral oscillation (hunting) and safely recovering during track discontinuities and lost-line events.

---

## 1. 🐍 Failure Mode A: High-Speed Oscillation (Hunting / Fishtailing)

### Diagnostic Profile
* **Symptom:** Continuous rapid lateral snaking along straight paths with high-frequency acoustic chatter from the gearmotors.
* **Root Causes:**
  1. **Excessive Proportional Stiffness ($K_p$):** Restorative torque overshoots the line before chassis angular momentum ($J \cdot \ddot{\theta}$) can be arrested.
  2. **Derivative Noise Amplification:** Differentiating discrete spatial noise from track bumps and vinyl edges creates large spikes in $\frac{\Delta e}{\Delta t}$.
  3. **Sampling Jitter ($\Delta t$ Variance):** Variable cycle durations from blocking routines destabilize the discrete-time derivative approximation.

```
Raw Sensor Noise ──► High-Frequency Spikes in de/dt ──► Actuator Jitter / Hunting
         │
         ▼ (Solution)
Digital Low-Pass Filter (EMA) ──► Smooth Viscous Damping (Zero Phase Jitter)
```

### Engineering Countermeasures
1. **Gain Recalibration:** Reduce $K_p$ by 20–30% while incrementally increasing $K_d$ until critical damping ($\zeta \approx 1.0$) is achieved.
2. **Deterministic Sampling Loop:** Lock execution frequency to $f_s = 200\text{ Hz}$ ($\Delta t = 5\text{ ms}$) via ESP32 `micros()` hardware timing.
3. **1st-Order Exponential Moving Average (EMA) Low-Pass Filter:**
   $$D_{\text{filt}}[k] = \alpha \cdot D_{\text{raw}}[k] + (1 - \alpha) \cdot D_{\text{filt}}[k - 1]$$
   With smoothing factor $\alpha = 0.70$, high-frequency measurement ripple is attenuated while retaining crisp transient steering response.

---

## 2. ❓ Failure Mode B: Lost-Line Condition ($\sum S_i < \text{Threshold}$)

When all 5 reflectance sensors detect the dark arena surface ($\sum S_i < 150$), the robot has either overshot an extreme turn or encountered an arena discontinuity.

```
                    [ Track Loss Detected: ∑ S_i < 150 ]
                                      │
                     ┌────────────────┴────────────────┐
                     ▼                                 ▼
             [ e_prev >= 0.0 ]                 [ e_prev < 0.0 ]
         (Line Slipped to Right)            (Line Slipped to Left)
                     │                                 │
                     ▼                                 ▼
           Command In-Place Pivot            Command In-Place Pivot
                 Clockwise                      Counter-Clockwise
             (L: +120, R: -120)                (L: -120, R: +120)
                     │                                 │
                     └────────────────┬────────────────┘
                                      │
                                      ▼
                      [ Watchdog Timeout Check: t > 1.2s? ]
                             │                     │
                          [ YES ]               [ NO ]
                             │                     │
                             ▼                     ▼
                     EMERGENCY STOP        Continue Searching
                   Active Motor Brake      (Resume PID on hit)
```

### Recovery Strategy
1. **Directional Memory Spin:** The controller preserves the sign of `previous_error`. If $e_{\text{prev}} \ge 0$, the line departed to the right $\to$ execute a clockwise pivot. If $e_{\text{prev}} < 0$, execute a counter-clockwise pivot.
2. **Failsafe Watchdog Timer:** If the line is not reacquired within **$1.2\text{ seconds}$**, both motor channels engage **TB6612FNG Short Brake** (`IN1 = HIGH`, `IN2 = HIGH`, `PWM = 0`) to prevent high-speed collisions with arena perimeter walls.

---

## 💻 Source Code Implementation

The complete C++ implementation featuring the 200 Hz deterministic timing loop, EMA derivative filter, differential mixing, and lost-line state machine has been decoupled into:

👉 **[`src/Task3/PID_Controller.ino`](../../src/Task3/PID_Controller.ino)**

Key features implemented in the sketch:
* **Non-blocking hardware timer execution** via `micros()`.
* **Sub-millimeter weighted centroid error calculation**.
* **Sign-preserving motor clamping** ($[-255, +255]$) for zero-radius turning.
* **Direction-aware timeout watchdog** with emergency short braking.

---

## 📓 Handwritten Notes

| Noise Filtering | Digital first order low pass filter eqn |
| :---: | :---: |
| ![Notes 10](../Handwritten%20Notes/Task3/image10.png) | ![Notes 11](../Handwritten%20Notes/Task3/image11.png) |

---

## 🤖 Credits & References
- 📚 **Noise & Filtering Reference:** [MATLAB Tech Talks — Why Derivative Needs a Filter](https://www.youtube.com/watch?v=7dUVdrs1e18)
- 📚 **Actuator Protection:** Toshiba TB6612FNG Inductive Suppression & Braking Logic
- 🛠️ **Author:** Aman Kumar
- 📝 **Format:** Structured for Task 3 deliverable review