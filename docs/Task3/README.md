# 🎛️ Task 3: Closed-Loop PID Control System Architecture

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Target:** ESP32-WROOM-32 | Toshiba TB6612FNG | GA12-N20 500 RPM | Pololu QTR-8A / 5-CH Mock  
**Author:** Aman Kumar | **Weight:** 24 Points

---

## 🧭 System Pipeline Overview

```text
[ 5 IR Sensors ] ──► [ Centroid p ] ──► [ Error e(t) ] ──► [ PD Loop ] ──► [ Mixing ] ──► [ TB6612FNG ]
  (0 to 1000)        [-2.0, +2.0]        Setpoint=0.0       Kp=48, Kd=24     Vbase ± u    [-255, +255]
```

---

## 📑 Evaluation Index & Quick Navigation

Judges can inspect detailed derivations and proofs across the 4 modular reports below:

| Section | Focus / Key Result | Core Equation / Rule | Deep-Dive Link |
| :--- | :--- | :--- | :---: |
| **Part 1** | **P, I, D Roles**<br>• $P$: Spring stiffness (present)<br>• $I$: Bias elimination (past)<br>• $D$: Viscous damping (future) | $$u(t) = K_p e + K_i \int e \, dt + K_d \frac{de}{dt}$$ | [**`pid_terms.md`**](./pid_terms.md) |
| **Part 2** | **Configuration Choice**<br>• **PD Selected** ($K_i = 0$)<br>• Rejects P-Only (fishtailing)<br>• Rejects Full PID (90° turn windup) | $$\zeta \approx 1.0 \quad (K_i \equiv 0)$$<br>*Zero windup lag at corners* | [**`configuration_selection.md`**](./configuration_selection.md) |
| **Part 3** | **Sensor $\to$ Motor Pipeline**<br>• Weighted centroid derivation<br>• Differential steering mixing<br>• Bi-directional PWM clamping | $$p = \frac{\sum W_i S_i}{\sum S_i}, \quad W=[-2 \dots +2]$$<br>$$\text{PWM}_{L,R} = V_{\text{base}} \pm u(t)$$ | [**`sensor_to_motor_pipeline.md`**](./sensor_to_motor_pipeline.md) |
| **Part 4** | **Failure Mitigation**<br>• EMA low-pass derivative filter<br>• 200 Hz fixed loop timer<br>• Lost-line watchdog & active brake | $$D_{\text{filt}} = \alpha D + (1-\alpha) D_{\text{prev}}$$<br>*1.2s timeout safety stop* | [**`oscillation_and_lost_line.md`**](./oscillation_and_lost_line.md) |

---

## 💻 Firmware Implementation

The complete, non-blocking control firmware is decoupled into the project codebase:

* **Production Sketch:** [**`src/Task3/PID_Controller.ino`**](../../src/Task3/PID_Controller.ino)
* **Key Specs:** $200\text{ Hz}$ deterministic loop (`micros()`), EMA filtered derivative ($\alpha = 0.70$), signed $[-255, +255]$ clamping for zero-radius pivot turns, and TB6612FNG short-braking.

---

## 📓 Handwritten Derivation Index

* **Centroid & Error Math:** `image1.png`, `image8.png`
* **P/I/D & Windup Proofs:** `image2.png`, `image4.png`, `image5.png`, `image7.png`
* **Differential Drive & Clamping:** `image9.png`, `image10.png`
* **Filter EMA & Recovery FSM:** `image10.png`, `image11.png`

---

## 📚 Key References
1. [Pololu 3pi Robot PID Line Following Guide](https://www.pololu.com/docs/0J21/7.c)
2. [Brian Douglas — MATLAB Tech Talks: PID Control Series](https://www.youtube.com/playlist?list=PLn8PRpmsu08pQBgjxYFXSsODEF3Jqmm-y)
3. [Modern Robotics (Lynch & Park) — Ch. 13.2 Differential Drive](https://modernrobotics.northwestern.edu/)
4. [Åström & Murray — Feedback Systems: Integrator Windup](https://fbswiki.org/wiki/index.php/Feedback_Systems)