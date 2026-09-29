# Complete System Test Cases

This document outlines the structured test cases for verifying the functionality of the IoT-Based Multi-Sensor Industrial Safety Prototype. Perform these tests systematically to ensure all local safety logic and cloud telemetry are working correctly.

## 1. Normal Operation (Baseline)
**Objective:** Verify that the system operates quietly and normally when there are no emergency conditions.
- **Action:** Power on the ESP32 and do not interact with any sensors. Allow the MQ-2 to calibrate (takes ~10 seconds).
- **Expected Results:**
  - **Serial Monitor:** Prints "System Ready. Entering Monitoring Mode." followed by periodic `[TEMP]` and `[GAS]` readings.
  - **LCD:** Rotates every 4 seconds between:
    - Screen 1: `SYSTEM: NORMAL` / `T: XX.XC H: XX%`
    - Screen 2: `GAS: XXX` / `VIBRATION: SAFE`
  - **Actuators:** Buzzer is SILENT. Relay is OFF. Red LED is OFF.
  - **ThingSpeak:** `System Status` field shows `0`.

## 2. Temperature Warning Test
**Objective:** Verify that elevated temperatures trigger a warning without causing a full shutdown.
- **Action:** Using a hairdryer on the lowest setting (or blowing warm breath), gently raise the temperature of the DHT11 above the `TEMP_WARNING` threshold (default: 35°C). 
- **Expected Results:**
  - **Serial Monitor:** Prints `[EVENT] TEMP WARNING`.
  - **LCD:** Displays `! WARNING !` / `TEMP WARNING`.
  - **Actuators:** Buzzer beeps intermittently (1 second ON, 1 second OFF). Relay/LED remain OFF.
  - **Recovery:** Once temperature drops below 35°C, the system automatically returns to NORMAL.

## 3. Gas Warning & Critical Test
**Objective:** Verify that the MQ-2 sensor escalation logic functions correctly.
- **Action:** Release a *small* amount of unlit butane from a lighter near the MQ-2 sensor.
- **Expected Results:**
  - **Warning Stage:** As raw gas ADC exceeds `GAS_WARNING_THRESHOLD` (default 1500), LCD shows `GAS WARNING` and buzzer beeps intermittently.
  - **Critical Stage:** As concentration increases and exceeds `GAS_CRITICAL_THRESHOLD` (default 2500), LCD shows `!! CRITICAL !!` / `CRITICAL GAS`. Buzzer beeps rapidly (or continuously). Relay activates (Red LED turns ON).
  - **ThingSpeak:** `System Status` field updates to `1` then `2`. `Event Code` shows `2`.

## 4. Flame Detection (Immediate Critical)
**Objective:** Verify that fire detection bypasses warnings and immediately triggers a critical shutdown.
- **Action:** Safely light a lighter a few inches away from the IR Flame Sensor.
- **Expected Results:**
  - **Serial Monitor:** Prints `[EVENT] FIRE ALERT`.
  - **LCD:** Instantly updates to `!! CRITICAL !!` / `FIRE ALERT`.
  - **Actuators:** Buzzer sounds rapidly. Relay activates instantly, turning ON the Red LED.
  - **Recovery:** Once the flame is removed, the system auto-recovers to NORMAL (assuming no other critical states exist).

## 5. Vibration Escalation Test
**Objective:** Verify the debounce and escalation logic of the SW-520D tilt/vibration sensor.
- **Action 1 (Warning):** Gently tap the vibration sensor 3 to 4 times within a 10-second window.
  - **Expected:** System enters WARNING mode. LCD shows `VIBRATION WARN`. Buzzer beeps intermittently.
- **Action 2 (Critical):** Rapidly shake or tap the sensor 10+ times within the 10-second window.
  - **Expected:** System escalates to CRITICAL mode. LCD shows `VIBRATION CRIT`. Relay activates (Red LED ON).
- **Action 3 (Reset):** Stop touching the sensor completely.
  - **Expected:** After 30 seconds of absolute quiet, the escalation counter resets. The system returns to NORMAL.

## 6. Manual Emergency E-Stop (Touch Sensor)
**Objective:** Verify the operator's ability to trigger a manual emergency stop.
- **Action:** Touch and hold the Touch Sensor pad.
- **Expected Results:**
  - **Serial Monitor:** Prints `[EVENT] MANUAL EMERGENCY`.
  - **LCD:** Displays `!! CRITICAL !!` / `MANUAL EMERGENCY`.
  - **Actuators:** Buzzer sounds rapidly. Relay activates instantly (Red LED ON).
  - **ThingSpeak:** `Touch Emergency` field logs `1`.
  - **Recovery:** When you remove your finger, the system returns to NORMAL.

## 7. Network Resilience (Wi-Fi Failure) Test
**Objective:** Prove that edge-computing safety logic functions independently of the cloud.
- **Action:**
  1. While the system is running in NORMAL mode, turn off your Wi-Fi router or mobile hotspot.
  2. Wait 30 seconds for the ESP32 to realize it is disconnected. Serial Monitor will print `[WIFI] Retrying connection...` and `[CLOUD] Wi-Fi offline. Upload skipped.`
  3. While Wi-Fi is still OFF, trigger the Flame Sensor or Touch Sensor.
- **Expected Results (CRITICAL SUCCESS CRITERIA):**
  - The LCD, Buzzer, and Relay/Red LED must react **instantly** to the critical event. 
  - The system must **not** freeze or lock up waiting for the internet to return.
  - When Wi-Fi is turned back on, the ESP32 should eventually reconnect automatically and resume sending data to ThingSpeak without requiring a manual reboot.
