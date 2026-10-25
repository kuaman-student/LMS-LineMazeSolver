# 🎛️ Part 1 — Roles of P, I, and D Terms

How each individual control term acts on our differential-drive maze robot.

---

## 📌 TL;DR Summary

$$u(t) = \underbrace{K_p \cdot e(t)}_{\text{React to Present}} + \underbrace{K_i \int e(t)\,dt}_{\text{Accumulate Past}} + \underbrace{K_d \frac{de(t)}{dt}}_{\text{Predict Future}}$$

```text
               [ Lateral Error e(t) ]
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
   [ P-Term ]       [ I-Term ]       [ D-Term ]
    Present           Past            Future
    Reaction       Accumulate        Dampening
        │                │                │
        └───────────┬────┴───────────┬────┘
                    ▼                ▼
               [ Motor Correction u(t) ]
```

---

## 🔍 Detailed Term Breakdown

### 1. 🔹 Proportional Term ($$P = K_p e(t)$$) — *"Present Reaction"*
* **Analogy:** Centering spring ($F = -kx$).
* **Robot Behavior:** The further the robot shifts from the center line, the harder $P$ commands the wheels to steer back.
* **Limitation:** $P$ cannot counteract rotational momentum ($J \cdot \omega$). At 500 RPM, when the robot reaches the center line ($e = 0$), momentum carries it right past the line, causing continuous **fish-tailing / hunting** (side-to-side oscillation).

### 2. 🔹 Integral Term ($I = K_i \int e(t)\,dt$) — *"Past Accumulation"*
* **Analogy:** A persistent pusher that gradually applies more force if a small offset refuses to clear.
* **Robot Behavior:** Eliminates small steady-state errors (such as uneven wheel wear or minor gearbox friction imbalances).
* **Limitation:** Introduces phase lag. During a sharp 90° maze turn, error stays high for ~100 ms. The accumulator explodes (**Integrator Windup**), driving the robot off the track right after exiting the turn.

### 3. 🔹 Derivative Term ($D = K_d \frac{de(t)}{dt}$) — *"Electronic Shock Absorber"*
* **Analogy:** Hydraulic dashpot / fluid damper ($F_d = -c \cdot v$).
* **Robot Behavior:** Measures how fast the robot approaches the line. As error rapidly drops toward zero ($\frac{de}{dt} < 0$), the negative term applies **counter-braking torque**, preventing overshoot and stabilizing the chassis.
* **Limitation:** Directly differentiates noise from floor seams and sensor ripple, requiring a deterministic loop rate ($\Delta t$) and digital filtering.

---

## 📊 Summary Table

| Term | Domain | Primary Function | Failure Mode if Gain is Too High |
| :---: | :---: | :--- | :--- |
| **$P$** | **Present** | Immediate restorative steering force | High-speed fish-tailing / undamped wobble |
| **$I$** | **Past** | Erases steady-state lateral error ($e_{ss} \to 0$) | Severe integrator windup & post-turn crashes |
| **$D$** | **Future** | Damps angular velocity; stops overshoot | High-frequency jitter & motor chatter |

---

## 📓 Handwritten Notes

| Understanding PID basics | P controller eqn proof1 | P controller eqn proof2 |
| :---: | :---: | :---: |
| ![Notes 1](../Handwritten%20Notes/Task3/image1.png) | ![Notes 2](../Handwritten%20Notes/Task3/image2.png) | ![Notes 3](../Handwritten%20Notes/Task3/image3.png) |

| Steady-State Error & solution | I proof | I proof 2 and D controller |
| :---: | :---: | :---: |
| ![Notes 4](../Handwritten%20Notes/Task3/image4.png) | ![Notes 5](../Handwritten%20Notes/Task3/image5.png) | ![Notes 6](../Handwritten%20Notes/Task3/image6.png) |

---

## 🤖 Credits & References
- 📚 **Video Lecture:** [Brian Douglas — What Is PID Control? (MATLAB)](https://www.youtube.com/playlist?list=PLn8PRpmsu08pQBgjxYFXSsODEF3Jqmm-y)
- 🛠️ **Author:** Aman Kumar
- 📝 **Format:** Structured for Task 3 deliverable review