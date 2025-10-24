# Dog Potty Tracker

A hardware device to track your dog's potty activities with three independent timers (Outside, Pee, Poop) displayed on an OLED screen with visual LED status indicators.

## Features

- **3 Independent Timers**: Track time since last Outside, Pee, and Poop events
- **OLED Display**: 0.96" display with auto-rotating views (elapsed time and timestamps)
- **LED Status Indicators**: Green (all good), Yellow (warning), Red (urgent)
- **WiFi & NTP Sync**: Automatic time synchronization for accurate timestamps
- **IFTTT Notifications**: Push notifications and Alexa announcements when dog needs to go out
- **Night Mode**: Auto sleep 11pm-5am to save power
- **Data Persistence**: Timers saved to EEPROM, survive power loss
- **Long-press Reset**: Hold button for 3 seconds to reset individual timers

## Hardware Required

### Components

- **WeMos D1 Mini** (ESP8266 ESP-12F) - micro USB, WiFi-enabled
- **UCTRONICS 0.96" OLED** (SSD1306, 128x64, I2C)
- **3x Tactile Push Buttons** (Outside, Pee, Poop)
- **3x LEDs** (Green, Yellow, Red - standard 5mm)
- **3x 10k-ohm Resistors** (button pull-down)
- **3x 220-330 ohm Resistors** (LED current limiting)
- **Breadboard** (optional): 170-point minimum, 400-point recommended
- **Jumper Wires**

### Total Cost
Approximately $10-15 for all components

## Pin Connections

### OLED Display (I2C)
- VCC -> 3V3
- GND -> GND
- SCL -> D1 (GPIO5)
- SDA -> D2 (GPIO4)

### Buttons
- Button 1 (Outside) -> D5 (GPIO14) + 10k-ohm resistor to GND
- Button 2 (Pee) -> D6 (GPIO12) + 10k-ohm resistor to GND
- Button 3 (Poop) -> D7 (GPIO13) + 10k-ohm resistor to GND
- All buttons: One leg to 3V3, other leg to pin

### Status LEDs
- LED Green -> D0 (GPIO16) + 220-330 ohm resistor
- LED Yellow -> D3 (GPIO0) + 220-330 ohm resistor
- LED Red -> D8 (GPIO15) + 220-330 ohm resistor
- All LEDs: Anode (+) to pin through resistor, Cathode (-) to GND

## Software Setup

### 1. Install Arduino IDE

1. Download Arduino IDE 2.x from https://www.arduino.cc/en/software
2. Install and launch Arduino IDE

### 2. Install ESP8266 Board Support

1. Open File -> Preferences
2. Add to "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to Tools -> Board -> Boards Manager
4. Search "esp8266" and install "esp8266 by ESP8266 Community"
5. Select board: Tools -> Board -> ESP8266 Boards -> "LOLIN(WEMOS) D1 R2 & mini"

### 3. Install Required Libraries

Open Tools -> Manage Libraries and install:

- **Adafruit SSD1306** (by Adafruit)
- **Adafruit GFX Library** (by Adafruit)
- **ESP8266WiFi** (built-in with ESP8266 board package)
- **Time** (by Michael Margolis)
- **EEPROM** (built-in)

### 4. Configure WiFi Credentials

1. Copy `dog-potty-tracker/secrets.h.example` to `dog-potty-tracker/secrets.h`
2. Edit `dog-potty-tracker/secrets.h` with your WiFi credentials:
   ```cpp
   const char* WIFI_SSID = "YourNetworkName";
   const char* WIFI_PASSWORD = "YourPassword";
   ```
3. Save the file (this file is ignored by git for security)

### 5. Configure IFTTT Notifications (Optional)

To receive push notifications and Alexa announcements when the red LED turns on (dog needs to pee):

