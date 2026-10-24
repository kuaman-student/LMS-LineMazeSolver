# 💻 Part 3 — Pseudocode, Finite State Machine & Path Reduction Engine

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Task 8:** Maze-Solving Algorithms
**Author:** Aman Kumar

---

## 📌 Executive Summary

This document presents the complete software architecture for the autonomous line-maze solver. It covers:
1. The **Dual-Mode Finite State Machine (FSM)** governing transitions between Exploration and Optimal Replay.
2. The **Algebraic Path Reduction Engine**, detailing the physical geometry and substitution identities of dead-end pruning.
3. The production-ready **C++/Arduino-style pseudocode**, including junction classification via the Pololu QTR-8A array, closed-loop turn execution with magnetic braking, and real-time path reduction.

```text
 ┌─────────────────────────────────────────────────────────────────────────────┐
 │                         FINITE STATE MACHINE (FSM)                          │
 ├─────────────────────────────────────────────────────────────────────────────┤
 │                                                                             │
 │   [ CALIBRATION ] ──► [ EXPLORATION_MODE ] ──► [ PATH_OPTIMIZATION ]        │
 │     Sensor Range         (LSRB Heuristic)         (6-Rule Reduction)        │
 │                                                           │                 │
 │                                                           ▼                 │
 │   [ GOAL_STANDBY ] ◄── [ REPLAY_SPEEDRUN ] ◄── [ USER_TRIGGER / PIT-STOP ]  │
 │     LED Indicator        (Full 500 RPM)          Ready for Run 2            │
 │                                                                             │
 └─────────────────────────────────────────────────────────────────────────────┘
```

---

## ⚙️ 1. Core System Architecture & State Machine

The control architecture is split across two operational phases executed deterministically at a **200 Hz (5 ms)** control cadence:

```text
                           200 Hz (5 ms) Control Cadence
                                         │
        ┌────────────────────────────────┴────────────────────────────────┐
        ▼                                                                 ▼
[ Normal Line Follow ]                                          [ Junction Detection ]
 • PD Centroid Error ($K_p, K_d$)                               • Outer Sensor Thresholds
 • Smooth Differential Steer                                    • Center Line Continuity
 • Zero Integrator ($K_i = 0$)                                  • Trigger State Machine
```

### Operational Modes

1. **`MODE_EXPLORATION` (Run 1 — Mapping Phase):**
   * **Cruising Speed:** Conservative $200 - 250\text{ RPM}$ ($PWM \approx 130/255$).
   * **Junction Protocol:** Robot drives forward $15 - 20\text{ mm}$ (inch-forward) to align wheels with the junction center and verify straight-line presence.
   * **Turn Selection:** Evaluates strict **LSRB** priority (`L` $\to$ `S` $\to$ `R` $\to$ `B`).
   * **Path Logging:** Appends every executed turn to a dynamic character array and runs in-line reduction on every `B` encounter.

2. **`MODE_REPLAY` (Run 2 — Speed-Run Phase):**
   * **Cruising Speed:** Maximum dynamic capacity $500\text{ RPM}$ ($PWM \approx 255$).
   * **Junction Protocol:** Zero inching delay. As soon as a junction is triggered, the pre-computed command `path[replayIndex++]` is executed immediately.
   * **Path Source:** Reads linearly from the reduced `path[]` array containing zero dead-end branches.

---

## 📐 2. The Algebraic Path Reduction Engine

### The Geometry of Dead-End Trapping
When a robot enters a dead-end branch, it is forced to execute a $180^\circ$ turn (`B`). From the reference frame of the parent junction, entering and exiting that branch introduces an unnecessary detour. 

```text
  EXAMPLE: Left into Dead-End, then Left out
  
         │                   │
         │   [ DEAD END ]    │
         │        ▲          │
         │        │ Step 2   │
         │        │ ('B')    │
         ├────────┴──────────┤
         │ Step 1 ('L')      │
  ───────┘                   └───────
  Robot In ──►               ──► Continues Straight
  
  Physical Outcome: Turning Left into a dead-end, making a U-turn (B), 
                    and turning Left again brings the robot onto the 
                    path it would have reached by simply going Straight!
                    ===>  L + B + L = S
```

### Complete 6-Rule Algebraic Substitution Matrix

Every sequence of three turns containing a central `B` ($X + B + Y$) simplifies into a single equivalent turn:

