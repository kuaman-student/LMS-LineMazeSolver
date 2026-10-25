# 🧭 Optional Extension 2: Weighted Routing & Turn-Cost Kinematic Modelling

**Techfest Meshmerize — Autonomous Line-Maze Solver**
**Subsystem:** Inertia-Aware Path Optimization, Trajectory Cost Formulation & Graph Weighting
**Author:** Aman Kumar

---

## 📌 1. The Core Engineering Problem: Geometric vs. Temporal Shortest Path

Standard pathfinding algorithms (unweighted BFS or distance-based Dijkstra) define the "optimal route" as the path with the minimum physical distance or the fewest edge transitions:

```text
Geometric Cost = sum of d(u, v) over every edge (u, v) in the path
```

### The Kinematic Reality

A differential-drive robot does not travel along a maze line at constant velocity. Its motors have rotational inertia and finite torque, so each type of move costs a different amount of time:

- **Straight traversal (S):** The robot holds cruising velocity (about 0.85 m/s) with almost no acceleration penalty.
- **Orthogonal turns (L / R):** The robot must brake to a stop, pivot 90° around its axle, and accelerate back to cruise speed.
- **U-turns (B):** The robot needs full active braking and a 180° in-place rotation, which wastes critical competition time.

> **The Turning Paradox:** A path that is physically 10 cm longer but runs straight can finish significantly faster than a shorter path made of frequent 90° switchbacks.

```text
PATH A (Shorter geometry, high turning cost):
  Start ──► [Turn 90° L] ──► [Turn 90° R] ──► [Turn 90° L] ──► Goal
  (Total: 80 cm, 3 turns)  →  Slow: repeated deceleration and pivot delays

PATH B (Longer geometry, pure straight cruise):
  Start ───────────────────► [Turn 90° L] ────────► Goal
  (Total: 95 cm, 1 turn)   →  Faster: sustained continuous momentum
```

---

## 🔬 2. Physics & Kinematic Formulation

Instead of a pure distance metric, each edge gets a **time-cost** that depends on the heading the robot arrives with:

```text
J(u, v, θ_in) = t_traversal(u, v) + t_turn(Δθ)
```

Where:

- **Linear traversal time:**

  ```text
  t_traversal = d(u, v) / V_cruise
  ```

- **Rotational turn penalty:**

  ```text
  t_turn(Δθ) = t_decel + ( |Δθ| / ω_pivot ) + t_accel
  ```

  Here `t_decel` is the braking time, `ω_pivot` is the pivot angular speed, and `t_accel` is the time to get back to cruise speed.

### Empirical Kinematic Budget (N20 motors @ 7.4 V LiPo)

Measured against a standard 200 Hz control loop and micro metal gearmotors:

| Action | Heading Change (Δθ) | Kinematic Sequence | Time Penalty (t_turn) | Equivalent Distance (Δd_eq) |
| --- | --- | --- | --- | --- |
| **Straight (S)** | 0° | Zero braking; continuous trajectory tracking | **0.04 s** | ≈ 3.4 cm |
| **Turn Left (L)** | +90° | Active brake → pivot 90° → re-accelerate | **0.42 s** | ≈ 35.7 cm |
| **Turn Right (R)** | −90° | Active brake → pivot 90° → re-accelerate | **0.42 s** | ≈ 35.7 cm |
| **U-Turn (B)** | 180° | Active short-brake → full 180° spin → track lock | **0.95 s** | ≈ 80.8 cm |

The last column converts a time penalty into the straight-line distance the robot could have covered in that time:

```text
Δd_eq = t_turn × V_cruise        (with V_cruise = 0.85 m/s)
```

---

## ⚖️ 3. Comparative Analysis: Geometric Shortest vs. Time-Optimal

Consider a bifurcation in the maze that offers two candidate routes between node N1 and the goal node N5:

```text
       [N2] ────── (90° R) ──────► [N3]
      ▲                                \ (90° L)
(90° L)                                  ▼
[N1] ────────────────────────────────► [N5] (Goal)
                     (Straight)
```

