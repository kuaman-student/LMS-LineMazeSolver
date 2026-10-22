# 📋 Task 4: Bill of Materials & Component Selection

Autonomous line-maze robot architecture for **Techfest Meshmerize (IIT Bombay)**.

---

## 🔗 Quick Links

- 📊 **Google Sheets BOM:** [BOM Sheet](https://docs.google.com/spreadsheets/d/1ojaPZ_JZtjW4_C1dWolM55V-_D-JKaqS8tzH1k0BkUU/edit?gid=0#gid=0)
- 📜 **Competition Rules:** Size $\le 220 \times 220 \times 220\text{ mm}$ | Budget $\le ₹10,000$ | Single start switch | End-zone Red LED

---

## 💰 Budget Snapshot

| Item | Cost (₹) | Details |
| :--- | :---: | :--- |
| **Components (16 items)** | ₹6,597.10 | Core setup + pit spares (dual ESP32, dual LiPo, extra driver) |
| **Shipping** | ₹200.00 | Priority Air to IIT Mandi (PIN 175005) |
| **Contingency (25%)** | ₹1,649.28 | Test track tape (30mm vinyl), spacers, emergency buffer |
| **Total Project Cost** | **₹8,446.38** | **₹1,553.62 safely under the ₹10,000 limit** ✅ |

---

## ⚡ Major Choice: TB6612FNG vs. L298N

We selected the **TB6612FNG** over the older **L298N** dual H-bridge driver.

### 📊 Quick Comparison

| Feature | Selected: TB6612FNG | Alternative: L298N | Why It Matters |
| :--- | :---: | :---: | :--- |
| **Voltage Drop** | **~0.2V** (MOSFET) | **~2.0V** (BJT) | Motors receive >7.2V vs only 5.4V on 2S LiPo. |
| **Logic Level** | **3.3V & 5V native** | **Needs 5V TTL** | Direct connection to ESP32 GPIO; no level-shifters. |
| **Size & Weight** | **20 × 20 mm (~5g)** | **43 × 43 mm (~30g)** | Saves space and weight inside the 220mm box constraint. |
| **Thermal Loss** | Runs cool (no heatsink) | Requires heavy aluminum heatsink | Prevents thermal throttling during the 5.5-min run. |
| **Braking** | **Active short brake** | Coast to stop | Prevents overshooting sharp 90° intersections. |

### 🛑 Example: Reaching a 90° Turn

```text
Robot at 500 RPM ──► Line sensors detect 90° junction
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
 [ With L298N ]           [ With TB6612FNG ]
 Coasts to a stop         Active dynamic brake stops instantly
 Slides 2-3 cm past line  Locks precisely on the junction
 ❌ Over-shoots turn       ✅ Executes clean 90° pivot
```

> **Verdict:** The L298N is too bulky and inefficient for battery-powered mini-bots. The TB6612FNG provides maximum motor voltage, cool operation, and precise stops.

---

## 🔌 Power Architecture

- ⚡ **Interactive Power Diagram:** [Draw.io Live Power Block Diagram](https://viewer.diagrams.net/?tags=%7B%7D&lightbox=1&highlight=0000ff&edit=_blank&layers=1&nav=1&title=PowerBlockDiagram&dark=auto#R%3Cmxfile%3E%3Cdiagram%20name%3D%22Page-1%22%20id%3D%22aDdpTUx8y8Sh7_ZTIIzu%22%3E7V1tc5s6Fv41nrn3QzIgXv0xxmnWc5s2a6dpd7%2FsEJvY3GLwYtyk%2B%2BtXEi8GCUmmBmynytzpNeIcSYij5zl6Oww0Z%2F12F7ub1X208IIBUBZvA208AEC1LBP%2BD6X8TFOGyjBNWMb%2BIhPaJ8z8%2F3lZopKl7vyFt60IJlEUJP6mmjiPwtCbJ5U0N46j16rYSxRUS924S49KmM3dgE796i%2BSVZpqA2uf%2Bg%2FPX67yklUze761mwtnT7JduYvotZSk3Q40J46iJP21fnO8ADVe3i6p3gfG3aJisRcmNQpftl78%2Bflv1CZACdxn%2BF6wUFY7L167%2FmLsJm6WbI0GAL4rDTa8Bn8q%2BD%2FzvztUvdECyRVXA%2B2mcvcliF7nKzdGJX2cDgwnTPVHN48DYwSv96JghmT8hwjddpPEi3%2FCmwG69RyjugNziS6sa%2F0JinyK1n7oBjg7eE%2Bx0%2BQHz%2F1eq6YairK%2BWVWKHBjjfZVmX8kazR5mj%2FDGvbuFlYE%2FptH8O%2F4xe%2FWT%2Baq2mOkOmgdQUEM60XoT%2BG4499iFjr44f5HF3vuhj4p9UA1bv%2F2EpHbz7zjD8IcX47rUlDxLvM3VOHoNcT%2BA%2FxjXCmqRqbfcBW7iLdi1GE8nT7dTsh6PI9NUwYdPd0hih5v6Pkoi1ADj2Ic1YWd4O3sgc4NJGmqVsffjLx%2BZw5PK1p9QlTGunJUL%2BzGqxaMzfYRvMxMEyk0cuz%2FZmd1%2Ffvw8nZEZfvRektRU3fUmtesp7qzQtFI19KxbdrbTWypP1M%2Bc28HNEDf71t%2Bi1mJm8PF2TGYwhW8pvcGxLQ01cLi4%2BncUcizr7tP4jz8qt6E9rqMwvVW58eefWK3SOYFydZWXO7Ac3OluwcBWBkMt62pfwrgwLcvJelCpNxFZFH1aCfzQy4306T7VzU2wqf4E9o9RmkXalyo9i8iC6hFA%2BfzlscgAW21D%2FSfHSZUn02ojosyg%2BnVZX7vWMEZ9vU9xEv64qX%2F8A7VHjbXHfgxx34d2gPOYfFKPzwLUZ5FdE22YQ0i0Sza7ZJuq5l1U1IB3D5PPqQbufeWuSIriTqS4IXQ7Ug3c3UoAQyrcjKHUB1wCZGQv3NLVzA2E6ilXqG6wV2GptHtV7ZgtUNgZW6TUkGyhrLk4EhNRFtlb4Aul7cqQKKRgF%2F0%2BS35ivILXDnRJHOzpJHH03cOGo3mGNtQMeCNNvHpNnagbffNWnxEEPkcnMnkB9hzYDTJBBUJnxoFemoP0hqh2CpEr8Oxnw6JzBTDXInHhQr8NU492Y%2BCcawtU0bOrAP2DHkDV0T%2BoFiqqhopKUW2ifAX%2F1ZdfFDIP3O22MMLnDB6p28gCnczKNtErpm1CJDcwBxuRgw0Fes2w7IAWzq3Ewb0u72fzZOembEeII9NAHny0CxclqgIOx6eEZb%2F4S9KrDHfIy%2BXpQZ9ki3CJ6Y6i17Cvwj4fK3V%2FCw96%2BM1%2B%2FvI0DqOV8c9v%2FzHm35YP7pVaeOaFx50NLdznfEyg0K535o1vsTngFNQYyOVIYjfcpqIj1DxoPKLCdG%2FhJ5M5wld8GSC3c3GXKo1RfXEGD%2B5i4YdLJJS2BSoF%2B4dvpYKz%2Bt550dpLMHu62bBnWSTtRwbwR%2FZs%2BWVprMAdO9R773kDH%2BSz58K5p14ejUyyF4M4B9t76d7I3XqzfeuuknWQtdzryk%2B82cado1uvcBQK09IelQ%2FYABqj%2BEHgRAE0XqSvveC%2FQrJ0B47tDPgDasC%2BwUr%2F4K79AI1op958B83xh5fdyAawsP%2FDrPMBn1pclPLLen9%2B5%2FPLy9ZLvsF7OpH2r0raKNjFmYFkQvDJ%2FQTVxSwMpNRmT26w87p9e9z%2BBOj%2BVO437D5Y6Uzydde%2Fbtw38%2BdWgZJXK9rFuH1WSbKB%2BGwghDTQ%2B4T%2FIIHt9TKKloHnbvzt9Txa4xvzLRb98JI%2BLXr%2B8vMejD2r8nyIYqd6r%2FvJE9XIJiGybLQMT3%2Fml9l1NxgmHu%2FnRl4%2FymcBFmJgiVdt4lUrb4oLTpoEJwlOfHDSc9cuQyegmV2i00HTgrnZiycDmd4VHilIuGoTrrp4dVz00iV6SfTio5dl632il3gxgQVI%2BeSEhKQ2IenA98FFGUOizO%2BMMqYlBBnoeFdRxuoUZOrWGFmwgqc6Jaa0iSnM5ueiiClRpMP3dV6QoRtCyIAucnXSR9X1LjFDvLGAhSAT6ZS0DCAHvgsunFgSTn5np8QWOyWqahETN%2FmWvW4Q5tCdRiycyRdcJda0iTWN3goXcWyJOL8z4hwyDNIMq4I4WqcuTe02RBa64F0cElpaXU1ntj8XR4YSR%2BRAiDN30rGbUt57TKyeVnYcs4AEa0ogaRNIGr0S%2Ft41RYLLbwMuv%2BKRALXTmdnK2QMWgpD3CASB5fqbLWqVkhm62016tuvFf4O9pc4G6yz1N7FLHrjQr4SPIDXbX38BQeRLvHBwUfN14zK65Gk5upj5Ok%2FhunS6cU90OqkOcLx0J3FxAgRt0su3GzLwZ7EsLqM4WUXLKHSD233qCG9yRwaMzQM29A98gV9s4sbJDTp1ChNCRNrayAsXecoz2uSdJmXmZtcZcnZkguwgyJT%2B3q03ee3ceJ6nZLYLqh2ubNbFIHvkzr%2Bn%2B%2FRLJc7xn6iL1PUp783Htq9mv5HNK9eaiZ8SvvpvWcvgi%2FQmejLYTksvefBiHxqIF6Mun%2B5y1wV4dpAR8CEO0BCHXnqpbzTAvLzHsuQAjY3v08D0CzKwzrB24b24uyD5WN8O6VHzzLzTCnIsR6tH6diDho5LK5lrHeBi1Ww9pRDYRD58slLODyihMhytTmFpwzJ4C%2BXBsHIGHf5Ia7CH%2FuJRjmQD5kFTFg3MvhYskG82kkzQfUfl9FNgHsMEfAPgU0DNvu%2FuKECTFHB%2BlnURFFDeetcbBRhDrQrpps2lAFJeU7jipjVsIm6b5OEgrriqmHojed0AjeRNw2wkb1t2E3mgKI3qT8kP9ZPybxqo4QAKTg8gSALuHCaNrkZiQgPgU3DN4QVJwRdGwUfZ1kUwcNlKO2DgyqRYW8jMjn%2FDnCGDYFwgM94yffbAnEUEuWBg5vmvOjgGmQUGwMflmuMe3eGyfo643IVpnc%2FQSGxaFwHMZrfAXO9tDwnnXzQ0IuVVU2s2ehEqkMMXQqFD%2F58RJe0giplcwtzb5TNMZ64%2F793z2aXmGJBklwtjl%2Ffv9ZdPl%2FRHLuRq%2BpA%2F9UPJW30tvdQGuGQBPxxO9LnugsL1FXfQxQOepkBR1tLofWhDydsY3shk2unVWSjBuj0tF0IWmsrp1pp9DF2w7YVPFjWHvLojC7NdsnhHdtjz2OU4O7wIfjnJuo5qkWMFgz96oRRMwJcfEsFGmsqrino6BmPPjtUyWDXkgGSwc2CwIW%2FaY9gygx00mVZzaFAy2LtnsOPsUDIYm8EscrZLwDCUgqLzFYbkBFxzhcqGue45rCbUfyMiA5LIzo3IdIsDIKbaApGxjIbPZjVHVyWbvXc2O9IYJZux2WxIspkhYDNSQTcbklNjhXQO5oRsVqUnIZtpks3Ojc0M3nqBddRGB4HR8L9LUHNWWrLZe2ezI41RslkDNuOfHKIVbMF0JDVd2FgBVAi2QzYjP3%2FG4rCUtwoay2OAnf3eCA4QNIamC6ExvPTQzexirbXwyavmmH535GW0S17vyAR7nlg8zgQvgrzKM%2Bb9nXqFAx9iFq%2ByEiVWMPsaJ6VfrjxgSIQjvsmNFudHJCZ3d%2B5RRLI3Dj57ADn0OQe765c9jrS7i2CP8gz1CZelrIbLUkDva9Go%2FL1kFotA5ihYBMufhkVQiaCL3pwZ3CVziM5bYj5qazdlIHwm6TWQwrA3Jrks2%2Bt5Qego27sIGqnE0byUY5%2F8L9uzwH4y7e3g57vpbz1PPHEXYI%2FB%2BgMsho%2F%2BvZ7htyT6nwH6H2eNF4H%2BJzlaqmo6EXbHEKyfUApAsFXNIELpNFbIv6jSwywXI36wR4TzxHKnWy3JUt4d5XCnCmxDQCuHRRoGvQYaAO2Sx9nYxRlNIWG7uAiArwS5Pl1oTUHwAFJe1%2FlrElpDeX1oNZInA7eJ5MnIbSJ5KviBUAHozZ6ACt4mVCCjt4kU4EM0e%2Bh9GP9DFXRNa6ZgmA0VmrYSpaBZlW2GJ%2FITSsHmpJvQCR3whgJWS15CrwEj%2Bg0T9169BKFZSCeB6SRQwVRV%2FlISKa8r%2FFEjReICeYrEBfI0iYsUKBIXKZAhj4QKFIkLFGgSFylQJC5SoEhcpECRuEjB1MzjFPJPB514sF8OGyVpvHe8Nlui8V5DefQb9%2Bl3pHFT0nijQIEiGqfkLYsrT4c5FymQNCtUoAKjixToUSBfgaJZoQJJs0IFkmaFCiTNChVMteFDUwrVD6yciGaJcwiSaDtBVJMX2gi0xLS9hhzp92TAe2VasV1IqmVPq1N791UR75AKRsM5WqECSSNCBZIVGisA8xymXMvnDySHdOOV87BCb4lDLjnQx9nYRc%2BjNaFdSA5hcggdGtESBMogFfLF1wbfgeIr0KMjkQI12BEpUIMdoQJBnL%2Bg0Fd0eh5LTeRAp2uHFnDAyGiHpLRe43e0vPn0bOyi54GO0C4kSbFJitrwaQt2r5AKmibY8EnN%2BYkUqDk%2FkQJFgwIFes5PpEDRoEiBokGhAslqjRXAOeyjTYONnBkVioGMOpmr4L9zoT7u6SiV%2FtofaIcMe40HYp%2BCDDuwjDM6N8ewDEmH7Hk%2FcsOCKVp5IeSBXtl%2FeyIU%2Fng7lhDc3%2FeJar632hICgz4RWFUkBPdjGRKB2QhM%2BrVEGIsDFOy2A%2FLBpDiKknIOsbtZ3UcLBMa3%2Fwc%3D%3C%2Fdiagram%3E%3C%2Fmxfile%3E)

```text
[ 2S LiPo (7.4V / 8.4V Peak) ] ──► [ SPST Master Switch ] (Rule 2)
                                           │
         ┌─────────────────────────────────┴─────────────────────────────────┐
         ▼ (VBAT: 7.4V Unregulated)                                          ▼
┌─────────────────────────┐                                ┌───────────────────────────────────┐
│ TB6612FNG (VM Pin)      │                                │ MP1584 Buck Converter (7.4V ➔ 5V) │
│ └─► Left & Right Motors │                                └─┬─────────────────────────────────┘
└─────────────────────────┘                                  │ (Clean 5.0V)
                                             ┌───────────────┴───────────────┐
                                             ▼                               ▼
                                    ┌─────────────────┐             ┌─────────────────┐
                                    │ ESP32 (VIN Pin) │             │ TCRT5000 IR Array│
                                    └────────┬────────┘             └─────────────────┘
                                             │
                       ┌─────────────────────┴─────────────────────┐
                       ▼ (3.3V Logic)                              ▼ (GPIO Output)
              [ TB6612FNG Control ]                    [ 220Ω Resistor + Red LED ]
              (PWM, Direction pins)                    (Rule 3: End-Zone Indicator)

* Common Ground (GND) is shared across all modules.
```
---

## 📓 Handwritten Notes

| Undestanding Basics of BOM |
|:---:|
| ![Note 1](../../docs/Handwritten%20Notes/Task4/image1.png) |

---

## 🤖 Credits

- 🛠️ **Hardware Architecture & BOM:** Aman Kumar
- 📊 **Spreadsheet:** [Live Google Sheet](https://docs.google.com/spreadsheets/d/1ojaPZ_JZtjW4_C1dWolM55V-_D-JKaqS8tzH1k0BkUU/edit?gid=0#gid=0)
- ⚡ **Interactive Power Diagram:** [Draw.io Live Power Block Diagram](https://viewer.diagrams.net/?tags=%7B%7D&lightbox=1&highlight=0000ff&edit=_blank&layers=1&nav=1&title=PowerBlockDiagram&dark=auto#R%3Cmxfile%3E%3Cdiagram%20name%3D%22Page-1%22%20id%3D%22aDdpTUx8y8Sh7_ZTIIzu%22%3E7V1tc5s6Fv41nrn3QzIgXv0xxmnWc5s2a6dpd7%2FsEJvY3GLwYtyk%2B%2BtXEi8GCUmmBmynytzpNeIcSYij5zl6Oww0Z%2F12F7ub1X208IIBUBZvA208AEC1LBP%2BD6X8TFOGyjBNWMb%2BIhPaJ8z8%2F3lZopKl7vyFt60IJlEUJP6mmjiPwtCbJ5U0N46j16rYSxRUS924S49KmM3dgE796i%2BSVZpqA2uf%2Bg%2FPX67yklUze761mwtnT7JduYvotZSk3Q40J46iJP21fnO8ADVe3i6p3gfG3aJisRcmNQpftl78%2Bflv1CZACdxn%2BF6wUFY7L167%2FmLsJm6WbI0GAL4rDTa8Bn8q%2BD%2FzvztUvdECyRVXA%2B2mcvcliF7nKzdGJX2cDgwnTPVHN48DYwSv96JghmT8hwjddpPEi3%2FCmwG69RyjugNziS6sa%2F0JinyK1n7oBjg7eE%2Bx0%2BQHz%2F1eq6YairK%2BWVWKHBjjfZVmX8kazR5mj%2FDGvbuFlYE%2FptH8O%2F4xe%2FWT%2Baq2mOkOmgdQUEM60XoT%2BG4499iFjr44f5HF3vuhj4p9UA1bv%2F2EpHbz7zjD8IcX47rUlDxLvM3VOHoNcT%2BA%2FxjXCmqRqbfcBW7iLdi1GE8nT7dTsh6PI9NUwYdPd0hih5v6Pkoi1ADj2Ic1YWd4O3sgc4NJGmqVsffjLx%2BZw5PK1p9QlTGunJUL%2BzGqxaMzfYRvMxMEyk0cuz%2FZmd1%2Ffvw8nZEZfvRektRU3fUmtesp7qzQtFI19KxbdrbTWypP1M%2Bc28HNEDf71t%2Bi1mJm8PF2TGYwhW8pvcGxLQ01cLi4%2BncUcizr7tP4jz8qt6E9rqMwvVW58eefWK3SOYFydZWXO7Ac3OluwcBWBkMt62pfwrgwLcvJelCpNxFZFH1aCfzQy4306T7VzU2wqf4E9o9RmkXalyo9i8iC6hFA%2BfzlscgAW21D%2FSfHSZUn02ojosyg%2BnVZX7vWMEZ9vU9xEv64qX%2F8A7VHjbXHfgxx34d2gPOYfFKPzwLUZ5FdE22YQ0i0Sza7ZJuq5l1U1IB3D5PPqQbufeWuSIriTqS4IXQ7Ug3c3UoAQyrcjKHUB1wCZGQv3NLVzA2E6ilXqG6wV2GptHtV7ZgtUNgZW6TUkGyhrLk4EhNRFtlb4Aul7cqQKKRgF%2F0%2BS35ivILXDnRJHOzpJHH03cOGo3mGNtQMeCNNvHpNnagbffNWnxEEPkcnMnkB9hzYDTJBBUJnxoFemoP0hqh2CpEr8Oxnw6JzBTDXInHhQr8NU492Y%2BCcawtU0bOrAP2DHkDV0T%2BoFiqqhopKUW2ifAX%2F1ZdfFDIP3O22MMLnDB6p28gCnczKNtErpm1CJDcwBxuRgw0Fes2w7IAWzq3Ewb0u72fzZOembEeII9NAHny0CxclqgIOx6eEZb%2F4S9KrDHfIy%2BXpQZ9ki3CJ6Y6i17Cvwj4fK3V%2FCw96%2BM1%2B%2FvI0DqOV8c9v%2FzHm35YP7pVaeOaFx50NLdznfEyg0K535o1vsTngFNQYyOVIYjfcpqIj1DxoPKLCdG%2FhJ5M5wld8GSC3c3GXKo1RfXEGD%2B5i4YdLJJS2BSoF%2B4dvpYKz%2Bt550dpLMHu62bBnWSTtRwbwR%2FZs%2BWVprMAdO9R773kDH%2BSz58K5p14ejUyyF4M4B9t76d7I3XqzfeuuknWQtdzryk%2B82cado1uvcBQK09IelQ%2FYABqj%2BEHgRAE0XqSvveC%2FQrJ0B47tDPgDasC%2BwUr%2F4K79AI1op958B83xh5fdyAawsP%2FDrPMBn1pclPLLen9%2B5%2FPLy9ZLvsF7OpH2r0raKNjFmYFkQvDJ%2FQTVxSwMpNRmT26w87p9e9z%2BBOj%2BVO437D5Y6Uzydde%2Fbtw38%2BdWgZJXK9rFuH1WSbKB%2BGwghDTQ%2B4T%2FIIHt9TKKloHnbvzt9Txa4xvzLRb98JI%2BLXr%2B8vMejD2r8nyIYqd6r%2FvJE9XIJiGybLQMT3%2Fml9l1NxgmHu%2FnRl4%2FymcBFmJgiVdt4lUrb4oLTpoEJwlOfHDSc9cuQyegmV2i00HTgrnZiycDmd4VHilIuGoTrrp4dVz00iV6SfTio5dl632il3gxgQVI%2BeSEhKQ2IenA98FFGUOizO%2BMMqYlBBnoeFdRxuoUZOrWGFmwgqc6Jaa0iSnM5ueiiClRpMP3dV6QoRtCyIAucnXSR9X1LjFDvLGAhSAT6ZS0DCAHvgsunFgSTn5np8QWOyWqahETN%2FmWvW4Q5tCdRiycyRdcJda0iTWN3goXcWyJOL8z4hwyDNIMq4I4WqcuTe02RBa64F0cElpaXU1ntj8XR4YSR%2BRAiDN30rGbUt57TKyeVnYcs4AEa0ogaRNIGr0S%2Ft41RYLLbwMuv%2BKRALXTmdnK2QMWgpD3CASB5fqbLWqVkhm62016tuvFf4O9pc4G6yz1N7FLHrjQr4SPIDXbX38BQeRLvHBwUfN14zK65Gk5upj5Ok%2FhunS6cU90OqkOcLx0J3FxAgRt0su3GzLwZ7EsLqM4WUXLKHSD233qCG9yRwaMzQM29A98gV9s4sbJDTp1ChNCRNrayAsXecoz2uSdJmXmZtcZcnZkguwgyJT%2B3q03ee3ceJ6nZLYLqh2ubNbFIHvkzr%2Bn%2B%2FRLJc7xn6iL1PUp783Htq9mv5HNK9eaiZ8SvvpvWcvgi%2FQmejLYTksvefBiHxqIF6Mun%2B5y1wV4dpAR8CEO0BCHXnqpbzTAvLzHsuQAjY3v08D0CzKwzrB24b24uyD5WN8O6VHzzLzTCnIsR6tH6diDho5LK5lrHeBi1Ww9pRDYRD58slLODyihMhytTmFpwzJ4C%2BXBsHIGHf5Ia7CH%2FuJRjmQD5kFTFg3MvhYskG82kkzQfUfl9FNgHsMEfAPgU0DNvu%2FuKECTFHB%2BlnURFFDeetcbBRhDrQrpps2lAFJeU7jipjVsIm6b5OEgrriqmHojed0AjeRNw2wkb1t2E3mgKI3qT8kP9ZPybxqo4QAKTg8gSALuHCaNrkZiQgPgU3DN4QVJwRdGwUfZ1kUwcNlKO2DgyqRYW8jMjn%2FDnCGDYFwgM94yffbAnEUEuWBg5vmvOjgGmQUGwMflmuMe3eGyfo643IVpnc%2FQSGxaFwHMZrfAXO9tDwnnXzQ0IuVVU2s2ehEqkMMXQqFD%2F58RJe0giplcwtzb5TNMZ64%2F793z2aXmGJBklwtjl%2Ffv9ZdPl%2FRHLuRq%2BpA%2F9UPJW30tvdQGuGQBPxxO9LnugsL1FXfQxQOepkBR1tLofWhDydsY3shk2unVWSjBuj0tF0IWmsrp1pp9DF2w7YVPFjWHvLojC7NdsnhHdtjz2OU4O7wIfjnJuo5qkWMFgz96oRRMwJcfEsFGmsqrino6BmPPjtUyWDXkgGSwc2CwIW%2FaY9gygx00mVZzaFAy2LtnsOPsUDIYm8EscrZLwDCUgqLzFYbkBFxzhcqGue45rCbUfyMiA5LIzo3IdIsDIKbaApGxjIbPZjVHVyWbvXc2O9IYJZux2WxIspkhYDNSQTcbklNjhXQO5oRsVqUnIZtpks3Ojc0M3nqBddRGB4HR8L9LUHNWWrLZe2ezI41RslkDNuOfHKIVbMF0JDVd2FgBVAi2QzYjP3%2FG4rCUtwoay2OAnf3eCA4QNIamC6ExvPTQzexirbXwyavmmH535GW0S17vyAR7nlg8zgQvgrzKM%2Bb9nXqFAx9iFq%2ByEiVWMPsaJ6VfrjxgSIQjvsmNFudHJCZ3d%2B5RRLI3Dj57ADn0OQe765c9jrS7i2CP8gz1CZelrIbLUkDva9Go%2FL1kFotA5ihYBMufhkVQiaCL3pwZ3CVziM5bYj5qazdlIHwm6TWQwrA3Jrks2%2Bt5Qego27sIGqnE0byUY5%2F8L9uzwH4y7e3g57vpbz1PPHEXYI%2FB%2BgMsho%2F%2BvZ7htyT6nwH6H2eNF4H%2BJzlaqmo6EXbHEKyfUApAsFXNIELpNFbIv6jSwywXI36wR4TzxHKnWy3JUt4d5XCnCmxDQCuHRRoGvQYaAO2Sx9nYxRlNIWG7uAiArwS5Pl1oTUHwAFJe1%2FlrElpDeX1oNZInA7eJ5MnIbSJ5KviBUAHozZ6ACt4mVCCjt4kU4EM0e%2Bh9GP9DFXRNa6ZgmA0VmrYSpaBZlW2GJ%2FITSsHmpJvQCR3whgJWS15CrwEj%2Bg0T9169BKFZSCeB6SRQwVRV%2FlISKa8r%2FFEjReICeYrEBfI0iYsUKBIXKZAhj4QKFIkLFGgSFylQJC5SoEhcpECRuEjB1MzjFPJPB514sF8OGyVpvHe8Nlui8V5DefQb9%2Bl3pHFT0nijQIEiGqfkLYsrT4c5FymQNCtUoAKjixToUSBfgaJZoQJJs0IFkmaFCiTNChVMteFDUwrVD6yciGaJcwiSaDtBVJMX2gi0xLS9hhzp92TAe2VasV1IqmVPq1N791UR75AKRsM5WqECSSNCBZIVGisA8xymXMvnDySHdOOV87BCb4lDLjnQx9nYRc%2BjNaFdSA5hcggdGtESBMogFfLF1wbfgeIr0KMjkQI12BEpUIMdoQJBnL%2Bg0Fd0eh5LTeRAp2uHFnDAyGiHpLRe43e0vPn0bOyi54GO0C4kSbFJitrwaQt2r5AKmibY8EnN%2BYkUqDk%2FkQJFgwIFes5PpEDRoEiBokGhAslqjRXAOeyjTYONnBkVioGMOpmr4L9zoT7u6SiV%2FtofaIcMe40HYp%2BCDDuwjDM6N8ewDEmH7Hk%2FcsOCKVp5IeSBXtl%2FeyIU%2Fng7lhDc3%2FeJar632hICgz4RWFUkBPdjGRKB2QhM%2BrVEGIsDFOy2A%2FLBpDiKknIOsbtZ3UcLBMa3%2Fwc%3D%3C%2Fdiagram%3E%3C%2Fmxfile%3E)
- 📝 **Documentation:** Prepared with assistance from Gemini
- 🔍 **Verification:** Verified against Toshiba TB6612FNG, ST L298N, and Espressif datasheets