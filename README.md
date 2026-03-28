# =====================================================
# TrafficOS — Complete System Documentation
# =====================================================

## 📁 Firebase Realtime Database Structure

```json
{
  "trafficLight": {
    "trafficLightState": "GREEN",
    "mode": 1,
    "countdown": 4,
    "timestamp": 1712000000
  }
}
```

### Field Reference

| Field              | Type    | Values                          | Set by   |
|--------------------|---------|---------------------------------|----------|
| trafficLightState  | String  | "RED", "YELLOW", "GREEN", "OFF", "BLINK" | ESP32 |
| mode               | Integer | 0 = OFF, 1 = NORMAL, 2 = BLINK | Dashboard |
| countdown          | Integer | 0–5 seconds remaining           | ESP32    |
| timestamp          | Integer | Unix epoch (seconds)            | ESP32    |

---

## 🔧 Firebase Rules (paste into Firebase console)

```json
{
  "rules": {
    "trafficLight": {
      ".read":  true,
      ".write": true
    }
  }
}
```

> ⚠️ For production: replace with proper auth rules.

---

## ⚙️ ESP32 Arduino Setup

### Required Libraries (Library Manager)
- `Firebase ESP Client` by Mobizt (v4.x)
- `ArduinoJson` by Benoit Blanchon (v6.x)

### Wiring Diagram

```
ESP32              Component
─────              ─────────
GPIO 25 ──[220Ω]── Red LED ── GND
GPIO 26 ──[220Ω]── Yellow LED ── GND
GPIO 27 ──[220Ω]── Green LED ── GND
3.3V / 5V ── Breadboard power rail
GND ── Breadboard GND rail
```

### Breadboard Layout (Text Diagram)

```
 ┌─────────────────────────────┐
 │  ESP32                      │
 │  [GPIO25]─────R1(220Ω)──┐  │
 │  [GPIO26]─────R2(220Ω)──┼─ │  ← LED Anodes
 │  [GPIO27]─────R3(220Ω)──┘  │
 │  [GND]────────────────────  │  ← LED Cathodes (all to GND)
 └─────────────────────────────┘

 LEDs (from top to bottom on breadboard):
   🔴 Red LED    → R1 → GPIO25
   🟡 Yellow LED → R2 → GPIO26
   🟢 Green LED  → R3 → GPIO27
   All cathodes (–) → GND
```

---

## 🌐 Dashboard Setup

1. Open `dashboard.html` in a modern browser
2. On the config dialog, enter:
   - **Database URL**: `https://YOUR_PROJECT.firebaseio.com`
   - **API Key**: From Firebase Console → Project Settings → Web API Key
   - **Project ID**: From Firebase Console → Project Settings
3. Click **CONNECT TO FIREBASE**
4. Use **DEMO MODE** to test without hardware

### Dashboard Controls

| Button       | Firebase Write              | Effect                        |
|--------------|-----------------------------|-------------------------------|
| START SYSTEM | `/trafficLight/mode` = 1    | Normal RED→YELLOW→GREEN cycle |
| STOP SYSTEM  | `/trafficLight/mode` = 0    | All LEDs OFF                  |
| BLINK MODE   | `/trafficLight/mode` = 2    | All LEDs blink simultaneously |

---

## 🔄 Data Flow

```
┌──────────┐    WiFi     ┌──────────────┐    SDK     ┌───────────┐
│  ESP32   │ ──────────▶ │   Firebase   │ ─────────▶ │  Browser  │
│ (LEDs)   │             │  Realtime DB │            │ Dashboard │
│          │ ◀────────── │              │ ◀───────── │           │
│ Reads    │  mode cmd   │              │  mode cmd  │  Buttons  │
│ Writes   │  state/cd   │              │  listening │  Listens  │
└──────────┘             └──────────────┘            └───────────┘

Read path  (ESP32 → Firebase → Dashboard):
  ESP32 writes trafficLightState + countdown every ~250ms
  Dashboard listens via .on('value') realtime listener
  UI updates: bulbs, car animation, countdown ring, metrics

Write path (Dashboard → Firebase → ESP32):
  User clicks button → Firebase mode field updated
  ESP32 polls /trafficLight/mode every 500ms
  ESP32 changes behavior + updates state
```

---

## ⏱ Timing Reference

```
Normal Cycle (mode = 1):
  GREEN  ─────────────── 5 seconds
  YELLOW ──── 2 seconds
  RED    ─────────────── 5 seconds
  (repeat)

Blink Mode (mode = 2):
  All LEDs alternate ON/OFF every 300ms

OFF Mode (mode = 0):
  All LEDs remain OFF
```

---

## 📡 ESP32 Configuration Checklist

```
[ ] Replace WIFI_SSID with your network name
[ ] Replace WIFI_PASSWORD with your password
[ ] Replace FIREBASE_HOST with your DB URL (no https://)
    Example: myproject-default-rtdb.firebaseio.com
[ ] Replace FIREBASE_AUTH with your Database Secret
    (Firebase Console → Project Settings → Service Accounts → Database secrets)
[ ] Upload code via Arduino IDE (set board: ESP32 Dev Module)
[ ] Open Serial Monitor at 115200 baud to verify connection
```

---

## 🚀 Quick Start (5 minutes)

1. **Firebase**: Create project at console.firebase.google.com
   → Enable Realtime Database → Set rules to allow read/write
   → Copy Database URL and API Key

2. **ESP32**: Install libraries → Update credentials in .ino → Upload

3. **Dashboard**: Open dashboard.html → Enter Firebase config → Connect

4. **Test**: Click START → watch LEDs cycle and car animate!