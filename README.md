# Cordia Labs — ESP32 Tactile Firmware (200 Hz)

Firmware streaming thin-film FSR contact pressure at 200 Hz for dual robot jaws, under $10 per hand.

```
Hardware Cost: <$10 per hand ($20 total)
Sample Rate:   200 Hz (hardware-timed via micros())
Payload:       24-byte little-endian UDP packet
Sensors:       4× RP-S40-ST thin-film piezoresistive FSRs
Host Sync:     Written straight to LeRobot dataset format
```

---

## Why Tactile Sensing at 200 Hz?

Cloth manipulation usually fails at the point of contact. A gripper closes, the fabric slips two millimeters, and the fold comes out crooked. Vision models miss the slip because cameras can't see pressure.

To capture this, the robot's jaws use thin-film force sensors streaming at 200 Hz. The contact data is hardware-synced with the camera views and joint states, and written straight to LeRobot's dataset format. When a fold fails, the slip is in the log.

---

## Hardware Bill of Materials (<$10 / hand)

| Component | Part | Vendor | Qty | Unit Cost | Total |
|---|---|---|---|---|---|
| **Piezoresistive FSR** | RP-S40-ST (0.5 mm thin film) | DFRobot / AliExpress | 4 | $4.50 | $18.00 |
| **Microcontroller** | ESP-WROOM-32 Dev Module | Espressif / Amazon | 1 | $6.00 | $6.00 |
| **Passives & Wiring** | 10kΩ pull-down resistors, FPC breakout, silicone wire | Adafruit / DigiKey | 1 | $5.50 | $5.50 |
| **Total (Dual Arms)** | | | | | **$29.50** |
| **Per Hand Cost** | (Sensors + conditioning share one MCU) | | | | **<$10.00 / hand** |

---

## Circuit & Pinout

All sensors are sampled using **ADC1** channels. ADC2 is intentionally avoided because Wi-Fi activity on the ESP32 disables ADC2.

```
                    3.3V
                     │
              [RP-S40-ST FSR]
                     │
ADC Pin ─────────────┼─────────────── (GPIO 32, 33, 34, 35)
                     │
                 [10kΩ 1%]
                     │
                    GND
```

### Pin Assignments

| Jaw Location | Sensor Channel | ESP32 GPIO | ADC Channel |
|---|---|---|---|
| **Left Arm — Fixed Jaw** | `left_fixed` | GPIO 34 | ADC1_CH6 |
| **Left Arm — Moving Jaw** | `left_moving` | GPIO 35 | ADC1_CH7 |
| **Right Arm — Fixed Jaw** | `right_fixed` | GPIO 32 | ADC1_CH4 |
| **Right Arm — Moving Jaw** | `right_moving` | GPIO 33 | ADC1_CH5 |

---

## Binary Packet Specification (24 Bytes)

Telemetry packets are broadcast over UDP on port **5006** as packed 24-byte little-endian structs:

```c
#pragma pack(push, 1)
struct TactilePacket {
  uint32_t seq;           // Monotonically increasing sequence number
  uint32_t t_us;          // Hardware micros() timestamp at sample time
  float left_fixed;       // 12-bit ADC raw value (0.0 - 4095.0)
  float left_moving;
  float right_fixed;
  float right_moving;
};
#pragma pack(pop)
```

---

## Quick Start

### 1. Configure Credentials
Copy `src/config.h.example` to `src/config.h` and specify your Wi-Fi SSID and target UDP broadcast IP:
```bash
cp src/config.h.example src/config.h
```

### 2. Compile and Flash (Arduino CLI)
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 src/tactile_udp_bridge.ino
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 src/tactile_udp_bridge.ino
```

### 3. Verify Live Telemetry
Run the included Python receiver:
```bash
python3 python/test_receiver.py --port 5006
```

Expected output:
```
Listening for 200 Hz tactile packets on 0.0.0.0:5006 ...
[199.8 Hz] Seq: 1042    | L-Jaw: 1820/1794 | R-Jaw:    0/   0 | Drops: 0
[200.1 Hz] Seq: 1142    | L-Jaw: 1812/1799 | R-Jaw:    0/   0 | Drops: 0
```

---

## License

Licensed under the [Apache License, Version 2.0](LICENSE).
