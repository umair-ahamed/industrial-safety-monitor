# IoT-Based Multi-Sensor Industrial Safety and Machine Condition Monitoring System

An educational IoT prototype built on the ESP32 to demonstrate multi-sensor monitoring, local decision logic, and automated emergency response without relying entirely on internet connectivity.

## Project Objective
In industrial settings, undetected machine overheating, dangerous gas leaks, and severe vibrations can lead to catastrophic failure. This prototype demonstrates an edge-computing architecture where sensors are processed locally by an ESP32. It evaluates data to classify the machine's state (NORMAL, WARNING, or CRITICAL) and triggers immediate local alarms (Buzzer, Emergency LED via Relay). Supplementary telemetry is sent to a ThingSpeak cloud dashboard for remote historical monitoring.

## Hardware Required
1. ESP32 DevKit V1 (or compatible Dev Module)
2. DHT11 Temperature and Humidity sensor
3. MQ-2 Gas sensor module (Analog output)
4. IR Flame sensor module (Digital output)
5. SW-520D Tilt/Vibration sensor module (Digital output)
6. Touch sensor module
7. Piezoelectric buzzer
8. 1-Channel 5V Relay module
9. 16x2 JHD162A LCD with I2C Backpack
10. One 5mm red LED + 220Ω 1/4W Resistor

*(Note: The HC-SR04 ultrasonic sensor is omitted in Version 1 to preserve 3.3V logic safety and reserve the single 220Ω resistor for the emergency LED).*

## Pin Mapping
See `docs/PINOUT.md` for exact wiring details.

## Software Requirements
- **IDE:** Arduino IDE 2.x
- **Board Package:** ESP32 by Espressif Systems
- **Libraries:**
  - `WiFi.h` (Built-in)
  - `Wire.h` (Built-in)
  - `LiquidCrystal_I2C` (by Frank de Brabander or similar)
  - `DHT sensor library` (by Adafruit)
  - `Adafruit Unified Sensor` (Dependency for DHT)
  - `ThingSpeak` (by MathWorks)

## Configuration
Before uploading, open `IoT_Industrial_Safety.ino` and edit the `CONFIGURATION` section at the top of the file:
1. Provide your `WIFI_SSID` and `WIFI_PASSWORD`.
2. Provide your `THINGSPEAK_CHANNEL_ID` and `THINGSPEAK_WRITE_API_KEY`.
3. Adjust sensor logic (Active HIGH vs LOW) if your specific modules differ from defaults.

## ThingSpeak Setup & Dashboard
See `docs/THINGSPEAK_SETUP.md` for instructions on creating the cloud dashboard.

## Upload Procedure
1. Open `IoT_Industrial_Safety.ino` in Arduino IDE.
2. Go to **Tools > Board** and select **ESP32 Dev Module**.
3. Select the correct COM port.
4. Click **Upload**. (If it fails to connect, hold the `BOOT` button on the ESP32 when "Connecting..." appears).

## LCD Address Detection
The code includes an automatic I2C scanner on boot. It will search for `0x27` or `0x3F` and initialize the LCD automatically. If no LCD is found, the system continues monitoring without freezing.

## MQ-2 Calibration
During startup, the system takes 10 samples from the MQ-2 to establish a raw clean-air baseline. This is printed to the Serial Monitor. **This is a raw ADC value, NOT exact ppm.** The default warning/critical thresholds in the code (1500/2500) are approximations and should be adjusted based on the baseline printed at boot.

## Warning & Critical Logic
- **NORMAL:** Safe conditions.
- **WARNING:** Elevated gas, high temp (>35°C), or sustained vibration. Triggers an intermittent buzzer and LCD warning.
- **CRITICAL:** Manual touch emergency, flame detection, extreme temp (>45°C), extreme gas, or escalated severe vibration. Triggers continuous buzzer, activates the emergency Relay/LED, and overrides all other states.

## College Demonstration Procedure
**IMPORTANT:** Always demonstrate safely.
1. **Temperature:** Blow warm air on the DHT11 or lower the `TEMP_WARNING` threshold to ambient to show the warning trigger.
2. **Humidity:** Read directly from the LCD or Serial Monitor.
3. **MQ-2:** Demonstrate using an unlit butane lighter near the sensor (no open flame).
4. **Flame Sensor:** Bring a safe, small flame (like a lighter) near the sensor, ensuring it is kept away from electronics.
5. **Vibration:** Tap the SW-520D sensor gently 3 times for a Warning, or 10 times rapidly for a Critical escalation.
6. **Touch:** Touch the pad to instantly enter a manual CRITICAL emergency state.
7. **Relay + LED:** Observe the Red LED turn on automatically during any Critical event.
8. **Resilience:** Disconnect the Wi-Fi router. Show the judges that the buzzer, relay, and LCD still react instantly to a flame or touch, proving local processing reliability.

## Limitations
- The MQ-2 provides a raw analog approximation, not a certified exact ppm reading.
- The relay demonstrates low-voltage 5V logic. **Do not connect AC mains power to this prototype.**
- Thresholds are demonstration values, not certified industrial safety limits.

## Future Improvements
- Integrate HC-SR04 for operator proximity sensing (requires voltage dividers).
- Replace SW-520D with an MPU6050 for FFT-based predictive maintenance vibration analysis.
- Implement an asynchronous local web server for low-latency dashboards without internet.

## Troubleshooting
See `docs/TROUBLESHOOTING.md` for common issues and fixes.
