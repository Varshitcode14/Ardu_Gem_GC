# Hungry Balls - Arduino Dual-Player Game

A fun, interactive two-player game built for Arduino with TFT display and rotary encoders. Players control balls that collect coins to score points within a 60-second time limit.

![Hungry Balls Game](https://placeholder.svg?height=300&width=400&text=Hungry+Balls+Game)

## Features

- Two-player competitive gameplay
- Interactive menu system with rotary encoder navigation
- Real-time score tracking
- Coin collection mechanics
- Physics-based jumping and movement
- Game restart functionality
- Colorful TFT display interface
- Consistent 60-second game timer

## Hardware Requirements

- Arduino board (Uno/Mega recommended)
- TFT_22_ILI9225 display
- 2 × Rotary encoders with push buttons
- Jumper wires
- Breadboard
- 5V power supply

## Wiring

### TFT Display
- RST → A4
- RS → A3
- CS → A5
- SDI/MOSI → A2
- CLK/SCK → A1
- LED → 5V (or pin 0 for brightness control)
- VCC → 5V
- GND → GND

### Rotary Encoder 1 (Player 1)
- CLK → Pin 4
- DT → Pin 5
- SW → Pin 9
- VCC → 5V
- GND → GND

### Rotary Encoder 2 (Player 2)
- CLK → Pin 11
- DT → Pin 12
- SW → Pin 8
- VCC → 5V
- GND → GND

## Installation

1. Install the TFT_22_ILI9225 library in your Arduino IDE:
   - Go to Sketch → Include Library → Manage Libraries
   - Search for "TFT_22_ILI9225" and install

2. Clone this repository:

