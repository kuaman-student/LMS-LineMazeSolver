# ⚙️ Optional Extension 3: Encoder-Based Closed-Loop Speed Control

**Techfest Meshmerize — Autonomous Line-Maze Solver**
**Subsystem:** High-Speed Odometry Feedback, Cascaded Dual-Loop PID & Velocity Regulation
**Target Microcontroller:** ESP32-WROOM-32 (hardware GPIO interrupts + LEDC PWM)

---

## 📌 1. The Engineering Need: Open-Loop Drift vs. Closed-Loop Invariance

In the Task 9 baseline, the motors run under **open-loop voltage control** (for example `PWM = 180`). In real competition runs, assigning PWM directly runs into three disturbances:

```text
A. OPEN-LOOP PWM FAILURE (variable real-world speed)
   7.4V LiPo battery sag (8.4V fully charged ──► 7.2V depleted) ──┐
   Uneven arena tile surface friction                          ──┼──► Variable wheel RPM!
   Motor manufacturing variance (left motor ≠ right motor)     ──┘    (steering drifts off-line)

B. CLOSED-LOOP VELOCITY REGULATION (disturbance rejection)
   Target Velocity ──►┌─────────────┐──► PWM Duty ──►┌─────────────┐──► Real Wheel RPM
   (v = 0.85 m/s)     │ Encoder PID │                │ Motor Drive │    (held constant
                      └──────▲──────┘                └──────┬──────┘     regardless of battery V)
                             │       Actual Ticks/dt        │
                             └──────[ Wheel Encoder ]◄──────┘
```

| Operational Condition | Open-Loop PWM Control (Task 9) | Closed-Loop Encoder Control (Extension 3) |
| --- | --- | --- |
| **Battery depletion (8.4 V → 7.2 V)** | Motor RPM drops by about 15%; turns become sluggish | PID raises PWM dynamically to hold RPM constant |
| **Floor friction variation** | Robot pulls toward the high-friction side, causing yaw error | Motor controller automatically supplies compensating drive |
| **Unit of control** | Raw 8-bit counts (0–255) with no physical meaning | Real SI units: linear velocity v (m/s) and yaw rate ω (rad/s) |

---

## 🔄 2. Cascaded Dual-Loop Architecture

The control engine is split into two nested loops that run at different rates:

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │   OUTER LOOP: 100 Hz Position Loop (Core 1)                            │
 │   • Reads 5-channel reflectance sensor centroid                        │
 │   • Error e = Centroid - 0.0                                           │
 │   • Computes target angular yaw velocity: ω_target (rad/s)             │
 └───────────────────────────────────┬────────────────────────────────────┘
                                     │
                     Differential kinematic model:
                     v_L_target = v_cruise - (ω_target × L / 2)
                     v_R_target = v_cruise + (ω_target × L / 2)
                                     │
                                     ▼
 ┌────────────────────────────────────────────────────────────────────────┐
 │   INNER LOOP: 200 Hz Velocity Loop (Core 0 / hardware timer)           │
 │   • Reads incremental ticks from the encoder ISRs                      │
 │   • Error_L = v_L_target - v_L_measured                                │
 │   • Error_R = v_R_target - v_R_measured                                │
 │   • Updates PWM duty cycles via independent left/right PID engines     │
 └────────────────────────────────────────────────────────────────────────┘
```

The outer loop decides **how fast each wheel should turn** to stay on the line. The inner loop makes each wheel actually **reach that speed**, whatever the battery voltage or floor friction.

---

## 🔬 3. Mathematical Kinematics & Discrete Filtering

### A. Ticks to Angular & Linear Velocity

For an N20 motor with an optical encoder disk that gives PPR pulses per revolution, behind a gearbox of ratio N:

```text
RPM = ( ΔTicks / (PPR × N × Δt) ) × 60

Linear velocity v = RPM × 2π × R_wheel / 60
                  = ΔTicks × 2π × R_wheel / (PPR × N × Δt)
```

Values used in this design:

- Wheel radius R_wheel = 16 mm = 0.016 m
- Gearbox ratio N = 30:1
- Encoder disk resolution PPR = 12 pulses per motor revolution (total 360 counts per wheel rotation)

### B. Unicycle Model to Wheel Velocities

For a chassis wheelbase L = 95 mm = 0.095 m and a forward cruise speed v_cruise:

```text
v_left  = v_cruise - ( ω × L / 2 )
v_right = v_cruise + ( ω × L / 2 )
```

A positive ω turns the robot left, so the right wheel runs faster than the left.

---

## 💻 4. ESP32 Firmware Implementation Snippet

```cpp
#include <Arduino.h>

// Encoder hardware interrupt pins
#define PIN_ENC_L_A 18
#define PIN_ENC_R_A 19

// Motor parameters
const float WHEEL_CIRCUMFERENCE = 0.10053f; // 2 * pi * 0.016 m
const int   TICKS_PER_REV       = 360;      // 12 PPR * 30:1 gear ratio

// Volatile counters for the Interrupt Service Routines
volatile long enc_ticks_left  = 0;
volatile long enc_ticks_right = 0;

// High-speed ISRs (triggered on rising edge)
void IRAM_ATTR isr_encoder_left()  { enc_ticks_left++; }
void IRAM_ATTR isr_encoder_right() { enc_ticks_right++; }

// Inner velocity PID gains
const float ENC_KP = 180.0f;
const float ENC_KI = 25.0f;
const float ENC_KD = 2.10f;

float err_integral_L = 0, err_integral_R = 0;
float last_err_L = 0, last_err_R = 0;

void setup_encoders() {
    pinMode(PIN_ENC_L_A, INPUT_PULLUP);
    pinMode(PIN_ENC_R_A, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_L_A), isr_encoder_left, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_R_A), isr_encoder_right, RISING);
}

// 200 Hz inner velocity loop (called every 5 ms)
void update_velocity_pid(float target_v_L, float target_v_R, float dt, int &pwm_L, int &pwm_R) {
    // Atomic copy of the tick registers
    noInterrupts();
    long ticks_L = enc_ticks_left;  enc_ticks_left = 0;
    long ticks_R = enc_ticks_right; enc_ticks_right = 0;
    interrupts();

    // Real linear speed (m/s)
    float measured_v_L = ((float)ticks_L / TICKS_PER_REV) * WHEEL_CIRCUMFERENCE / dt;
    float measured_v_R = ((float)ticks_R / TICKS_PER_REV) * WHEEL_CIRCUMFERENCE / dt;

    // Velocity errors
    float err_L = target_v_L - measured_v_L;
    float err_R = target_v_R - measured_v_R;

    // Left motor PID
    err_integral_L = constrain(err_integral_L + (err_L * dt), -0.5f, 0.5f); // windup clamp
    float deriv_L  = (err_L - last_err_L) / dt;
    float out_L    = (ENC_KP * err_L) + (ENC_KI * err_integral_L) + (ENC_KD * deriv_L);
    last_err_L     = err_L;

    // Right motor PID
    err_integral_R = constrain(err_integral_R + (err_R * dt), -0.5f, 0.5f);
    float deriv_R  = (err_R - last_err_R) / dt;
    float out_R    = (ENC_KP * err_R) + (ENC_KI * err_integral_R) + (ENC_KD * deriv_R);
    last_err_R     = err_R;

    // Add the feedforward baseline and clamp to PWM bounds [0, 255]
    pwm_L = constrain((int)(target_v_L * 220.0f + out_L), 0, 255);
    pwm_R = constrain((int)(target_v_R * 220.0f + out_R), 0, 255);
}
```

**How the function works:**

1. It copies and clears the tick counters with interrupts disabled, so no pulse is lost or counted twice.
2. It converts ticks to wheel speed in m/s.
3. It runs a PID on each wheel's speed error, with the integral term clamped to prevent windup.
4. It adds a feedforward term (`target_v × 220`) so the PID only corrects the small remaining error instead of building the whole PWM from zero.

---

## 🛡️ 5. Odometer Drift vs. Topological Line Grounding

A basic problem in mobile robotics is **cumulative dead-reckoning slip error**:

```text
       ACTUAL TRACK:   ────────────────────────────────────────►
       ENCODER MAP:    ─────────────────────────- - - - ↗  (drifts 35 mm after 3 meters)
```

### The Odometry Paradox

Wheel encoders cannot sense wheel slip on slick vinyl arenas. Over a 10 m maze run, an uncorrected odometry system can drift by more than 50 mm in position and more than 15° in heading, which would corrupt any Cartesian-coordinate map of the maze.

### The Line-Maze Solution

In this system the wheel encoders are **never used for global coordinate navigation**. Instead:

1. **Encoders regulate local wheel dynamics.** If the robot asks for 0.85 m/s straight-line velocity, both wheels turn at matching mechanical speeds.
2. **The IR sensor array provides absolute topological grounding.** Each time the array crosses a node, line edge, or black mark, the physical line acts as an exact reference and resets heading and lateral position errors to zero.

---

## 🔗 Related Documentation

- 🏎️ **Extension 1:** [Optimal Route Replay & String Reduction](./optional_extension_1_maze_replay.md)
- 🧭 **Extension 2:** [Turn-Cost Modelling & Kinematics](./turn_cost_modelling.md)
- 🎛️ **Task 9:** [PID Controller Implementation & Validation](../Task9/pid_tuning_and_validation.md)