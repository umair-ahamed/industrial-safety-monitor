# Troubleshooting Guide

### 1. COM Port Missing / Upload Failure
- Make sure you are using a data-capable USB cable, not just a charging cable.
- You may need to install the CP210x or CH340 USB drivers depending on your ESP32 board.
- If the IDE says "Connecting..." but fails to upload, press and hold the **BOOT** button on the ESP32 until the upload progress starts.

### 2. LCD is Blank or Displays Blocks
- **Contrast Issue:** Take a small Phillips screwdriver and turn the blue potentiometer on the back of the I2C backpack until the text becomes visible.
- **Address Issue:** Open the Serial Monitor. If the startup log says `LCD: NOT DETECTED`, check your SDA (GPIO 21) and SCL (GPIO 22) wiring. Ensure the LCD gets 5V (VIN).

### 3. DHT11 Invalid Readings
- Check the Serial Monitor. If it says `[DHT ERROR] Invalid reading`, the data wire (GPIO 4) is loose or disconnected. 

### 4. MQ-2 Readings are Unstable
- Gas sensors require a "burn-in" period. Leave the system powered on for 10-15 minutes to let the internal heater stabilize before relying on the baseline value.

### 5. Sensor Logic is Reversed (Flame/Vibration constantly triggering)
- Some modules output HIGH when triggered, others output LOW.
- If the system enters CRITICAL immediately upon turning on without any fire, change `const bool FLAME_ACTIVE_LOW = true;` to `false;` at the top of the `.ino` file.

### 6. Relay Logic is Reversed
- If the red LED is ON during NORMAL and OFF during CRITICAL, change `const bool RELAY_ACTIVE_LOW = true;` to `false;`.

### 7. Buzzer Not Sounding
- Piezo buzzers are polarized. Ensure the positive (longer) leg is connected to GPIO 25, and the negative (shorter) leg is connected to GND.

### 8. Wi-Fi Connection Failure
- Ensure you entered your 2.4GHz Wi-Fi SSID and password correctly. The ESP32 **cannot** connect to 5GHz networks.

### 9. ThingSpeak Update Failure
- Free ThingSpeak accounts enforce a strict 15-second minimum interval between updates. The code uses 20 seconds. If it says `Update failed`, ensure your API key is correct and you haven't changed the interval to something too fast.
