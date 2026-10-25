# 🧩 Part 1 — Algorithm Selection & Theoretical Benchmark

**Techfest Meshmerize — Autonomous Line-Maze Solver**  
**Task 8:** Maze-Solving Algorithms
---

## 📌 Executive Summary

For an unknown competition line-maze, the robot must balance complete topological coverage with real-world embedded hardware constraints. This document evaluates **Depth-First Search (DFS)** against the **Left-Hand Wall-Preference Rule (LSRB)**, analyzes the failure modes of metric pathfinders (**Dijkstra / A\***) under physical wheel-slip, and justifies selecting **LSRB with Algebraic Path Simplification** as the production navigation engine.

```text
                           Maze Algorithm Topology
                                      │
       ┌──────────────────────────────┴──────────────────────────────┐
       ▼                                                             ▼
 [ Graph DFS ]                                             [ Wall Follower: LSRB ]
  • Node stack in RAM (O(V))                                • O(1) Reactive decisions
  • Requires odometry / loop detection                      • Immune to vinyl wheel-slip
  • Handles multiply connected mazes                        • Solves simply connected trees
       │                                                             │
       └──────────────────────────────┬──────────────────────────────┘
                                      ▼
                        [ Selected: LSRB + Reduction ]
```

---

## ⚖️ 1. Theoretical Comparison: DFS vs. Left-Hand Rule

```text
                  DFS Tree Backtracking                 Left-Hand Rule (LSRB) Reactive
                ┌─────────────────────────┐               ┌─────────────────────────┐
Exploration:    │ Explicit Stack (LIFO)   │               │ Strict Priority Sensor  │
                │ Vertex memory: visited[]│               │ Priority: L -> S -> R -> B│
Backtracking:   │ Physical reverse trace  │               │ 180° turn (B) on dead-end│
Storage Cost:   │ O(V + E) Adjacency list │               │ O(N) Action string      │
Sensory Basis:  │ Coordinate / Node IDs   │               │ Direct IR Line Geometry │
                └─────────────────────────┘               └─────────────────────────┘
```

### Comprehensive Comparison Matrix

| Evaluation Parameter | Full Graph Depth-First Search (DFS) | Left-Hand Rule (LSRB) with String Reduction | Physical / Competition Implication |
| :--- | :--- | :--- | :--- |
| **Topological Scope** | Solves **Simply Connected** (tree) & **Multiply Connected** (cyclic) mazes. | Guaranteed for all **Simply Connected** mazes. Trapped by isolated island loops. | Techfest Meshmerize rules enforce simply connected mazes without floating loop islands. |
| **Memory Footprint** | High: Adjacency list, edge weights, node coordinates, and `visited[]` set. | Minimal: Single dynamic character array recording intersection choices (`L`, `R`, `S`, `B`). | ESP32 SRAM is conserved; zero dynamic allocation fragmentation risk. |
| **Odometry Dependency** | **Critical:** Must measure metric displacement $(X,Y)$ to verify revisited nodes. | **Zero:** Decisions depend solely on instantaneous 8-channel IR sensor geometry. | Wheel slip across smooth vinyl destroys metric coordinate accuracy. |
| **Decision Latency** | $\approx 2 - 5\,\text{ms}$ (Graph traversal, hash lookup, stack mutation). | **$< 0.1\,\text{ms}$** (Pure combinational priority evaluation). | Enables immediate turn decisions at high velocities without stopping. |
| **Failure Recovery** | Catastrophic if a node coordinate error cascades through graph edges. | Self-healing: Misaligned junctions are re-acquired via line-following PD. | High tolerance to mechanical vibration and track surface irregularities. |
| **Replay Optimization** | Computes topological path over saved tree edges. | Algebraic string reduction (`LBL -> S`, `LBS -> R`, etc.). | Replay path reduces to the identical single unique route in tree networks. |

---

## 🏆 2. Engineering Justification: Why LSRB is Selected

For an **unknown line-maze**, theoretical optimality must yield to physical electromechanical stability:

```text
 [ Wheel Acceleration / Braking ] ──► [ Micro-Slip on Vinyl Surface ] ──► [ (X, Y) Odometry Drift ]
                                                                                   │
 ┌─────────────────────────────────────────────────────────────────────────────────┘
 ▼
 [ False Node Merging in DFS ] ──► [ Desynchronized Coordinate Map ] ──► [ ROBOT CRASH ]
```

1. **The Wheel-Slip Barrier:**  
   Brushed DC motors accelerating at 500 RPM generate micro-slip against smooth vinyl tracks. While optical wheel encoders track motor shaft rotations, mechanical slip introduces cumulative dead-reckoning drift:

