# ⚡ MOSFET (60-Second Field Guide)

**Context:** Toshiba TB6612FNG Driver • ESP32 • GA12-N20 Motors

---

## 💡 What is a MOSFET?
An electronic switch controlled by **voltage**, not current.

* Applying voltage to the **Gate (G)** opens an ultra-low resistance tunnel (`R_DS(ON)` ≈ 0.5 Ω) between **Drain (D)** and **Source (S)**.
* Gate draws zero DC current (`I_G` ≈ 0 A), so the ESP32 GPIOs never heat up or overload.

```text
  Gate (G)  ──► [Voltage Switch: ESP32 Pin]
  Drain (D) ──► [Power In: Battery 7.4V]
  Source (S)──► [Power Out: To Motor / GND]
```

---

## 🔄 H-Bridge in 4 Lines (TB6612FNG)
An H-bridge uses 4 MOSFETs (`Q1`, `Q2` high-side; `Q3`, `Q4` low-side) around each motor:

* **Forward:** `Q1 + Q4` ON ⇒ Current flows Left → Right.
* **Reverse:** `Q2 + Q3` ON ⇒ Current flows Right → Left.
* **Coast:** All OFF ⇒ Robot glides to a stop.
* **Brake:** `Q3 + Q4` ON ⇒ Shorts motor leads to GND (instant magnetic lock, zero overshoot on 90° turns).

---

## 🥊 Why TB6612FNG Beats L298N

| Feature | Old L298N (BJT) | Modern TB6612FNG (MOSFET) | Impact |
| :--- | :---: | :---: | :--- |
| **Voltage Loss** | ≈ 2.0 V dropped | ≈ 0.3 V (`I × R_ON`) | Motors get full battery voltage |
| **Heat at Stall** | 1.30 W (burning hot) | 0.21 W (barely warm) | **84% less heat generated** |
| **Heatsink** | Heavy metal plate required | Zero heatsink needed | **Saves 28.5 g dead weight** |
| **PWM Speed** | < 20 kHz (whining noise) | Up to 100 kHz (silent) | Smooth, precise PID tracking |

---

## ⚠️ 2 Ways MOSFETs Burn & How We Prevent It

1. **Gate-Oxide Puncture (`V_M` > 15 V):**
   * *Cause:* Motor inductance spikes (`V = -L × di/dt`) during sudden stops.
   * *Fix:* 470 µF low-ESR electrolytic + 0.1 µF ceramic cap placed right at the driver's `VM` pin.
2. **Switching Overheating:**
   * *Cause:* Lingering in the mid-transition zone between ON and OFF.
   * *Fix:* 10 ms software brake deceleration ramp before reversing direction.

---

## 🎯 3 Rules for Circuit Design
1. **`V_CC` → 3.3 V:** Keeps high threshold at `V_IH` = 2.31 V, matching ESP32 directly.
2. **`V_M` ≥ 4.5 V:** Required for PWM speed control; a 2S LiPo (7.4 V) is the sweet spot.
3. **Common GND:** Logic GND and Battery GND must connect at a single point to prevent reference shift.