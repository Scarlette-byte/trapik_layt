/*
 * =====================================================
 * ESP32 Traffic Light Simulation with Firebase
 * =====================================================
 * Hardware:
 *   - Red LED    → GPIO 23
 *   - Yellow LED → GPIO 22
 *   - Green LED  → GPIO 21
 *   - All LEDs with 220Ω resistors to GND
 *
 * Libraries Required (install via Arduino Library Manager):
 *   - Firebase ESP Client by Mobizt
 *   - ArduinoJson
 * =====================================================
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

// ─── WiFi Credentials ──────────────────────────────
#define WIFI_SSID     "K3NT0Z4K1"
#define WIFI_PASSWORD "12345678"

// ─── Firebase Credentials ──────────────────────────
#define FIREBASE_HOST "traffic-sim-5a9ef-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "7etDsjZimq0GkGepNOJ61hIY9HYvruznDSW0A0kZ"

// ─── LED Pin Definitions ───────────────────────────
#define RED_PIN    4
#define YELLOW_PIN 5
#define GREEN_PIN  18

// ─── Traffic Light Timing (milliseconds) ───────────
#define GREEN_DURATION  5000
#define YELLOW_DURATION 2000
#define RED_DURATION    5000
#define BLINK_INTERVAL  300

// ─── Firebase Objects ───────────────────────────────
FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

// ─── State Variables ───────────────────────────────
enum TrafficState { STATE_RED, STATE_YELLOW, STATE_GREEN, STATE_OFF, STATE_BLINK };

TrafficState currentState   = STATE_RED;
int          currentMode    = 1;     // 0=OFF, 1=NORMAL, 2=BLINK
int          countdown      = 5;
bool         blinkOn        = false;
unsigned long lastUpdate    = 0;
unsigned long lastFirebase  = 0;
unsigned long lastBlink     = 0;
unsigned long phaseStart    = 0;

// ─── Helper: All LEDs OFF ──────────────────────────
void allOff() {
  digitalWrite(RED_PIN,    LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN,  LOW);
}

// ─── Helper: Set specific LEDs ─────────────────────
void setLight(int r, int y, int g) {
  digitalWrite(RED_PIN,    r ? HIGH : LOW);
  digitalWrite(YELLOW_PIN, y ? HIGH : LOW);
  digitalWrite(GREEN_PIN,  g ? HIGH : LOW);
}

// ─── Send State to Firebase ────────────────────────
void sendStateToFirebase(const char* state, int cdSecs) {
  if (millis() - lastFirebase < 250) return; // throttle to 4x/sec
  lastFirebase = millis();

  FirebaseJson json;
  json.set("trafficLightState", state);
  json.set("mode",              currentMode);
  json.set("countdown",         cdSecs);
  json.set("timestamp",         (int)(millis() / 1000));

  Firebase.setJSON(fbData, "/trafficLight", json);
}

// ─── Read Mode Command from Firebase ───────────────
void checkFirebaseMode() {
  if (Firebase.getInt(fbData, "/trafficLight/mode")) {
    int newMode = fbData.intData();
    if (newMode != currentMode) {
      currentMode = newMode;
      Serial.printf("Mode changed to: %d\n", currentMode);
      phaseStart = millis();

      if (currentMode == 0) {
        allOff();
        currentState = STATE_OFF;
      } else if (currentMode == 2) {
        currentState = STATE_BLINK;
      } else {
        currentState = STATE_GREEN;
      }
    }
  }
}

// ─── Normal Traffic Cycle ──────────────────────────
void runNormalCycle() {
  unsigned long elapsed = millis() - phaseStart;

  switch (currentState) {
    case STATE_GREEN:
  setLight(0, 0, 1);
  countdown = max(0L, (long)(GREEN_DURATION - (long)elapsed) / 1000L);
  sendStateToFirebase("GREEN", countdown);
  if (elapsed >= GREEN_DURATION) {
    currentState = STATE_YELLOW;
    phaseStart = millis();
    Serial.println("→ YELLOW");
  }
  break;

case STATE_YELLOW:
  setLight(0, 1, 0);
  countdown = max(0L, (long)(YELLOW_DURATION - (long)elapsed) / 1000L);
  sendStateToFirebase("YELLOW", countdown);
  if (elapsed >= YELLOW_DURATION) {
    currentState = STATE_RED;
    phaseStart = millis();
    Serial.println("→ RED");
  }
  break;

case STATE_RED:
  setLight(1, 0, 0);
  countdown = max(0L, (long)(RED_DURATION - (long)elapsed) / 1000L);
  sendStateToFirebase("RED", countdown);
  if (elapsed >= RED_DURATION) {
    currentState = STATE_GREEN;
    phaseStart = millis();
    Serial.println("→ GREEN");
  }
  break;
    default:
      currentState = STATE_GREEN;
      phaseStart = millis();
      break;
  }
}

// ─── Blink Mode ────────────────────────────────────
void runBlinkMode() {
  if (millis() - lastBlink >= BLINK_INTERVAL) {
    lastBlink = millis();
    blinkOn = !blinkOn;
    setLight(blinkOn, blinkOn, blinkOn);
  }
  sendStateToFirebase("BLINK", 0);
}

// ─── Setup ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN,    OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN,  OUTPUT);
  allOff();

  // Connect to WiFi
  Serial.printf("\nConnecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());

  // Connect to Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Write initial mode to Firebase
  Firebase.setInt(fbData, "/trafficLight/mode", 1);
  Serial.println("Firebase connected. Starting cycle...");

  phaseStart = millis();
  currentState = STATE_GREEN;
}

// ─── Main Loop ─────────────────────────────────────
void loop() {
  // Poll Firebase for mode changes every 500ms
  if (millis() - lastUpdate >= 500) {
    lastUpdate = millis();
    checkFirebaseMode();
  }

  // Execute current mode
  switch (currentMode) {
    case 0: // OFF
      allOff();
      sendStateToFirebase("OFF", 0);
      delay(1000);
      break;

    case 1: // Normal cycle
      runNormalCycle();
      break;

    case 2: // Blink all
      runBlinkMode();
      break;
  }

  delay(50);
}