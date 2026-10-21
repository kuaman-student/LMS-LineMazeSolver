# ⚙️ Motor Command Limiting

Keeps the left and right motor commands inside `[-max, +max]` **without changing their sign**, because a negative value means the motor spins in reverse.

---

## 🧮 Logic

```
if command >  max  →  max
if command < -max  → -max
otherwise          →  command (unchanged)
```

Same thing as one formula: `clamped = max(-M, min(command, M))`

**Example:** left = `300`, right = `-180`, max = `255`

| Motor | Command | Check | Final |
|:---:|---:|---|---:|
| Left | 300 | 300 > 255, so cut to max | **255** |
| Right | -180 | inside -255 to 255, so keep | **-180** |

```
Enter left command, right command, max magnitude: 300 -180 255
Left motor:  255
Right motor: -180
```

---

## ✅ Test Cases

| # | Left | Right | Max | Left out | Right out |
|:---:|---:|---:|---:|---:|---:|
| 1 | 100 | -100 | 255 | 100 | -100 |
| 2 | 300 | -180 | 255 | 255 | -180 |
| 3 | -300 | 300 | 255 | -255 | 255 |
| 4 | 0 | 0 | 255 | 0 | 0 |
| 5 | 255 | -255 | 255 | 255 | -255 |
| 6 | 50 | -50 | 0 | 0 | 0 |
| 7 | 100 | 100 | -5 | ❌ "Max magnitude must be 0 or more." | |

> 💡 The limit is checked on each motor separately, so one motor being clamped does not affect the other.

---

## 📓 Handwritten Notes

| Basics of clamping | Deeper notes on Integral windup (Made from MATLAB YT channel) |
|:---:|:---:|
| ![Note 1](../../docs/Handwritten%20Notes/Task1/image3.png) | ![Note 2](../../docs/Handwritten%20Notes/Task3/image7.png) |

---

## 🚀 Build and Run

```bash
g++ -std=c++17 motor_limit.cpp -o motor_limit
./motor_limit            # Windows: .\motor_limit.exe
```

> 🧪 Manual test program. It does not read real sensors from the robot.

---

## 🤖 Credits

- 💻 **Code and logic:** Aman Kumar, with some help from GitHub Copilot (in VS Code)
- 📝 **README formatting and wording:** drafted with help from Claude (Anthropic)
- 🔎 **Review:** test cases and example verified by hand