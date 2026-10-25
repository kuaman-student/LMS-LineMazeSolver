# 🎯 8-Channel IR Line-Tracking: Mathematical PID Derivation & Motor Dynamics

How to mathematically calculate $K_p$ and $K_d$ from first principles, and how differential mixing controls wheel speeds across all maze intersections.

---

## 📐 1. Sensor Geometry & Centroid Formulation

For an **8-Channel Reflectance Sensor Array** (QTR-8RC / QTR-8A), sensors $S_1$ to $S_8$ are spaced symmetrically with linear coordinates:

$$W = [-3.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +3.5]$$

```text
[ S1 ]   [ S2 ]   [ S3 ]   [ S4 ] │ [ S5 ]   [ S6 ]   [ S7 ]   [ S8 ]
 -3.5     -2.5     -1.5     -0.5  │  +0.5    +1.5     +2.5     +3.5
                                  ▲
                         Center Line (SetPoint = 0.0)
```

The normalized position $p$ and track error $e(t)$ are calculated as:

$$p = \frac{\sum_{i=1}^8 W_i \cdot S_i}{\sum_{i=1}^8 S_i}$$

$$e(t) = p - 0.0 = p \implies e(t) \in [-3.5, +3.5]$$

* **Zero-Denominator Guard:** If $\sum_{i=1}^8 S_i < \varepsilon$ (robot completely off the line or lifted off ground), the firmware holds the last known valid sign of $p$ to prevent division-by-zero crashes.

---

## 🧮 2. Mathematical Derivation of Gains ($K_p$ and $K_d$)

### A. System Constraints & Operating Conditions
* **Base Cruising Speed:** $V_{\text{base}} = 180$ PWM units (8-bit timer scale: $0 - 255$).
* **Available Headroom:** $\Delta \text{PWM}_{\text{max}} = 255 - 180 = 75$ units.
* **Sampling Loop Interval:** $\Delta t = 5\text{ ms}$ ($f_{\text{loop}} = 200\text{ Hz}$).
* **Chassis Time Constant:** $\tau_m \approx 0.12\text{ s}$ (GA12-N20 geared motor + wheel inertia).

---

### B. Proportional Gain ($K_p$) Derivation
$K_p$ must scale the available steering headroom ($\Delta \text{PWM} = 75$) to match the linear boundary displacement ($e = 2.5$, corresponding to sensor $S_7$ on track edge):

$$u_P = K_p \cdot e$$

$$75 = K_p \cdot 2.5 \implies \mathbf{K_p = \frac{75}{2.5} = 30.0}$$

* If $e = 1.0$ (mild shift): $u = 30 \times 1.0 = 30$ PWM adjustment.
* If $e = 2.5$ (track edge): $u = 30 \times 2.5 = 75 \implies \text{Outer wheel hits exactly } 180 + 75 = 255$.

---

### C. Derivative Gain ($K_d$) Derivation (Critical Damping)
The lateral motion obeys a standard second-order continuous system:

$$\ddot{e}(t) + 2\zeta \omega_n \dot{e}(t) + \omega_n^2 e(t) = 0$$

To completely eliminate straight-line fish-tailing oscillations, the system must be **critically damped ($\zeta = 1.0$)**:

$$K_d \approx 2 \zeta \sqrt{K_p} \cdot \tau_m$$

$$K_d \approx 2 \times 1.0 \times \sqrt{30} \times 0.12 = 2 \times 5.477 \times 0.12 \approx \mathbf{1.31}$$

#### Discrete Firmware Implementation:
At a $200\text{ Hz}$ loop rate ($\Delta t = 0.005\text{ s}$), absorbing the division by $\Delta t$ into the firmware gain avoids floating-point operations:

$$u_D = K_{d,\text{discrete}} \cdot (e[k] - e[k-1])$$

$$K_{d,\text{discrete}} = \frac{K_d}{\Delta t} = \frac{1.31}{0.005} \approx \mathbf{262}$$

---

### D. Why $K_i = 0$?
* $K_i > 0$ accumulates past error ($\int e\,dt$), causing **Integral Windup** and sluggish exit lag during sharp 90° transitions. Flat surface tracking does not have steady-state gravitational loads, so $K_i$ is strictly $0.0$.

---

## ⚙️ 3. Differential Mixing & Motor Speed Response

Given $V_{\text{base}} = 180$:

$$\text{PWM}_{\text{Left}} = \max(-255, \min(V_{\text{base}} + u(t), 255))$$

$$\text{PWM}_{\text{Right}} = \max(-255, \min(V_{\text{base}} - u(t), 255))$$

* When $u(t) > 0$ (line shifted right): Left motor accelerates, Right motor decelerates $\to$ Right turn.
* When $u(t) < 0$ (line shifted left): Left motor decelerates, Right motor accelerates $\to$ Left turn.

---

## 🚦 4. All Maze Cases: Sensor Pattern to Motor Actions

| Track Scenario | Active Sensors ($S_1 \dots S_8$) | Centroid ($p$) | Steering Effort ($u$) | Left Motor PWM | Right Motor PWM | Chassis Motion / Behavior |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Dead Center (Straight)** | `0 0 0 1 1 0 0 0` | $0.0$ | $0$ | **$180$** | **$180$** | Pure forward cruising at balanced RPM |
| **Mild Right Drift** | `0 0 0 0 1 1 0 0` | $+1.0$ | $+30$ | **$210$** | **$150$** | Gentle bank to the right |
| **Severe Right Drift** | `0 0 0 0 0 0 1 1` | $+2.5$ | $+75$ | **$255$** | **$105$** | Hard bank; outer motor at max saturation |
| **Sharp 90° Right Corner** | `0 0 0 0 0 0 0 1` | $+3.5$ | $+105$ | **$255$** | **$+75 \to -50$** | In-place zero-radius pivot (Active Brake/Reverse) |
| **Sharp 90° Left Corner** | `1 0 0 0 0 0 0 0` | $-3.5$ | $-105$ | **$+75 \to -50$** | **$255$** | In-place zero-radius pivot (Active Brake/Reverse) |
| **T-Intersection / Cross** | `1 1 1 1 1 1 1 1` | $\approx 0.0$ | Bypass PD | **$0 \to \text{Turn}$** | **$0 \to \text{Turn}$** | FSM overrides PD: halts forward motion to execute Left-Hand rule |
| **Dead-End (180° Turn)** | `0 0 0 0 0 0 0 0` | Lost ($\sum S_i < \varepsilon$) | FSM U-Turn | **$-180$** | **$+180$** | Counter-rotation spin until $S_4/S_5$ re-detects the line |
| **Maze Finish / End Point** | Grid Marker / Stop | Trigger | FSM Halt | **`BRAKE (0)`** | **`BRAKE (0)`** | Low-side MOSFET short active-brake; dynamic halt in $< 5\text{ cm}$ |

---

## ⚡ 5. Quick Tuning Reference

```text
Problem: Continuous left-right wobble on straight lines
Cause:   Under-damped system (Kd too low) OR over-reactive P (Kp too high).
Fix:     Increase Kd by +20 or reduce Kp by -5.

Problem: Robot cuts outside on sharp turns / sluggish response
Cause:   Insufficient proportional authority.
Fix:     Increase Kp by +5 until sharp turns complete without clipping.

Problem: Robot jerks violently right after exiting a turn
Cause:   Over-damped derivative kick.
Fix:     Reduce Kd by -15.
```