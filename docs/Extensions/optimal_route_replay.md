# ⚡ Optional Extension 1: Maze Exploration & Optimal-Route Replay

**Techfest Meshmerize — Autonomous Line-Maze Solver**
**Subsystem:** Two-Pass Maze Traversal, LSRB String Reduction & Autonomous Speed-Run
**Target Architecture:** ESP32-WROOM-32 (Flash/RAM-efficient in-place pruning)

---

## 📌 1. High-Level Concept: The Two-Pass Strategy

Competitive line-maze solving is split into two distinct operational phases:

```text
  ┌─────────────────────────┐                     ┌─────────────────────────┐
  │   PASS 1: EXPLORATION   │                     │    PASS 2: SPEED-RUN    │
  │     (The "Dry Run")     │                     │     (The "Replay")      │
  │ • Unknown maze topology │                     │ • Fully resolved path   │
  │ • Strict Left-Hand Rule │───►[ IN-PLACE ]───► │ • Zero dead-end entries │
  │ • Enters dead-ends (B)  │    [ PRUNING  ]     │ • Full acceleration     │
  │ • Records raw junctions │                     │ • Deterministic replay  │
  └─────────────────────────┘                     └─────────────────────────┘
```

1. **Exploration Run (Mapping & Discovery):**
   The robot starts with an empty buffer. At each junction it applies the strict **Left-Hand Rule priority** (`L` > `S` > `R` > `B`), and every decision taken is logged into an array.
2. **Path Optimization (Pruning Dead-Ends):**
   Whenever a dead-end (`B`) is logged, the incoming turn, the dead-end rebound, and the outgoing turn are compressed into a single equivalent turn using topological substitutions.
3. **Speed-Run Replay (Optimal Execution):**
   The robot is placed back on the Start Pad. It runs pure PID line following on straight sections, but at junctions it skips the sensor decision logic and executes the pre-computed optimal sequence directly from memory.

---

## 🔬 2. Theoretical Formulation: LSRB Substitution Algebra

In a loop-free, simply connected tree graph, entering a dead-end forces a 180° back-turn (`B`). The trip into and out of that dead-end forms a redundant sub-path of the form:

```text
Sequence = T_in → B → T_out
```

Geometrically, this three-step sequence can be replaced by a single equivalent action:

```text
CASE 1: L-B-L ──► S                             CASE 2: L-B-S ──► R
    ┌──────────┐                                    ┌──────────┐
    │ Dead End │                                    │ Dead End │
    └───▲──┬───┘                                    └───▲──┬───┘
     (L)│  │(L)                                      (L)│  │(S)
   ─────┘  └─────► Going Straight                 ──────┘  └──────
         ▲                                               ▲
         │ Incoming                                      │ Incoming
```

### Complete Reduction Transformation Matrix

The substitution set covers all 6 possible dead-end transitions under the Left-Hand Rule:

| Raw Sequence | Geometric Equivalent | Net Rotation Angle | Physical Meaning |
| --- | --- | --- | --- |
| **`L - B - L`** | **`S`** | +90° + 180° + 90° = 360° ≡ 0° | Skip the left pocket; proceed straight. |
| **`L - B - S`** | **`R`** | +90° + 180° + 0° = 270° ≡ −90° | Left pocket is a dead-end; take the right turn instead. |
| **`L - B - R`** | **`B`** | +90° + 180° − 90° = 180° | Double nested dead-end; triggers a cascade. |
| **`S - B - L`** | **`R`** | 0° + 180° + 90° = 270° ≡ −90° | Straight path is dead; take the right fork. |
| **`S - B - S`** | **`B`** | 0° + 180° + 0° = 180° | Straight corridor is dead; cascade backward. |
| **`R - B - L`** | **`B`** | −90° + 180° + 90° = 180° | T-stem dead-end; cascade backward. |

> **Recursive Cascading Property:** If a reduction evaluates to `B` (for example `L-B-R` → `B`), that new `B` can combine with the preceding turn (for example `L - [B] - L` → `S`), triggering another in-place simplification automatically.

---

## 🧪 3. Worked Walkthrough Example

Consider an exploration run that produces this raw sequence:

```text
Raw Log = [ L, L, B, L, S, L, B, S, R ]
```

