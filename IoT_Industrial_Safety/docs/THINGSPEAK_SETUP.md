# ThingSpeak Cloud Setup

ThingSpeak provides a free dashboard to visualize the data sent from the ESP32.

## 1. Create a Channel
1. Go to [thingspeak.com](https://thingspeak.com) and sign up for a free account.
2. Go to **Channels > My Channels > New Channel**.
3. Name the channel (e.g., "Industrial Safety Monitor").

## 2. Configure Fields
Check the boxes for 8 fields and name them exactly as follows to match the ESP32 data format:
- **Field 1:** Temperature (C)
- **Field 2:** Humidity (%)
- **Field 3:** MQ-2 Raw Gas Value
- **Field 4:** Flame Status (0=Safe, 1=Fire)
- **Field 5:** Vibration Status (0=Safe, 1=Warn, 2=Crit)
- **Field 6:** Touch Emergency (0=Normal, 1=Emergency)
- **Field 7:** System Status (0=Normal, 1=Warning, 2=Critical)
- **Field 8:** Event Code 

Click **Save Channel** at the bottom.

## 3. Get Credentials
1. Click the **API Keys** tab on your new channel.
2. Copy the **Channel ID** (a number).
3. Copy the **Write API Key** (alphanumeric string).

## 4. Add to ESP32 Code
Open `IoT_Industrial_Safety.ino` and replace the placeholders:
```cpp
unsigned long THINGSPEAK_CHANNEL_ID = 1234567; // Your ID here (no quotes)
const char* THINGSPEAK_WRITE_API_KEY = "ABCDEFG123456"; // Your key here (keep quotes)
```

## Dashboard Visualization
Once the ESP32 connects to Wi-Fi, it will upload data every 20 seconds. You can view the live updating graphs on the **Private View** tab of your ThingSpeak channel.
