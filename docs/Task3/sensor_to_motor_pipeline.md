# 🧮 Part 3 — Sensor-to-Motor Control Pipeline

How raw analog IR reflectance readings transform into differential wheel commands and clamped PWM.

---

## 🔄 End-to-End Signal Flow

```text
[5 Analog IR Readings: S1..S5]
              │
              ▼
[Step 1: Weighted Centroid Estimation] ──► p ∈ [-2.0, +2.0]
              │
              ▼
[Step 2: Error Formulation] ───────────► e(t) = p - 0.0
              │
              ▼
[Step 3: PD Control Calculation] ──────► u(t) = Kp*e + Kd*(de/dt)
              │
              ▼
[Step 4: Differential Speed Mixing] ────► Left = Vbase + u, Right = Vbase - u
              │
              ▼
[Step 5: Bi-Directional Clamping] ─────► [-255, +255] PWM to TB6612FNG
```

---

## 🔢 Step-by-Step Mathematical Pipeline

### Step 1: Weighted Centroid Line Estimation
Using 5 calibrated reflectance sensors positioned with linear coordinate weights $W = [-2, -1, 0, 1, 2]$:

$$p = \frac{\sum_{i=1}^5 W_i \cdot S_i}{\sum_{i=1}^5 S_i} = \frac{(-2 \cdot S_1) + (-1 \cdot S_2) + (0 \cdot S_3) + (1 \cdot S_4) + (2 \cdot S_5)}{S_1 + S_2 + S_3 + S_4 + S_5}$$

* Output $p$ is a normalized continuous position estimate bounded strictly between $[-2.0, +2.0]$.
* **Zero-Denominator Guard:** If $\sum_{i=1}^5 S_i < \varepsilon$ (robot has completely lost the line or lifted off ground), the firmware holds the last known valid sign of $p$ to prevent division-by-zero crashes.

---

### Step 2: Error Formulation
Target reference setpoint is dead-center ($x_{\text{target}} = 0.0$):

$$e(t) = p - x_{\text{target}} = p$$

* **$e(t) > 0$:** Track line shifted to the **Right** $\to$ Command robot to steer Right.
* **$e(t) < 0$:** Track line shifted to the **Left** $\to$ Command robot to steer Left.
* **$e(t) = 0$:** Robot is perfectly centered $\to$ Drive straight forward.

---

### Step 3: Steering Effort Calculation ($u(t)$)
The PD controller computes steering correction $u(t)$ based on lateral displacement and rate of line shift:

$$u(t) = K_p \cdot e(t) + K_d \cdot \left(\frac{e(t) - e_{\text{prev}}}{\Delta t}\right)$$

* $K_p$: Proportional gain for immediate corrective steering.
* $K_d$: Derivative gain for straight-line stabilization and critical damping against fish-tailing oscillations.
* Integral term is set to zero ($K_i = 0$) to eliminate phase lag and avoid integral windup during sharp turns.

---

### Step 4: Differential Motor Mixing
Given a nominal forward baseline velocity $V_{\text{base}} = 180$:

$$\text{PWM}_{\text{Left}} = V_{\text{base}} + u(t)$$

$$\text{PWM}_{\text{Right}} = V_{\text{base}} - u(t)$$

* **Line shifts Right ($e > 0 \implies u > 0$):** Left wheel accelerates, right wheel decelerates $\to$ Robot banks right.
* **Line shifts Left ($e < 0 \implies u < 0$):** Left wheel decelerates, right wheel accelerates $\to$ Robot banks left.

---

### Step 5: Actuator Saturation & Bi-Directional Clamping
Hardware 8-bit timer registers are clamped to the range $[-255, +255]$:

$$\text{Clamped\_PWM} = \max(-255, \min(\text{Calculated\_PWM}, 255))$$

> 💡 **Why allow signed negative commands?**  
> When lateral steering effort exceeds forward baseline speed ($\lvert u \rvert > V_{\text{base}}$), a signed negative PWM commands the TB6612FNG H-bridge to reverse that specific wheel. Counter-rotating wheels generate a **zero-radius pivot turn**, essential for sharp $90^\circ$ maze junctions.

---

## 📊 Summary Mapping Table

| Line Position | Centroid ($p$) | Error ($e$) | Correction ($u$) | Left Motor PWM | Right Motor PWM | Chassis Motion |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Dead Center** | $0.0$ | $0.0$ | $0$ | $V_{\text{base}}$ | $V_{\text{base}}$ | Straight line tracking |
| **Mild Right** | $+0.8$ | $+0.8$ | $> 0$ | $V_{\text{base}} + u$ | $V_{\text{base}} - u$ | Gentle right bank |
| **Mild Left** | $-0.8$ | $-0.8$ | $< 0$ | $V_{\text{base}} - \lvert u \rvert$ | $V_{\text{base}} + \lvert u \rvert$ | Gentle left bank |
| **90° Right Corner** | $+2.0$ | $+2.0$ | $\gg 0$ | $+255$ | $-100$ to $-255$ | Zero-radius right pivot |
| **90° Left Corner** | $-2.0$ | $-2.0$ | $\ll 0$ | $-100$ to $-255$ | $+255$ | Zero-radius left pivot |

---

## 📓 Handwritten Notes

| Controlling Motor with Error from Sensor |
| :---: |
| ![Notes 8](../Handwritten%20Notes/Task3/image8.png) |

---

## 🤖 Credits & References

* 📚 **Reference:** [Pololu 3pi PID Line Following Guide](https://www.pololu.com/docs/0J21/7.c)
* 📚 **Textbook:** [Modern Robotics (Lynch & Park) — Chapter 13.2](https://modernrobotics.northwestern.edu/)
* 🛠️ **Author:** Aman Kumar (IIT Mandi)
* 📝 **Format:** Structured for Task 3 Deliverable Review