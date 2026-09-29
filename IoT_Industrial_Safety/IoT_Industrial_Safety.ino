/*
  ================================================================================
  PROJECT: IoT-Based Multi-Sensor Industrial Safety & Machine Condition Monitoring
  TARGET BOARD: ESP32 DevKit
  FRAMEWORK: Arduino C++
  ================================================================================
*/

#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ThingSpeak.h>

// ================= CONFIGURATION =================
// --- Wi-Fi ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// --- ThingSpeak ---
unsigned long THINGSPEAK_CHANNEL_ID = 3513219;
const char* THINGSPEAK_WRITE_API_KEY = "YOUR_THINGSPEAK_WRITE_API_KEY";
const unsigned long THINGSPEAK_INTERVAL_MS = 20000; // 20 sec

// --- Sensor Logic (Active HIGH/LOW) ---
const bool FLAME_ACTIVE_LOW = true;
const bool VIBRATION_ACTIVE_LOW = true;
const bool TOUCH_ACTIVE_HIGH = true;
const bool RELAY_ACTIVE_LOW = true;

// --- Thresholds ---
const float TEMP_WARNING = 35.0;
const float TEMP_CRITICAL = 45.0;

int GAS_WARNING_THRESHOLD = 1500;  // Calibration needed based on module
int GAS_CRITICAL_THRESHOLD = 2500;

// --- Vibration Escalation ---
const int VIBRATION_WARNING_COUNT = 3;
const int VIBRATION_CRITICAL_COUNT = 10;
const unsigned long VIBRATION_WINDOW_MS = 10000; // 10 seconds window
const unsigned long VIBRATION_RESET_MS = 30000;  // Reset after 30 sec of quiet

// --- System ---
const unsigned long SENSOR_INTERVAL_MS = 1000;
#define DEBUG_MODE true

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
#define DHTPIN 4
#define DHTTYPE DHT11

#define PIN_FLAME 27
#define PIN_MQ 34
#define PIN_TILT 26
#define PIN_TOUCH 13
#define PIN_BUZZER 25
#define PIN_RELAY 23

// I2C Pins for LCD: SDA=21, SCL=22. 
LiquidCrystal_I2C* lcd = nullptr;
bool lcdConnected = false;

// ============================================================================
// SYSTEM STATES & EVENTS
// ============================================================================
enum SystemStatus {
  STATUS_NORMAL = 0,
  STATUS_WARNING = 1,
  STATUS_CRITICAL = 2
};

enum EventCode {
  EVENT_NONE = 0,
  EVENT_HIGH_TEMP = 1,
  EVENT_HIGH_GAS = 2,
  EVENT_FLAME = 3,
  EVENT_VIBRATION = 4,
  EVENT_TOUCH = 5,
  EVENT_MULTIPLE = 6
};

SystemStatus currentStatus = STATUS_NORMAL;
SystemStatus previousStatus = STATUS_NORMAL;

EventCode currentEventCode = EVENT_NONE;
String currentEventMessage = "NORMAL";
String previousEventMessage = "";

// Sensor Readings
float currentTemp = 0.0;
float currentHumidity = 0.0;
int currentGas = 0;
bool flameDetected = false;
bool touchDetected = false;

// Vibration Tracking
int vibrationCount = 0;
unsigned long firstVibrationTime = 0;
unsigned long lastVibrationTime = 0;
bool vibrationWarning = false;
bool vibrationCritical = false;

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastThingSpeakUpdate = 0;
unsigned long lastBuzzerToggle = 0;
unsigned long lastLCDRotate = 0;

