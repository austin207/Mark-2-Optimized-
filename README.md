# Mark-2-Optimized

BLE-controlled E-ATV (Electric All-Terrain Vehicle) firmware for ESP32 microcontroller.

## Overview

Mark-2-Optimized is an optimized firmware implementation for a four-wheel drive electric vehicle controlled via Bluetooth Low Energy (BLE). The system uses dual TB6612FNG motor drivers to control four independent DC motors with directional control.

## Features

- **BLE Communication**: Real-time control via BLE server interface
- **4-Motor Control**: Independent control of front-left, front-right, rear-left, and rear-right motors
- **Movement Modes**: Forward, backward, left/right turns, and diagonal movements
- **Command Processing**: Simple string-based command parsing (MOVE, LIGHT, HORN, STOP)
- **Emergency Stop**: Immediate motor cutoff via STOP command

## Hardware Requirements

- ESP32 microcontroller
- 2x TB6612FNG dual motor driver modules
- 4x DC motors
- Power supply (battery)

## Motor Configuration

- **Left Side**: TB6612FNG #1 (LF_DIR1, LF_DIR2, LR_DIR1, LR_DIR2)
- **Right Side**: TB6612FNG #2 (RF_DIR1, RF_DIR2, RR_DIR1, RR_DIR2)

## Command Format

Commands are sent via BLE characteristic writes:

```
MOVE:UP=1,DOWN=0,LEFT=0,RIGHT=0,SPEED=255,BOOST=0
STOP:1
LIGHT:1
HORN:1
```

## Getting Started

1. Upload the sketch to ESP32
2. Connect via BLE client (device name: "E-ATV")
3. Send movement commands to control the vehicle

## License

MIT
