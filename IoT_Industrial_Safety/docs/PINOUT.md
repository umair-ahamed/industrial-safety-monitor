# PINOUT & WIRING GUIDE

**Important Rule:** All modules must share a common Ground (GND) with the ESP32. Do not feed 5V logic directly into an ESP32 GPIO pin. 

| Component / Module | VCC (Power) | GND (Ground) | Signal Pin | ESP32 GPIO | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **DHT11 (Temp/Hum)** | 3.3 V | GND | DATA | **GPIO 4** | |
| **IR Flame Sensor** | 3.3 V | GND | DO | **GPIO 27** | Adjust onboard potentiometer for sensitivity |
| **MQ-2 Gas Sensor** | 3.3 V | GND | AO (Analog) | **GPIO 34** | Must use GPIO 34 (Input only). Do not use DO. |
| **SW-520D (Tilt)** | 3.3 V | GND | DO | **GPIO 26** | |
| **Touch Sensor** | 3.3 V | GND | OUT | **GPIO 13** | Manual Emergency trigger |
| **Buzzer (Piezo)** | -- | GND | Positive (+) | **GPIO 25** | Connect negative to GND. |
| **5V Relay Module** | 5 V (VIN) | GND | IN | **GPIO 23** | 5V power for the coil, 3.3V GPIO triggers the logic safely. |
| **16x2 LCD (I2C)** | 5 V (VIN) | GND | SDA / SCL | **21 / 22** | LCD handles its own 5V logic safely over I2C |

## Emergency LED Wiring
The relay controls the emergency red LED.
1. Connect **5V** from the ESP32 `VIN` pin to the **Common (COM)** terminal of the relay.
2. Connect the **Normally Open (NO)** terminal of the relay to one end of the **220Ω resistor**.
3. Connect the other end of the **resistor** to the **Long leg (Anode/+)** of the red LED.
4. Connect the **Short leg (Cathode/-)** of the red LED to **GND**.

*Warning: This is for the 5V demonstration LED only. Do NOT wire AC mains.*