bool buzzerState = false;
int lcdScreenState = 0; 
WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println(F("\n========================================"));
  Serial.println(F("IoT Industrial Safety System"));
  Serial.println(F("========================================"));
  Serial.println(F("ESP32 Initializing..."));

  // 1. GPIO
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_MQ, INPUT);
  pinMode(PIN_TILT, INPUT);
  pinMode(PIN_TOUCH, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_RELAY, RELAY_ACTIVE_LOW ? HIGH : LOW);

  // 2. DHT11
  dht.begin();
  Serial.println(F("DHT11: OK"));

  // 3. I2C & LCD Auto-Scan
  Wire.begin(21, 22);
  byte error, address;
  int foundAddress = 0;
  
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      if (address == 0x27 || address == 0x3F) {
        foundAddress = address;
        break;
      }
    }
  }

  if (foundAddress != 0) {
    lcd = new LiquidCrystal_I2C(foundAddress, 16, 2);
    lcd->init();
    lcd->backlight();
    lcdConnected = true;
    Serial.print(F("LCD: DETECTED at 0x"));
    Serial.println(foundAddress, HEX);
    lcd->setCursor(0,0);
    lcd->print("SYSTEM BOOTING");
  } else {
    Serial.println(F("LCD: NOT DETECTED"));
  }

  // 4. Wi-Fi
  Serial.println(F("WiFi: CONNECTING..."));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // We don't block forever
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 5000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWiFi CONNECTED"));
  } else {
    Serial.println(F("\nWiFi OFFLINE (Operating in Local Mode)"));
  }

  // 5. ThingSpeak
  ThingSpeak.begin(client);
  Serial.println(F("ThingSpeak: READY"));

  // 6. MQ-2 Baseline (Simple)
  Serial.println(F("MQ-2: Calibrating baseline..."));
  long gasSum = 0;
  for(int i=0; i<10; i++) {
    gasSum += analogRead(PIN_MQ);
    delay(100);
  }
  Serial.print(F("MQ-2 Baseline: "));
  Serial.println(gasSum / 10);
  
  Serial.println(F("========================================"));
  Serial.println(F("System Ready. Entering Monitoring Mode."));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  unsigned long currentMillis = millis();

  // 1. Polling Sensors
  if (currentMillis - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = currentMillis;
    readSensors();
    updateVibrationLogic(currentMillis);
    determineSystemStatus();
    handleEvents();
    updateLCD(currentMillis);
    if (DEBUG_MODE) printSerialStatus();
  }

  // 2. High-speed emergency polling (Flame & Touch)
  readFastSensors();
  if (flameDetected || touchDetected) {
     determineSystemStatus();
     handleEvents();
  }

  // 3. Actuators
  updateOutputs(currentMillis);

  // 4. Wi-Fi Maintenance
  maintainWiFi(currentMillis);

  // 5. Cloud Upload
  if (currentMillis - lastThingSpeakUpdate >= THINGSPEAK_INTERVAL_MS) {
    lastThingSpeakUpdate = currentMillis;
    sendThingSpeakData();
  }
}

// ============================================================================
// SENSOR READING
// ============================================================================
void readFastSensors() {
  bool rawFlame = digitalRead(PIN_FLAME);
  flameDetected = FLAME_ACTIVE_LOW ? !rawFlame : rawFlame;
  
  bool rawTouch = digitalRead(PIN_TOUCH);
  touchDetected = TOUCH_ACTIVE_HIGH ? rawTouch : !rawTouch;
}

void readSensors() {
  readFastSensors();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    if (DEBUG_MODE) Serial.println(F("[DHT ERROR] Invalid reading"));
  } else {
    currentHumidity = h;
    currentTemp = t;
  }
  
  currentGas = analogRead(PIN_MQ);
}