$$\Delta \vec{P}_{\text{error}} = \int_{0}^{t} \mathbf{R}(\theta) \cdot \vec{v}_{\text{slip}}(\tau) \, d\tau$$

   Under DFS, an accumulated error of just $30\text{ mm}$ causes the robot to misidentify a newly discovered junction as an existing node, corrupting the adjacency tree. LSRB uses purely topological line references, remaining immune to distance errors.

2. **Topological Invariant of Simply Connected Mazes:**  
   A line-maze without loops forms a topological tree graph ($|E| = |V| - 1$). In such a graph:
   * Exactly **one unique, non-repeating path** connects the Start Pad to the Goal Pad.
   * Continuous boundary wall tracking mathematically traverses every branch until the exit is encountered.

3. **Deterministic Real-Time Execution:**  
   Evaluating the priority chain (`Left` $\to$ `Straight` $\to$ `Right` $\to$ `Back`) requires single-cycle bitmask checks on the calibrated IR array. The robot does not need to pause at junctions to query a spatial graph.

---

## 🛡️ 3. "Why Not Dijkstra or A*?"

*"Why rely on heuristic wall-following when Dijkstra or A\* computes provably shortest paths?"* The mathematical counters are clear:

```text
                  Dijkstra / A* Limitations in Unknown Mazes
   ┌───────────────────────────────────────────────────────────────────────┐
   │ 1. Unknown Graph Topology: Cannot run without prior edge knowledge.   │
   │ 2. Heuristic Failure: A* requires known Euclidean target coordinates.  │
   │ 3. Single Unique Path: In tree graphs, shortest path == only path.    │
   └───────────────────────────────────────────────────────────────────────┘
```

### Defense Point 1: Dijkstra Requires an Offline Map
* **Mathematical Reality:** Dijkstra solves the Single-Source Shortest Path (SSSP) problem on a **fully known, pre-weighted graph** $G = (V, E, w)$ where $w(u,v) \ge 0$.
* **The Exploration Paradox:** During the first exploratory run, the graph does not exist in memory. The robot must physically traverse edges to measure distances. Dijkstra cannot guide exploration into unexplored space—it can only run *post-exploration*.

### Defense Point 2: A* Heuristic Collapse in Blind Mazes
* **Heuristic Requirement:** A\* guides search using the evaluation function:
  $$f(n) = g(n) + h(n)$$
  where $h(n)$ is an admissible heuristic (e.g., Euclidean distance $\sqrt{(X_g - X_n)^2 + (Y_g - Y_n)^2}$).
* **Physical Reality:** In a blind competition arena, the target goal pad coordinates $(X_g, Y_g)$ are **unknown**. Without a target reference point, $h(n) = 0$, degenerating A\* into unguided Dijkstra.

### Defense Point 3: The Tree Graph Path Equivalence
* In simply connected line-mazes, cycles do not exist. Therefore, between Start ($S$) and Goal ($G$), there is **only one simple path**:
  $$\Pi(S, G) = \{e_1, e_2, \dots, e_k\}$$
* When LSRB string reduction prunes all backtrack loops (`B`), the surviving sequence is identical to the path computed by Dijkstra:
  $$\text{Path}_{\text{LSRB\_Reduced}} \equiv \text{Path}_{\text{Dijkstra\_Optimal}}$$
* Running an $O(V \log V + E)$ priority-queue algorithm over a tree graph yields the same result as a string reduction pass, but incurs substantial memory overhead and coordinate drift risk.

---

## 📋 4. Comparative Algorithm Summary

```text
 +---------------------------------------------------------------------------------------+
 | Algorithm   | Exploration? | Shortest Path? | Memory   | Odometry Sensitivity | Verdict |
 |-------------+--------------+----------------+----------+----------------------+---------|
 | LSRB (Wall) | YES (Direct) | YES (on Trees) | O(N)     | ZERO (Immune)        | SELECTED|
 | DFS (Graph) | YES (Stack)  | First Path     | O(V + E) | LOW-MEDIUM           | REJECTED|
 | Dijkstra    | NO (Replay)  | YES (Weighted) | O(V^2)   | CRITICAL (Weights)   | OVERKILL|
 | A* Search   | NO (Replay)  | YES (Heuristic)| O(V)     | CRITICAL (Coords)    | INVALID |
 +---------------------------------------------------------------------------------------+
```

---

## 📚 References & Authority Sources
- 📄 **Cormen, Leiserson, Rivest, Stein (CLRS):** *Introduction to Algorithms* — Chapter 22 (Elementary Graph Algorithms: DFS).
- 📄 **Pololu Robotics:** [3pi Robot Maze Solving Logic & Dead-End Elimination](https://www.pololu.com/docs/0J21/8.a)
- 📄 **IEEE Transactions on Robotics:** *Error Propagation in Odometry-Based Mobile Mapping*.
- 🛠️ **Author:** Aman Kumar 