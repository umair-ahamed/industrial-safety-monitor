# Web Dashboard - Industrial Safety Monitor

This dashboard visualizes data uploaded by the ESP32 to ThingSpeak. It acts purely as a monitoring interface. **The ESP32 firmware remains the authoritative decision-maker for local safety.**

## Setup Instructions

1. **Get ThingSpeak Details:** Ensure your ThingSpeak channel is created and you have the **Channel ID**. If your channel is set to Private, you will also need the **Read API Key**. (If public, no Read API key is needed).
2. **Edit `app.js`:** Open `app.js` in a text editor.
3. Locate the `CONFIG` block at the top:
   ```javascript
   const CONFIG = {
       channelId: "YOUR_CHANNEL_ID",     
       readApiKey: "YOUR_READ_API_KEY",  // Leave "" if public
       updateInterval: 15000,
       staleTimeout: 60000 
   };
   ```
4. Insert your actual Channel ID.

## Running the Dashboard

Simply open the `index.html` file in any modern web browser (Chrome, Edge, Firefox, Safari). You do not need to install Node.js, run a local web server, or set up a complex framework.

## Dashboard Features

- **Live Sensor Data:** Temperature, Humidity, MQ-2 Raw Gas Level, Flame, Vibration, and Touch Emergency.
- **System Status Card:** Highlights NORMAL (Green), WARNING (Yellow), or CRITICAL (Red) based on `Field 7`.
- **Historical Graphs:** Visualizes the last 60 data points using Chart.js.
- **Event History:** Translates the numeric Event Code (`Field 8`) into human-readable events.
- **Offline Resilience:** If ThingSpeak stops responding, the dashboard marks data as stale/offline, rather than pretending the system is safe.

## Security Warning

**Never put your ThingSpeak WRITE API KEY into `app.js`.** 
Because this dashboard runs on the client-side (in the browser), anyone who views the webpage source code can see the API keys. Only use the **Read API Key**. The ESP32 is the only device that should have the Write API Key.

## College Demonstration Guide

1. Make sure the ESP32 is running and uploading to ThingSpeak.
2. Open `index.html` on your laptop browser.
3. Show the judges the `NORMAL` status.
4. Trigger a warning (e.g., tap the vibration sensor 3 times). Wait up to 15 seconds for the next ThingSpeak update.
5. The dashboard will automatically update to `WARNING` without refreshing the page.
6. Trigger the touch sensor (Emergency). The ESP32 will immediately sound the buzzer locally. The next ThingSpeak update will push the dashboard into `CRITICAL` mode.