void updateVibrationLogic(unsigned long currentMillis) {
  bool rawTilt = digitalRead(PIN_TILT);
  bool tiltNow = VIBRATION_ACTIVE_LOW ? !rawTilt : rawTilt;
  
  if (tiltNow) {
    if (vibrationCount == 0) {
      firstVibrationTime = currentMillis;
    }
    lastVibrationTime = currentMillis;
    vibrationCount++;
  }

  // Escalation Evaluation
  if (vibrationCount > 0) {
    if (currentMillis - firstVibrationTime <= VIBRATION_WINDOW_MS) {
      if (vibrationCount >= VIBRATION_CRITICAL_COUNT) {
        vibrationCritical = true;
        vibrationWarning = false;
      } else if (vibrationCount >= VIBRATION_WARNING_COUNT) {
        vibrationWarning = true;
      }
    }
  }

  // Reset logic
  if (vibrationCount > 0 && (currentMillis - lastVibrationTime >= VIBRATION_RESET_MS)) {
    vibrationCount = 0;
    vibrationWarning = false;
    vibrationCritical = false;
    if (DEBUG_MODE) Serial.println(F("[VIBRATION] Cooldown passed. Logic reset."));
  }
}

// ============================================================================
// STATUS DETERMINATION
// ============================================================================
void determineSystemStatus() {
  previousStatus = currentStatus;
  
  // 1. CRITICAL
  if (touchDetected) {
    currentStatus = STATUS_CRITICAL;
    currentEventCode = EVENT_TOUCH;
    currentEventMessage = "MANUAL EMERGENCY";
  } else if (flameDetected) {
    currentStatus = STATUS_CRITICAL;
    currentEventCode = EVENT_FLAME;
    currentEventMessage = "FIRE ALERT";
  } else if (currentTemp >= TEMP_CRITICAL) {
    currentStatus = STATUS_CRITICAL;
    currentEventCode = EVENT_HIGH_TEMP;
    currentEventMessage = "HIGH TEMP";
  } else if (currentGas >= GAS_CRITICAL_THRESHOLD) {
    currentStatus = STATUS_CRITICAL;
    currentEventCode = EVENT_HIGH_GAS;
    currentEventMessage = "CRITICAL GAS";
  } else if (vibrationCritical) {
    currentStatus = STATUS_CRITICAL;
    currentEventCode = EVENT_VIBRATION;
    currentEventMessage = "VIBRATION CRIT";
  }
  // 2. WARNING
  else if (currentTemp >= TEMP_WARNING) {
    currentStatus = STATUS_WARNING;
    currentEventCode = EVENT_HIGH_TEMP;
    currentEventMessage = "TEMP WARNING";
  } else if (currentGas >= GAS_WARNING_THRESHOLD) {
    currentStatus = STATUS_WARNING;
    currentEventCode = EVENT_HIGH_GAS;
    currentEventMessage = "GAS WARNING";
  } else if (vibrationWarning) {
    currentStatus = STATUS_WARNING;
    currentEventCode = EVENT_VIBRATION;
    currentEventMessage = "VIBRATION WARN";
  }
  // 3. NORMAL
  else {
    currentStatus = STATUS_NORMAL;
    currentEventCode = EVENT_NONE;
    currentEventMessage = "NORMAL";
  }
}

// ============================================================================
// EVENT HANDLING (DEDUPLICATION)
// ============================================================================
void handleEvents() {
  if (currentStatus != previousStatus || currentEventMessage != previousEventMessage) {
    logEvent(currentEventMessage);
    previousEventMessage = currentEventMessage;
  }
}

void logEvent(String msg) {
  Serial.print(F("[EVENT] "));
  Serial.println(msg);
}

void printSerialStatus() {
  if (currentStatus == STATUS_NORMAL) return; // Keep log quiet during normal
  Serial.print(F("[TEMP] ")); Serial.print(currentTemp); Serial.println(F(" C"));
  Serial.print(F("[GAS] Raw ADC: ")); Serial.println(currentGas);
  Serial.print(F("[STATUS] ")); Serial.println(currentStatus == STATUS_CRITICAL ? "CRITICAL" : "WARNING");
  Serial.println(F("---"));
}