```text
Step 1: Scan for first 'B' (Index 2)
  [ L, (L, B, L), S, L, B, S, R ]
        └──┬──┘
         LBL ──► S
  Array becomes: [ L, S, S, L, B, S, R ]

Step 2: Scan for next 'B' (Index 4)
  [ L, S, S, (L, B, S), R ]
              └──┬──┘
               LBS ──► R
  Array becomes: [ L, S, S, R, R ]

Final Optimized Replay Path:
  [ L ──► S ──► S ──► R ──► R ]   (zero U-turns, shortest topological route)
```

---

## 💻 4. ESP32 Implementation Architecture

The path optimizer works in place on a fixed-size buffer, so there is no dynamic memory allocation (`malloc`) and no heap fragmentation:

```cpp
#ifndef PATH_OPTIMIZER_H
#define PATH_OPTIMIZER_H

#include <Arduino.h>

class PathOptimizer {
private:
    char path[128];     // Static buffer: supports mazes up to 128 decisions
    uint8_t length;     // Current valid path length

    // Performs a single 3-character reduction pass on the tail of the path
    bool reduce_step() {
        if (length < 3) return false;

        // The middle element of the last 3 items must be a dead-end 'B'
        if (path[length - 2] != 'B') return false;

        char action_in  = path[length - 3];
        char dead_end   = path[length - 2]; // 'B'
        char action_out = path[length - 1];
        char resolved   = ' ';

        if      (action_in == 'L' && action_out == 'L') resolved = 'S';
        else if (action_in == 'L' && action_out == 'S') resolved = 'R';
        else if (action_in == 'L' && action_out == 'R') resolved = 'B';
        else if (action_in == 'S' && action_out == 'L') resolved = 'R';
        else if (action_in == 'S' && action_out == 'S') resolved = 'B';
        else if (action_in == 'R' && action_out == 'L') resolved = 'B';
        else return false; // Unhandled combination

        // In-place replacement: contract the buffer by 2 entries
        path[length - 3] = resolved;
        length -= 2;
        return true;
    }

public:
    PathOptimizer() : length(0) {}

    // Adds a junction decision and triggers instant on-the-fly reduction
    void record_turn(char turn) {
        if (length < 128) {
            path[length++] = turn;
            // Keep reducing while a dead-end pattern exists at the tail
            while (reduce_step());
        }
    }

    uint8_t get_path_length() const { return length; }
    char get_action(uint8_t index) const { return (index < length) ? path[index] : 'E'; }

    void print_path() const {
        Serial.print(F("[REPLAY PATH] (Length "));
        Serial.print(length);
        Serial.print(F("): "));
        for (uint8_t i = 0; i < length; i++) {
            Serial.print(path[i]);
            if (i < length - 1) Serial.print(F(" -> "));
        }
        Serial.println();
    }
};

#endif
```

---

## 🏎️ 5. Replay Phase State Machine

During Pass 2, junction detection triggers the pre-recorded turn without evaluating sensor priorities:

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                      INTERSECTION DETECTED                             │
 │            (Array senses: Left / T-Junction / Cross)                   │
 └───────────────────────────────────┬────────────────────────────────────┘
                                     │
                     Is Robot in REPLAY Mode?
                                     │
                    ┌────────────────┴────────────────┐
                    ▼ YES                             ▼ NO (Exploration)
    ┌──────────────────────────────┐   ┌──────────────────────────────┐
    │ Command = path[replay_idx++] │   │ Execute Left-Hand Rule (LSRB)│
    │ Drive forward through turn   │   │ Record chosen action         │
    │ Zero pause, maximum cruise   │   │ Trigger path.record_turn()   │
    └──────────────────────────────┘   └──────────────────────────────┘
```

### Performance Impact

| Parameter | Pass 1: Exploration | Pass 2: Speed-Run | Gain / Advantage |
| --- | --- | --- | --- |
| **Average Velocity (V_base)** | 160 PWM (0.45 m/s) | 225 PWM (0.85 m/s) | **+88% speed boost** |
| **Dead-End Penalties** | Traverses every false alley | Zero false-alley entries | **100% dead-end pruning** |
| **Junction Stoppage** | 120 ms sensor confirmation | Continuous rolling transition | **Smooth inertial momentum** |
| **Memory Footprint** | Dynamic allocation: none | Static array: under 130 bytes | **Zero heap fragmentation** |