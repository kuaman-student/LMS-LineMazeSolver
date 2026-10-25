# ⚖️ Part 2 — Controller Selection: Why PD over P and Full PID?

Architecture comparison and justification for selecting a **PD Controller ($K_p + K_d$) with $K_i = 0$** for the Techfest Meshmerize line-maze robot.

---

## 🎯 Architecture Comparison

```text
                      Control Architectures
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
     P-Only                 Full PID               PD (Selected)
❌ Fish-tailing         ❌ Severe Windup        ✅ Critically Damped
   at high speed           Lag at 90° turns        Fastest settling
```

| Criteria | P-Only | Full PID ($K_i > 0$) | PD ($K_i = 0$) [Selected] |
| :--- | :---: | :---: | :---: |
| **High-Speed Straight Line** | Continuous fish-tailing | Smooth | **Smooth & Critically Damped** |
| **90° Turn Performance** | Sweeps wide / sluggish | Slings robot off track | **Sharp, crisp zero-radius pivot** |
| **Integrator Windup Risk** | None | Extremely High | **Zero (Term is eliminated)** |
| **Response Latency** | Instant | Phase Lagged | **Predictive & Immediate** |
| **Verdict** | ❌ Rejected | ❌ Rejected | **✅ Selected for Competition** |

---

## 💡 Why $K_i = 0$ is the Winning Choice

### 1. No Constant Lateral Forces
Unlike a drone holding altitude against gravity or an autonomous boat fighting cross-currents, our line-maze robot operates on a **flat horizontal floor**. There is no persistent external lateral side-force pulling the robot off track, meaning a true continuous steady-state error does not exist.

### 2. The 90° Turn "Integrator Windup" Crash
In Techfest Meshmerize, all path intersections turn at sharp $90^\circ$ angles.

```text
Robot Enters 90° Turn ──► Large error persists (~100 ms) ──► Integrator sum explodes (Windup)
                                                                       │
Bot exits turn on center line (e = 0) ◄────────────────────────────────┘
             │
             ▼
Stale integrator accumulator commands aggressive turn ──► Robot thrown out of arena 💥
```

Setting **$K_i = 0$** eliminates memory lag completely, allowing the robot to transition out of a $90^\circ$ pivot turn and back onto a high-speed straight line with zero delay and zero overshoot.

---

## 📓 Handwritten Notes

| Integral Windup error | Motor Control | Configuration Justification |
| :---: | :---: | :---: |
| ![Notes 7](../Handwritten%20Notes/Task3/image7.png) | ![Notes 8](../Handwritten%20Notes/Task3/image8.png) | ![Notes 9](../Handwritten%20Notes/Task3/image9.png) |

---

## 🤖 Credits & References

- 📚 **Reference:** [Anti windup for PID control](https://www.youtube.com/watch?v=NVLXCwc8HzM&list=PLn8PRpmsu08pQBgjxYFXSsODEF3Jqmm-y&index=2)
- 🛠️ **Author:** Aman Kumar
- 📝 **Format:** Structured for Task 3 deliverable review