| Rule # | Traversal Sequence | Geometric Meaning | Simplified Action | Net Result |
| :---: | :---: | :--- | :---: | :--- |
| **1** | **`L + B + L`** | Left into dead-end $\to$ U-turn $\to$ Left exit | **`S`** | Direct Straight |
| **2** | **`L + B + S`** | Left into dead-end $\to$ U-turn $\to$ Straight exit | **`R`** | Direct 90° Right |
| **3** | **`L + B + R`** | Left into dead-end $\to$ U-turn $\to$ Right exit | **`B`** | Nested Backtrack |
| **4** | **`S + B + L`** | Straight into dead-end $\to$ U-turn $\to$ Left exit | **`R`** | Direct 90° Right |
| **5** | **`S + B + S`** | Straight into dead-end $\to$ U-turn $\to$ Straight exit | **`B`** | Nested Backtrack |
| **6** | **`R + B + L`** | Right into dead-end $\to$ U-turn $\to$ Left exit | **`B`** | Nested Backtrack |

### In-Line String Compression Algorithm
Instead of waiting for the full exploration to finish, path simplification runs **in-line immediately after each U-turn (`B`)**:
1. When a turn is executed, it is added to `path[pathLength]`.
2. If `path[pathLength - 1] == 'B'` and `pathLength >= 3`:
   * Extract sub-sequence: `A = path[pathLength - 2]`, `B = 'B'`, `C = path[pathLength]`.
   * Look up substitution in the 6-rule matrix $\to$ returns simplified character `X`.
   * Replace `path[pathLength - 2] = X`.
   * Decrement pointer: `pathLength -= 2`.
3. If `X == 'B'`, recursive reduction automatically cascades if preceded by another branch.

---

## 📝 3. Formal Algorithmic Pseudocode

The following language-agnostic algorithms describe the mathematical and decision-making logic of the solver:

### Algorithm 1: Continuous Weighted Centroid Estimation
```text
ALGORITHM ComputeWeightedCentroid(rawADC[1..8], calibMin[1..8], calibMax[1..8])
INPUT: Raw 12-bit ADC readings, calibration minima and maxima arrays
OUTPUT: Tuple (lineFound: Boolean, errorVal: Float, normalized[1..8]: FloatArray)

CONSTANTS:
    WEIGHTS = [-3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5]
    MIN_SUM_THRESHOLD = 150.0

BEGIN
    weightedSum ← 0.0
    totalActiveValue ← 0.0

    FOR i FROM 1 TO 8 DO
        normalized[i] ← (rawADC[i] - calibMin[i]) * 1000.0 / (calibMax[i] - calibMin[i])
        normalized[i] ← CLAMP(normalized[i], 0.0, 1000.0)
        
        weightedSum ← weightedSum + (normalized[i] * WEIGHTS[i])
        totalActiveValue ← totalActiveValue + normalized[i]
    END FOR

    IF totalActiveValue < MIN_SUM_THRESHOLD THEN
        RETURN (lineFound: FALSE, errorVal: 0.0, normalized: normalized)
    END IF

    errorVal ← weightedSum / totalActiveValue
    RETURN (lineFound: TRUE, errorVal: errorVal, normalized: normalized)
END ALGORITHM
```

### Algorithm 2: Junction Geometry Classification
```text
ALGORITHM ClassifyJunction()
INPUT: Output of ComputeWeightedCentroid()
OUTPUT: JunctionType ∈ { NONE, CROSS, T_JUNCTION, LEFT_ONLY, RIGHT_ONLY, DEAD_END, GOAL }

BEGIN
    (lineFound, errorVal, norm) ← ComputeWeightedCentroid()

    IF NOT lineFound THEN
        RETURN DEAD_END
    END IF

    activeCount ← COUNT(norm[i] > 600.0 FOR i IN 1..8)
    
    // Check for solid cross or goal pad
    IF activeCount ≥ 7 THEN
        DRIVE_FORWARD(distance = 20 mm)
        (lineFound, errorVal, norm) ← ComputeWeightedCentroid()
        activeCount ← COUNT(norm[i] > 600.0 FOR i IN 1..8)
        IF activeCount ≥ 7 THEN
            RETURN GOAL
        ELSE
            RETURN CROSS
        END IF
    END IF

    farLeft  ← (norm[1] > 600.0) OR (norm[2] > 600.0)
    farRight ← (norm[7] > 600.0) OR (norm[8] > 600.0)
    center   ← (norm[4] > 600.0) OR (norm[5] > 600.0)

    IF farLeft AND farRight THEN
        RETURN T_JUNCTION
    END IF

    IF farLeft AND NOT farRight THEN
        DRIVE_FORWARD(distance = 20 mm)
        (lineFound, errorVal, norm) ← ComputeWeightedCentroid()
        center ← (norm[4] > 600.0) OR (norm[5] > 600.0)
        IF center THEN
            RETURN T_JUNCTION  // Branch left with straight continuation
        ELSE
            RETURN LEFT_ONLY   // 90 degree corner
        END IF
    END IF

    IF farRight AND NOT farLeft THEN
        DRIVE_FORWARD(distance = 20 mm)
        (lineFound, errorVal, norm) ← ComputeWeightedCentroid()
        center ← (norm[4] > 600.0) OR (norm[5] > 600.0)
        IF center THEN
            RETURN T_JUNCTION  // Branch right with straight continuation
        ELSE
            RETURN RIGHT_ONLY  // 90 degree corner
        END IF
    END IF

    RETURN NONE
END ALGORITHM
```