#### Step 1: Create IFTTT Account
1. Go to [ifttt.com](https://ifttt.com) and create a free account
2. Download the IFTTT mobile app (iOS/Android)

#### Step 2: Get Your Webhook Key
1. Go to [https://ifttt.com/maker_webhooks/settings](https://ifttt.com/maker_webhooks/settings)
2. Copy your webhook key (looks like: `a1b2c3d4e5f6g7h8i9j0`)

#### Step 3: Create Applet for Push Notifications
1. Go to [https://ifttt.com/create](https://ifttt.com/create)
2. Click "If This"
   - Choose "Webhooks"
   - Choose "Receive a web request"
   - Event name: `dog_pee_alert`
   - Click "Create trigger"
3. Click "Then That"
   - Choose "Notifications"
   - Choose "Send a notification from the IFTTT app"
   - Message: `{{Value1}}` (this will show the time since last pee)
   - Click "Create action"
4. Click "Continue" and "Finish"

#### Step 4: Create Applet for Alexa Announcement
1. Create another applet at [https://ifttt.com/create](https://ifttt.com/create)
2. Click "If This"
   - Choose "Webhooks"
   - Choose "Receive a web request"
   - Event name: `dog_pee_alert` (same as before)
   - Click "Create trigger"
3. Click "Then That"
   - Choose "Amazon Alexa"
   - Choose "Make an announcement"
   - Message: `Your dog needs to pee! {{Value1}}`
   - Click "Create action"
4. Click "Continue" and "Finish"

#### Step 5: Add Webhook Keys to secrets.h
1. Edit `dog-potty-tracker/secrets.h`
2. Add your webhook keys (supports up to 3 people):
   ```cpp
   const char* IFTTT_WEBHOOK_KEY_1 = "your_webhook_key_here";
   const char* IFTTT_WEBHOOK_KEY_2 = "girlfriend_webhook_key_here";
   const char* IFTTT_WEBHOOK_KEY_3 = "";  // Optional third person
   const char* IFTTT_EVENT_NAME = "dog_pee_alert";
   ```
3. Save the file

**Multiple Users Setup:**
- **Each person needs their own IFTTT account** and webhook key
- Get webhook keys from https://ifttt.com/maker_webhooks/settings (after logging in)
- Both people should create the same applets (Steps 3 & 4) in their own accounts
- The device will send notifications to all configured webhook keys
- Leave a webhook key blank (`""`) to disable that notification slot

**Example:**
- You create an IFTTT account → Get your webhook key → Add as `IFTTT_WEBHOOK_KEY_1`
- Girlfriend creates her own IFTTT account → Gets her webhook key → Add as `IFTTT_WEBHOOK_KEY_2`
- Both of you will receive notifications when the red LED turns on!

**Note:** You'll receive a notification when the pee timer exceeds 3 hours, with a 1-hour cooldown to prevent spam. The display will show "Alert Sent (2)" to confirm both notifications were sent successfully.

### 6. Install USB Driver (if needed)

If your computer doesn't recognize the WeMos D1 Mini:
- WeMos D1 Mini uses **CH340** USB-to-serial chip
- Download driver from: https://www.wemos.cc/en/latest/ch340_driver.html
- Install and restart Arduino IDE

## Deployment Instructions

### Upload to Device

1. **Connect WeMos D1 Mini** to your computer via micro USB cable

2. **Configure Arduino IDE**:
   - Board: "LOLIN(WEMOS) D1 R2 & mini"
   - Upload Speed: 921600 (or 115200 if upload fails)
   - CPU Frequency: 80 MHz
   - Flash Size: "4MB (FS:2MB OTA:~1019KB)"
   - Port: Select the COM port where D1 Mini is connected

3. **Open the sketch**:
   - File -> Open -> `dog-potty-tracker/dog-potty-tracker.ino`

4. **Verify/Compile**:
   - Click the checkmark icon or Sketch -> Verify/Compile
   - Check for any errors

5. **Upload**:
   - Click the arrow icon or Sketch -> Upload
   - Wait for "Done uploading" message

6. **Monitor Output** (optional):
   - Tools -> Serial Monitor
   - Set baud rate to **115200**
   - You should see WiFi connection status and debug output

### First Boot

1. Device will attempt to connect to WiFi (displays "Connecting WiFi...")
2. Once connected, it will sync time via NTP (displays "Syncing time...")
3. Display will show all three timers counting up from 00h 00m
4. Green LED should turn on (all good status)
5. Display will auto-rotate every 5 seconds between:
   - View A: Elapsed time (e.g., "2h 15m ago")
   - View B: Timestamp (e.g., "1:30 PM")

## Usage

### Button Functions

**Outside Button (1 press)**
- Logs "went outside" event
- Resets Outside timer only

**Pee Button (1 press)**
- Logs "pee" event
- Resets Pee timer AND Outside timer

**Poop Button (1 press)**
- Logs "poop" event
- Resets Poop timer AND Outside timer

**Any Button (Hold 3 seconds)**
- Resets that specific timer only
- Display shows "Resetting [Timer Name]..."

### LED Status Indicators

LEDs are based on the **Pee timer only**:

- **Green**: Pee timer < 90 minutes (all good)
- **Yellow**: Pee timer > 90 minutes (warning)
- **Red**: Pee timer > 3 hours (urgent)

### Night Mode (11pm - 5am)

- Display and LEDs automatically turn off at 11pm
- Press any button to wake display AND log event
- Display stays on for 10 seconds then turns back off
- Timers continue running in background

### Display Views

**View A - Elapsed Time** (5 seconds)
```
OUT: 2h 15m ago
PEE: 0h 45m ago
POO: 1h 30m ago
[3:45 PM]
```

**View B - Timestamps** (5 seconds)
```
OUT: 1:30 PM
PEE: 3:00 PM
POO: 2:15 PM
[3:45 PM]
```

## Troubleshooting

### OLED Not Displaying

- Check I2C address (0x3C or 0x3D)
  - Run I2C scanner code from CLAUDE.md to find address
  - Update `OLED_ADDRESS` in `config.h` if needed
- Verify wiring: SDA->D2, SCL->D1
- Ensure VCC connected to 3V3
- Check if Adafruit_SSD1306 library is installed

### Button Not Responding

- Verify pull-down resistor (10k-ohm to GND)
- Check button wiring: one leg to 3V3, other to GPIO
- Test with multimeter: should read 0V when not pressed, 3.3V when pressed
- Try increasing debounce delay in `config.h`

### LEDs Not Lighting

- Check polarity: anode (+) to GPIO through resistor, cathode (-) to GND
- Verify resistor value (220-330 ohm)
- Test LED directly: connect anode to 3V3, cathode to GND
- Check if correct pin defined in `config.h`

### WiFi Not Connecting

- Verify credentials in `secrets.h`
- Check 2.4GHz WiFi (ESP8266 doesn't support 5GHz)
- Monitor Serial output for connection status
- Device still works without WiFi (shows elapsed timers only)

### Time Not Syncing

- Verify WiFi is connected first
- Check NTP server accessibility
- Ensure timezone offset is correct in `config.h`
- May take 1-2 seconds on first sync

### EEPROM Data Corrupted

- Check checksum verification logic
- EEPROM may be blank on first boot (expected)
- Device will initialize to zero and start fresh

## Configuration

Edit `src/config.h` to customize:

- **Timer Thresholds**: Adjust yellow/red LED warning times
- **Night Mode Hours**: Change 11pm-5am to different times
- **Display Rotation**: Change 5-second interval
- **Timezone**: Adjust NTP timezone offset
- **Debounce Delay**: Adjust button sensitivity

## Power Requirements

- USB powered via micro USB on WeMos D1 Mini
- Always-on device (~80mA draw)
- No battery operation in current version

## Data Persistence

- Timer data automatically saved to EEPROM:
  - On button press (immediate save)
  - Every 5 minutes (automatic backup)
- Data survives power loss and device restarts
- Timers resume from last saved state on boot

## Future Enhancements

Potential features to add:
- WiFi data logging (ThingSpeak, Google Sheets)
- Web interface for viewing history
- Multiple dog support
- Audio alerts (buzzer)
- Battery operation with deep sleep
- 3D printed enclosure

## License

This project is intended for personal use. Modify and enhance as needed.

## Support

For detailed technical documentation, wiring diagrams, and code architecture, see [CLAUDE.md](CLAUDE.md).

---

**Last Updated**: 2025-10-22
**Platform**: WeMos D1 Mini (ESP8266 ESP-12F)
**Arduino IDE**: 2.x
**ESP8266 Board Package**: 3.x
