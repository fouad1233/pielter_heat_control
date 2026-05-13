# Peltier Heat Control

This project controls a Peltier element using an H-Bridge (e.g., BTS7960) with an Arduino Nano, reading hardware buttons, monitoring temperature with an NTC 10K, and allowing Manual or Automatic (PID) control through a pure HTML+JS Web Serial Dashboard.

## Architecture

This project is built around the **Web Serial API**. No Python or Node.js backend is required! 
A purely frontend `ui/index.html` file can connect directly to your Arduino Nano over USB. The Arduino constantly outputs telemetry in JSON format and accepts JSON commands.

## Wiring Connections

| Component   | Pin / Connection       | Note                                                                       |
| ----------- | ---------------------- | -------------------------------------------------------------------------- |
| **Peltier** | M+ / M- on H-Bridge    | Ensure proper thermal dissipation (heatsinks/fans) on BOTH sides.          |
| **H-Bridge**| LPWM -> D11            | PWM for one direction.                                                     |
|             | RPWM -> D3             | PWM for opposite direction.                                                |
|             | L_EN -> D2             | Logic Enable.                                                              |
|             | R_EN -> GND (or D2)    | *Note: BTS7960 usually requires both EN pins HIGH for bi-directional. Try linking R_EN and L_EN to D2.* |
| **NTC Thermistor**| A7             | Uses a 1K NTC and a 1K pulldown resistor.                                |
| **VCC**     | 5V / 3.3V / A7         | Reference voltage block.                                                   |
| **Buttons** | D8 (Red/Warm)          | Wired with internal Pullup. Ground the pin to activate.                    |
|             | D7 (Blue/Cold)         | Wired with internal Pullup. Ground the pin to activate.                    |

*Note: If your H-Bridge module requires R_EN to go HIGH to steer the reverse current, tie both L_EN and R_EN to D2.*

## How to use

1. **Flash Firmware**:
   - Install [PlatformIO](https://platformio.org/) in VSCode.
   - Run the Upload command to program your Nano ATmega328.

2. **Open the UI**:
   - Simply open `ui/index.html` in an Edge or Chrome browser.
   - Click **Connect to Arduino** and select your Arduino COM/Serial port from the popup list (baud rate is `115200`).

3. **Functionality**:
   - **Manual Mode**: Drag the slider from -1024 (Max Cold) to 1024 (Max Warm).
   - **Auto/PID Mode**: Enable Auto mode, type your target setpoint, and see the PID dynamically adjust the PWM output based on the NTC readings natively.
   - **Hardware Buttons**: The Blue & Red buttons will immediately override the UI to max Cold or max Warm.

## Dependencies Embedded (automatically loaded by PlatformIO)
- `bblanchon/ArduinoJson`
- `br3ttb/PID`
# pielter_heat_control