| Trajectory Candidate | Segment Lengths | Turns Required | Raw Distance | Calculated Travel Time | Status |
| --- | --- | --- | --- | --- | --- |
| **Route 1 (Zig-Zag)** | N1→N2 (25 cm); N2→N3 (30 cm); N3→N5 (20 cm) | 1× Left, 1× Right, 1× Left (all 90°) | **75 cm** | 0.75 / 0.85 + 3 × 0.42 = **2.14 s** | Sub-optimal (high turning lag) |
| **Route 2 (Direct)** | N1→N5 (95 cm) | None (straight through) | **95 cm** | 0.95 / 0.85 + 1 × 0.04 = **1.16 s** | **Optimal (about 46% faster)** |

> **Conclusion:** Route 2 is physically 20 cm longer, yet it reaches the goal about **0.98 seconds faster**, because it removes three full stop → pivot → accelerate cycles.

---

## 💻 4. Algorithmic Implementation: State-Augmented Dijkstra Cost Engine

To bring turn costs into Dijkstra's algorithm, each graph state must remember the **incoming heading**. A state is the pair `(u, θ)`, where `u` is the intersection index and `θ` is one of 0°, 90°, 180°, 270°.

The class below computes the edge cost that the Dijkstra relaxation step uses.

```cpp
#ifndef TURN_AWARE_DIJKSTRA_H
#define TURN_AWARE_DIJKSTRA_H

#include <Arduino.h>

const int MAX_NODES = 32;
const float INF_COST = 99999.0f;

// Turn penalty definitions in milliseconds
const float PENALTY_STRAIGHT = 40.0f;   // 0.04 s
const float PENALTY_TURN_90  = 420.0f;  // 0.42 s
const float PENALTY_U_TURN   = 950.0f;  // 0.95 s
const float CRUISE_SPEED_MM_MS = 0.85f; // 0.85 mm/ms (850 mm/s)

struct Edge {
    uint8_t target_node;
    float distance_mm;
    uint16_t heading_deg; // 0: North, 90: East, 180: South, 270: West
};

class TurnCostRouter {
public:
    // Computes the turn penalty from the angle between incoming and outgoing edges
    static float compute_turn_penalty(uint16_t in_heading, uint16_t out_heading) {
        int diff = abs((int)out_heading - (int)in_heading) % 360;
        if (diff > 180) diff = 360 - diff;

        if (diff == 0)   return PENALTY_STRAIGHT;
        if (diff == 90)  return PENALTY_TURN_90;
        if (diff == 180) return PENALTY_U_TURN;
        return PENALTY_TURN_90; // Default orthogonal fallback
    }

    /**
     * State-augmented edge cost:
     * total transit time = travel time + physical deceleration/pivot delay.
     */
    static float calculate_edge_cost(float distance_mm, uint16_t in_heading, uint16_t out_heading) {
        float transit_time_ms = distance_mm / CRUISE_SPEED_MM_MS;
        float kinematic_penalty_ms = compute_turn_penalty(in_heading, out_heading);
        return transit_time_ms + kinematic_penalty_ms;
    }
};

#endif
```

---

## 🏁 5. Hardware Integration & Execution Trade-Offs

| System Metric | Traditional Distance Dijkstra | Inertia-Aware Turn-Cost Router |
| --- | --- | --- |
| **Search Space** | V nodes; complexity O(E log V) | 4V (node, heading) states; still O(E log V), with about 4× more states |
| **Memory Footprint** | ≈ 256 bytes RAM | ≈ 1.1 KB RAM (easily fits in the ESP32's 520 KB SRAM) |
| **Physical Replay Time** | Risk of slow runs due to excessive chicanes | Replay trajectory actively seeks uninterrupted straightaways |
| **Motor Thermal Stress** | Repeated high stall-current draws during pivot turns | Maximizes continuous cruising, which reduces inductive L·di/dt peaks |

---

## 🔗 Related Documentation

- 🚀 **Extension 1:** [Two-Pass Exploration & LSRB Replay](./optional_route_replay.md)
- 📐 **Task 8 Master:** [Graph Modelling & Example Trace](../Task8/graph_modelling_and_example.md)
- 🎛️ **Task 9 Firmware:** [PID Tuning & Test Harness](../../src/Task9/pid_tuning_and_validation.md)