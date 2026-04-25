# ESP32 Pool Monitor Display

An ESP32-based display that shows pool water temperature, pH, and ORP (chlorine) data received from Home Assistant via MQTT.

## Features

- **MQTT Subscriber**: Receives pool data from Home Assistant
- **OLED Display**: 128x64 SSD1306 screen showing all readings
- **WiFi Connectivity**: Automatic reconnection with configurable timeout
- **Status Bar**: Shows WiFi, MQTT, and data freshness status
- **Large Font Dashboard**: Easy-to-read temperature, pH, and ORP display
- **Auto-Refresh**: Updates display every second

## Hardware Requirements

- ESP32 Cheap Yellow Display (CYD) - 2.8" 320x240 TFT (ST7789)
- USB power supply
- Optional: Waterproof enclosure

## Hardware

This project uses the **ESP32 Cheap Yellow Display (CYD)** board with integrated:
- ST7789 2.8" 320x240 TFT display
- ESP32-WROOM module
- MicroSD card slot
- USB-C or microUSB power

The display uses SPI on these pins:
- MOSI: GPIO 13
- MISO: GPIO 12
- SCK: GPIO 14
- CS: GPIO 15
- DC: GPIO 2
- Backlight: GPIO 21

No wiring required - it's all on the board!

## Configuration

**DO NOT commit credentials to git!**

1. Copy the example environment file:
   ```bash
   cp .env.example .env
   ```

2. Edit `.env` with your credentials:
   ```bash
   WIFI_SSID=your_wifi_ssid
   WIFI_PASS=your_wifi_password
   MQTT_BROKER=your_mqtt_broker_ip
   MQTT_USER=your_mqtt_username
   MQTT_PASS=your_mqtt_password
   ```

3. Build and upload with the environment variables:
   ```bash
   export $(cat .env | xargs) && pio run --target upload
   ```

The `.env` file is already in `.gitignore` and will never be committed.

## Building and Uploading

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## MQTT Topics (Subscribed)

The display is configured for your iopool sensors. Subscribed topics:

| Topic | Description |
|-------|-------------|
| `homeassistant/sensor/iopool_my_pool_temperature/state` | iopool Temperature |
| `homeassistant/sensor/iopool_my_pool_ph/state` | iopool pH |
| `homeassistant/sensor/iopool_my_pool_orp/state` | iopool ORP |
| `pool/temperature/f` | Fallback: Temperature in Fahrenheit |
| `pool/temperature/c` | Fallback: Temperature in Celsius |
| `pool/ph` | Fallback: pH level |
| `pool/orp` | Fallback: ORP level in mV |

## Home Assistant Integration

Your iopool sensors (`sensor.iopool_my_pool_temperature`, `sensor.iopool_my_pool_ph`, `sensor.iopool_my_pool_orp`) need to be published to MQTT. Add this automation to Home Assistant:

```yaml
automation:
  - alias: "Publish iopool Data to MQTT"
    trigger:
      - platform: state
        entity_id:
          - sensor.iopool_my_pool_temperature
          - sensor.iopool_my_pool_ph
          - sensor.iopool_my_pool_orp
    action:
      - service: mqtt.publish
        data:
          topic: "homeassistant/sensor/{{ trigger.entity_id | replace('sensor.', '') }}/state"
          payload: "{{ trigger.to_state.state }}"
          retain: true
```

Or publish periodically:

```yaml
automation:
  - alias: "Publish iopool Data Every 30s"
    trigger:
      - platform: time_pattern
        seconds: "/30"
    action:
      - service: mqtt.publish
        data:
          topic: "homeassistant/sensor/iopool_my_pool_temperature/state"
          payload: "{{ states('sensor.iopool_my_pool_temperature') }}"
          retain: true
      - service: mqtt.publish
        data:
          topic: "homeassistant/sensor/iopool_my_pool_ph/state"
          payload: "{{ states('sensor.iopool_my_pool_ph') }}"
          retain: true
      - service: mqtt.publish
        data:
          topic: "homeassistant/sensor/iopool_my_pool_orp/state"
          payload: "{{ states('sensor.iopool_my_pool_orp') }}"
          retain: true
```

**Note:** Ensure your Home Assistant MQTT integration is configured and the MQTT broker is running.

## To Do

- [ ] Add touch interface for settings
- [ ] Add web configuration interface
- [ ] Add OTA update capability
- [ ] Add graphs/historical display
- [ ] Add alerts for out-of-range values
- [ ] Use microSD for data logging
