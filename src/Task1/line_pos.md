# 📍 Five-Sensor Line Position

Estimates **where the line is** under a 5-sensor array using a weighted average of 5 ADC readings (`0–4095`). Output is one float number from `-2` to `+2`.

---

## 🧮 Method

```
   S1      S2      S3      S4      S5
   -2      -1       0      +1      +2
  LEFT  ◄─────────────────────────► RIGHT

position = (S1×-2 + S2×-1 + S3×0 + S4×1 + S5×2) / (S1 + S2 + S3 + S4 + S5)
```

| Result | Meaning |
|:---:|---|
| ⬅️ Negative | Signal is more towards the **left** |
| ⏺️ Near 0 | Signal is **balanced** around S3 |
| ➡️ Positive | Signal is more towards the **right** |

> 📌 Higher reading = stronger line signal (dark line on a lighter surface). For a white line, invert or calibrate first.

**Example:** `1000 2000 3000 3500 4095`
→ weighted sum = **7690**, total = **13595**, position = 7690 / 13595 = **0.5656** (slightly right of S3)

---

## ⚠️ Errors and Warnings

| Situation | Output |
|---|---|
| ❌ Any reading < 0 or > 4095 | Input error |
| ❌ All readings are 0 (sum = 0) | Cannot calculate position |
| ⚠️ All five sensors ≥ 3000 | Possible wide line or intersection |
| ⚠️ S1 and S5 both ≥ 3000 | Position may be confusing |

> 💡 Threshold `3000` is an example, so tune it on real sensors. Position `0` does not always mean "centered" (equal S1 and S5 cancel out). Warnings are only hints and do not confirm a junction.

---

## ✅ Test Cases

| # | Input | Expected output |
|:---:|:---:|---|
| 1 | `0 0 1000 0 0` | `0.0000` |
| 2 | `4095 0 0 0 0` | `-2.0000` |
| 3 | `0 0 0 0 4095` | `2.0000` |
| 4 | `0 0 1000 1000 0` | `0.5000` |
| 5 | `0 0 0 0 0` | ❌ Cannot calculate position |
| 6 | `4096 0 0 0 0` | ❌ Input error |
| 7 | `4095 0 0 0 4095` | `0.0000` + ⚠️ corner-sensor warning |
| 8 | `4095 4095 4095 4095 4095` | `0.0000` + ⚠️ wide line / intersection warning |

---

## 📓 Handwritten Notes

| Analyzing the problem and weighted center | Handling the exceptional cases |
|:---:|:---:|
| ![Note 1](../../docs/Handwritten%20Notes/Task1/image1.png) | ![Note 2](../../docs/Handwritten%20Notes/Task1/image2.png) |

---

## 🚀 Build and Run

```bash
g++ -std=c++17 line_pos.cpp -o line_position
./line_position            # Windows: .\line_position.exe
```

> 🧪 Manual test program. It does not read real sensors from the robot.

---

## 🤖 Credits

- 💻 **Code and logic:** Aman Kumar, with some help from GitHub Copilot (in VS Code)
- 📝 **README formatting and wording:** drafted with help from Claude (Anthropic)
- 🔎 **Review:** test cases and example verified by hand