### Algorithm 3: In-Line Algebraic Path Reduction
```text
ALGORITHM SimplifyPath(pathArray: Reference to CharArray, pathLength: Reference to Integer)
INPUT: Dynamic path array and its current length
OUTPUT: Simplified path array with reduced dead-end loops

BEGIN
    // Dead-end reduction requires at least 3 moves and a central 'B'
    IF pathLength < 3 OR pathArray[pathLength - 2] ≠ 'B' THEN
        RETURN
    END IF

    A ← pathArray[pathLength - 3]
    B ← pathArray[pathLength - 2]  // Guaranteed 'B'
    C ← pathArray[pathLength - 1]
    reducedAction ← NULL

    // Evaluate 6-Rule Reduction Matrix
    MATCH (A, B, C) WITH:
        ('L', 'B', 'L') → reducedAction ← 'S'
        ('L', 'B', 'S') → reducedAction ← 'R'
        ('L', 'B', 'R') → reducedAction ← 'B'
        ('S', 'B', 'L') → reducedAction ← 'R'
        ('S', 'B', 'S') → reducedAction ← 'B'
        ('R', 'B', 'L') → reducedAction ← 'B'
    END MATCH

    IF reducedAction ≠ NULL THEN
        pathArray[pathLength - 3] ← reducedAction
        pathLength ← pathLength - 2
        pathArray[pathLength] ← '\0'
    END IF
END ALGORITHM
```

### Algorithm 4: Top-Level Finite State Machine Orchestration
```text
ALGORITHM MazeSolverFSM()
STATE VARIABLES: currentState ← CALIBRATION, path ← [], pathLength ← 0, replayIndex ← 0

BEGIN
    WHILE TRUE DO
        EVERY 5 ms (200 Hz loop) DO
            SWITCH currentState DO
                CASE CALIBRATION:
                    EXECUTE SensorCalibrationRoutine()
                    currentState ← EXPLORATION
                
                CASE EXPLORATION:
                    junc ← ClassifyJunction()
                    IF junc ≠ NONE THEN
                        decision ← SELECT_LSRB_TURN(junc) // L > S > R > B
                        IF junc == GOAL THEN
                            EXECUTE HardBrake()
                            ACTIVATE_LED(PIN_LED_GOAL)
                            currentState ← OPTIMIZATION
                        ELSE
                            EXECUTE Turn(decision)
                            APPEND decision TO path
                            SimplifyPath(path, pathLength)
                        END IF
                    ELSE
                        EXECUTE ClosedLoopPDLineFollow(speed = EXPLORE_SPEED)
                    END IF

                CASE OPTIMIZATION:
                    WAIT 3000 ms ON GOAL PAD
                    DEACTIVATE_LED(PIN_LED_GOAL)
                    replayIndex ← 0
                    currentState ← REPLAY

                CASE REPLAY:
                    junc ← ClassifyJunction()
                    IF junc == GOAL THEN
                        EXECUTE HardBrake()
                        ACTIVATE_LED(PIN_LED_GOAL)
                        currentState ← STOP
                    ELSE IF junc ≠ NONE THEN
                        turn ← path[replayIndex]
                        replayIndex ← replayIndex + 1
                        EXECUTE Turn(turn)
                    ELSE
                        EXECUTE ClosedLoopPDLineFollow(speed = REPLAY_MAX_SPEED)
                    END IF

                CASE STOP:
                    EXECUTE HardBrake()
            END SWITCH
        END EVERY
    END WHILE
END ALGORITHM
```

---

## 🔍 Verification & Edge Case Handling

| Edge Case | Sensor Manifestation | Firmware Handling Mechanism |
| :--- | :--- | :--- |
| **False Dead-End on Track Bump** | Single channel drops abruptly to zero | Task 7 spatial filter ignores single-sensor dropouts; dead-end only triggers if **all 8 channels read zero**. |
| **Overshooting Cross-Junctions** | High inertia carries bot past line | Active magnetic short-brake ($20\text{ ms}$) locks both motor shafts immediately upon detection. |
| **Nested Dead-End Backtracks** | Exploration encounters successive dead-ends | Recursive invocation of `simplifyPath()` reduces cascaded `...B...B...` sequences into single optimal turns. |

---

## 🔗 Related Documentation
* 🧩 **Part 1:** [Algorithm Selection & Benchmark Analysis](./algorithm_selection.md)
* 🗺️ **Part 2:** [Graph Modeling, Example Maze & Replay Dynamics](./graph_modeling_and_example.md)
* 🗂️ **Master Dashboard:** [Task 8 Master README](./README.md)