// ============================================================================
// OUTPUTS & ACTUATORS
// ============================================================================
void updateOutputs(unsigned long currentMillis) {
  // Buzzer
  if (currentStatus == STATUS_NORMAL) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerState = false;
  } else if (currentStatus == STATUS_WARNING) {
    if (currentMillis - lastBuzzerToggle >= 1000) {
      lastBuzzerToggle = currentMillis;
      buzzerState = !buzzerState;
      digitalWrite(PIN_BUZZER, buzzerState ? HIGH : LOW);
    }
  } else if (currentStatus == STATUS_CRITICAL) {
    // Rapid beep or continuous
    if (currentMillis - lastBuzzerToggle >= 200) {
      lastBuzzerToggle = currentMillis;
      buzzerState = !buzzerState;
      digitalWrite(PIN_BUZZER, buzzerState ? HIGH : LOW);
    }
  }

  // Relay
  if (currentStatus == STATUS_CRITICAL) {
    digitalWrite(PIN_RELAY, RELAY_ACTIVE_LOW ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RELAY, RELAY_ACTIVE_LOW ? HIGH : LOW);
  }
}

void updateLCD(unsigned long currentMillis) {
  if (!lcdConnected) return;
  
  if (currentStatus == STATUS_CRITICAL) {
    lcd->clear();
    lcd->setCursor(0,0);
    lcd->print("!! CRITICAL !!");
    lcd->setCursor(0,1);
    lcd->print(currentEventMessage);
  } 
  else if (currentStatus == STATUS_WARNING) {
    lcd->clear();
    lcd->setCursor(0,0);
    lcd->print("! WARNING !");
    lcd->setCursor(0,1);
    lcd->print(currentEventMessage);
  }
  else {
    // Rotate normal screens every 4 seconds
    if (currentMillis - lastLCDRotate >= 4000) {
      lastLCDRotate = currentMillis;
      lcdScreenState = !lcdScreenState;
      lcd->clear();
    }
    
    if (lcdScreenState == 0) {
      lcd->setCursor(0,0);
      lcd->print("SYSTEM: NORMAL");
      lcd->setCursor(0,1);
      lcd->print("T:"); lcd->print(currentTemp, 1);
      lcd->print("C H:"); lcd->print(currentHumidity, 0); lcd->print("%");
    } else {
      lcd->setCursor(0,0);
      lcd->print("GAS: "); lcd->print(currentGas);
      lcd->setCursor(0,1);
      lcd->print("VIBRATION: SAFE");
    }
  }
}

// ============================================================================
// WIFI & THINGSPEAK
// ============================================================================
void maintainWiFi(unsigned long currentMillis) {
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiRetry = 0;
    if (currentMillis - lastWiFiRetry >= 30000) {
      lastWiFiRetry = currentMillis;
      Serial.println(F("[WIFI] Retrying connection..."));
      WiFi.reconnect();
    }
  }
}

void sendThingSpeakData() {
  if (WiFi.status() != WL_CONNECTED) {
    if (DEBUG_MODE) Serial.println(F("[CLOUD] Wi-Fi offline. Upload skipped."));
    return;
  }
  if (THINGSPEAK_CHANNEL_ID == 0) return; // Unconfigured

  ThingSpeak.setField(1, currentTemp);
  ThingSpeak.setField(2, currentHumidity);
  ThingSpeak.setField(3, currentGas);
  ThingSpeak.setField(4, flameDetected ? 1 : 0);
  ThingSpeak.setField(5, vibrationCritical ? 2 : (vibrationWarning ? 1 : 0));
  ThingSpeak.setField(6, touchDetected ? 1 : 0);
  ThingSpeak.setField(7, (int)currentStatus);
  ThingSpeak.setField(8, (int)currentEventCode);

  int x = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_API_KEY);
  if (x == 200) {
    if (DEBUG_MODE) Serial.println(F("[CLOUD] ThingSpeak Updated"));
  } else {
    Serial.print(F("[CLOUD] Update failed: ")); Serial.println(x);
  }
}
