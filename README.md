# 🌿 Plant Monitor — IoT Dashboard

Real-time plant health monitoring with **Arduino R4**. Track soil moisture — your plant's face reacts to how it's feeling with custom expressions.

**Hardware used:** Arduino R4 · YL-38 Soil Moisture Sensor · MP3 TF 1CP · Speaker

---

## 📑 Table of Contents

1. [How It Works](#1-how-it-works)
2. [Components](#2-components)
3. [Step 1 — Wire the Hardware](#3-step-1--wire-the-hardware)
4. [Step 2 — Set Up Arduino](#4-step-2--set-up-arduino)
5. [Step 3 — Upload the Code](#5-step-3--upload-the-code)
6. [Step 4 — Open the Dashboard](#6-step-4--open-the-dashboard)
7. [Step 5 — Connect Arduino to Dashboard](#7-step-5--connect-arduino-to-dashboard)
8. [What You See on the Dashboard](#8-what-you-see-on-the-dashboard)
9. [Plant Profile & Expressions](#9-plant-profile--expressions)
10. [Troubleshooting](#10-troubleshooting)
11. [Deploy & Share](#11-deploy--share)

---

## 1. How It Works

```
  ┌──────────────┐       ┌──────────────┐
  │  YL-38       │       │  MP3 TF 1CP  │
  │  SOIL SENSOR │       │  + SPEAKER   │
  │              │       │              │
  │ Soil         │       │ Voice alerts │
  │ Moisture     │       │ when plant   │
  │              │       │ needs help   │
  └──────┬───────┘       └──────┬───────┘
         │ A0                   │ D10/D11
         │                      │
         └──────────────────────┼───────────────────┘
                                │
                    ┌───────────▼───────────┐
                    │     ARDUINO R4        │
                    │                       │
                    │  • Reads YL-38 sensor │
                    │  • Sends JSON serial  │
                    │  • Triggers MP3       │
                    └───────────┬───────────┘
                                │ USB
                    ┌───────────▼───────────┐
                    │   WEB DASHBOARD       │
                    │                       │
                    │  • Live gauge          │
                    │  • Plant expressions  │
                    │  • Alerts & tips      │
                    │  • Profile editor     │
                    └───────────────────────┘
```

**In plain English:**
1. The **YL-38 soil sensor** checks if the plant needs water
2. The **Arduino** sends the reading to your computer every 5 seconds
3. The **dashboard** shows live data and your plant's face reacts to the readings
4. If the soil is too dry, the **speaker plays a voice alert** ("Water me!")

---

## 2. Components

| # | Component | What It Does | You Have? |
|---|-----------|-------------|-----------|
| 1 | **Arduino UNO R4** | Brain — reads sensors, sends data | ✅ |
| 2 | **YL-38 Soil Moisture Sensor** | Measures water in soil (with YL-69 probe) | ✅ |
| 3 | **MP3 TF 1CP Module** | Plays voice alerts through speaker | ✅ |
| 4 | **Speaker (8Ω)** | Audio output | ✅ |
| 5 | **Breadboard** | Prototyping | ✅ |
| 6 | **Jumper Wires** | Connections | ✅ |
| 7 | **USB Cable (Type-C)** | Power + data to PC | ✅ |
| 8 | **MicroSD Card (≤32GB)** | Stores MP3 files for alerts | ✅ |

> **The YL-38 module** includes the YL-69 probe that goes into the soil. It gives you an analog reading proportional to soil moisture — that's everything the dashboard shows.

---

## 3. Step 1 — Wire the Hardware

### 3A. YL-38 Soil Moisture Sensor

```
  YL-38 Module                 Arduino R4
  ┌──────────────┐             ┌──────────────┐
  │   VCC ───────┼─────────────┼── 5V         │
  │   GND ───────┼─────────────┼── GND        │
  │   AOUT ──────┼─────────────┼── A0         │
  └──────────────┘             └──────────────┘

  YL-69 Probe connects to YL-38 module via the 2-pin header.
```

### 3B. MP3 TF 1CP + Speaker

```
  MP3 TF 1CP Module            Arduino R4
  ┌──────────────┐             ┌──────────────┐
  │   VCC ───────┼─────────────┼── 5V         │
  │   GND ───────┼─────────────┼── GND        │
  │   RX ────────┼─────────────┼── D11 (TX)   │  ← crossed
  │   TX ────────┼─────────────┼── D10 (RX)   │  ← crossed
  │   SPK+ ──────┼─────────────┼── Speaker +  │
  │   SPK- ──────┼─────────────┼── Speaker -  │
  └──────────────┘             └──────────────┘
```

### 3C. Prepare MicroSD Card

| File | Alert | When |
|------|-------|------|
| `0001.mp3` | "Water me please!" | Soil < 20% |

Format as **FAT32**, files at root level.

### 3D. Wiring Summary

```
Arduino Pin    →    Component
──────────────────────────────
  5V           →    YL-38 VCC, MP3 VCC
  GND          →    YL-38 GND, MP3 GND
  A0           →    YL-38 AOUT
  D10          →    MP3 TX
  D11          →    MP3 RX
  Speaker      →    MP3 SPK+ / SPK-
```

---

## 4. Step 2 — Set Up Arduino

### 4A. Install Arduino IDE
Download from [arduino.cc/en/software](https://www.arduino.cc/en/software) (version 2.x)

### 4B. Add R4 Board Support
**Tools → Board → Boards Manager → Search "Arduino UNO R4 Boards" → Install**

### 4C. Install Libraries
**Sketch → Include Library → Manage Libraries:**
- `DFRobotDFPlayerMini`

---

## 5. Step 3 — Upload the Code

```cpp
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

#define SOIL_PIN  A0
#define MP3_RX    10
#define MP3_TX    11

SoftwareSerial mp3Serial(MP3_RX, MP3_TX);
DFRobotDFPlayerMini mp3;

unsigned long lastSend = 0, lastAlert = 0;

void setup() {
  Serial.begin(9600);
  mp3Serial.begin(9600);
  if (mp3.begin(mp3Serial)) mp3.volume(25);
  Serial.println("{\"status\":\"ready\"}");
}

void loop() {
  int soil = constrain(map(analogRead(SOIL_PIN), 1023, 300, 0, 100), 0, 100);

  if (millis() - lastSend > 5000) {
    Serial.print("{\"soil\":"); Serial.print(soil);
    Serial.println("}");
    lastSend = millis();
  }

  if (millis() - lastAlert > 60000) {
    if (soil < 20) { mp3.play(1); lastAlert = millis(); }
  }
  delay(1000);
}
```

**Upload:** Paste → Verify (✓) → Upload (→) → Open Serial Monitor at 9600 baud to verify JSON output.

> **Close Serial Monitor** before connecting the dashboard.

---

## 6. Step 4 — Open the Dashboard

1. Open `index.html` in **Chrome** or **Edge**
2. You'll see simulated data flowing in (for testing)
3. Your plant's face will react to the readings

**Browser requirement:** Chrome/Edge 89+. Firefox and Safari don't support Web Serial.

---

## 7. Step 5 — Connect Arduino to Dashboard

### Direct Connection (Web Serial API)
1. Connect Arduino via USB
2. Close Arduino Serial Monitor
3. Click **🔌 Connect Arduino** on the dashboard
4. Select your port → badge changes to "Live"
5. Real data flows in!

### Python Bridge (Network Access)
For viewing on phone or other devices on your network:

```bash
pip install pyserial websockets
```

See the `bridge.py` example in the code section, or use the WebSocket approach described in the Arduino code comments.

---

## 8. What You See on the Dashboard

### 🌱 Plant Expressions (Top Card)

Your plant has two faces that react based on soil moisture:

| Expression | When | What It Means |
|------------|------|---------------|
| 😊 **Happy** | Soil moisture in ideal range | Plant is thriving |
| 😢 **Sad** | Soil moisture out of range | Needs attention — water or let dry |

The expression switches **automatically** based on your configured thresholds.

### 💧 Soil Moisture

| Range | Status |
|-------|--------|
| Within thresholds | ✅ Ideal |
| Below minimum | ⚠️ Too Dry |
| Above maximum | ⚠️ Waterlogged |
| Below 20% | 🚨 Critical (voice alert) |

### 🔊 Speaker (MP3 Alert)

Voice track plays through the speaker when soil is critically dry:
- `0001.mp3` → "Water me!" (soil < 20%)

---

## 9. Plant Profile & Expressions

### Opening the Profile Editor

Click **⚙️ Profile** in the header, or **click the plant's name** on the dashboard.

### What You Can Customize

**Plant Identity**
- **Name** — shown in the hero card (e.g., "Basil", "Fern", "My Little Friend")
- **Species / Note** — subtitle text (e.g., "Ocimum basilicum • Kitchen Window")

**Expression Thresholds (Sliders)**

Drag the sliders to set when the sensor triggers an alert:

| Sensor | "Too Low" Slider | "Too High" Slider |
|--------|------------------|-------------------|
| 💧 Soil Moisture | Minimum % before "Too Dry" | Maximum % before "Too Wet" |

**How expressions are calculated:**
```
Soil in range  →  😊 Happy
Soil out of range  →  😢 Sad
```

### Custom Expression Images

The expression images are embedded directly in the HTML (base64). To use your own:

1. Open the HTML file in a text editor
2. Find the `<img>` tags with `id="expHappy"`, `id="expModerate"`, `id="expSad"`
3. Replace the base64 strings with your own images
4. Use a tool like [base64-image.de](https://www.base64-image.de/) to convert images to base64

**Tips:**
- Use square images (160×160px recommended for small file size)
- JPEG format keeps the file size small
- The images are stored as `data:image/jpeg;base64,...`

### Saving

All profile settings are saved to **browser localStorage** — they persist across page refreshes and browser restarts. No server needed.

---

## 10. Troubleshooting

### Arduino

| Problem | Fix |
|---------|-----|
| Soil always 0 or 100 | Check probe placement. Verify wiring to A0. Check YL-38 module LED. |
| MP3 not playing | Format MicroSD as FAT32. Files at root level. Check speaker wiring. |
| No serial output | Use data USB cable. Try different port. |
| Upload fails | Press reset before uploading. Try different cable. |

### Dashboard

| Problem | Fix |
|---------|-----|
| No port dialog | Use Chrome/Edge. Close Serial Monitor first. |
| Data not updating | Verify 9600 baud. Check Arduino sends JSON. |
| Expression not changing | Check thresholds in Profile editor. Hard refresh (Ctrl+Shift+R). |

### Debug Checklist
```
□ Arduino IDE shows correct port?
□ Serial Monitor shows JSON at 9600 baud?
□ Serial Monitor is CLOSED before connecting dashboard?
□ Using Chrome or Edge?
□ USB cable transfers data (not charge-only)?
□ YL-69 probe is inserted in soil?
□ MicroSD is FAT32 with files at root?
□ Speaker wires go to MP3 module (not Arduino)?
```

---

## 11. Deploy & Share

| Method | How |
|--------|-----|
| **Local file** | Double-click `index.html` |
| **Local server** | `python -m http.server 8080` |
| **GitHub Pages** | Push to repo → Settings → Pages |
| **Raspberry Pi** | Kiosk mode with Chromium on boot |

> Web Serial requires HTTPS or localhost.

---

## 📁 Project Files

```
plant-dashboard/
├── index.html              ← Dashboard (open in Chrome, everything is self-contained)
└── README.md               ← This guide
```

> Expression images are embedded directly in `index.html` as base64 — no external files needed.

---

## License

MIT — use it however you like. 🌱
