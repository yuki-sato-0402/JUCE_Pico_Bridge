# PicoBridge

PicoBridge is a project designed to bridge a JUCE-based audio plugin with a Raspberry Pi Pico via MIDI, allowing for intuitive hardware control and real-time parameter feedback.

This project is derived from the [Fourier Series Expansion Synthesizer](https://github.com/yuki-sato-0402/RNBO_FourierSeriesExpansionSynthesizer). The original is only available as a VST/AU plugin.

## Key Features

- **Bidirectional MIDI Communication**:
  - **JUCE → Pico (SysEx)**: Sends parameter names, current display values (as strings), and range information from the plugin to the Pico using SysEx messages.
  - **Pico → JUCE (CC)**: Translates hardware interactions (rotary encoder, switches) into MIDI CC messages to control plugin parameters.

- **LCD Feedback**: Real-time display of the currently selected parameter name and its value on an LCD connected to the Pico.

- **Intuitive Hardware Interface**:
  - **Left Tactile Switches (Parameter Select)**: Navigate through available parameters. Pressing the top switch advances to the next parameter, and the bottom switch moves to the previous one.
  - **Rotary Encoder (Main Control)**: Smoothly adjusts parameter values across the 0–127 MIDI range, mapped to a 360-degree rotation.
  - **Right Tactile Switches (Step Control)**: Ideal for discrete parameters like checkboxes or combo boxes. Increases or decreases the value by 1 step.

- **Visual Feedback**: LEDs provide immediate visual confirmation of hardware interactions and value levels via PWM dimming.

## Demonstration
[Youtube<img width="1552" height="923" alt="Screenshot 2026-05-07 at 20 34 11" src="https://github.com/user-attachments/assets/7f5a23da-7530-4b67-98a2-132a26f0e817" />](https://youtu.be/uP3yyErYtf8) 

## System Architecture

### Hardware (Raspberry Pi Pico)
- **Platform**: CircuitPython
- **Key Libraries**: `adafruit_midi`, `adafruit_bus_device`
- **Pin Assignments**:
  - **I2C (LCD)**: SDA=GP14, SCL=GP15 (To prevent exceeding the rated voltage of the pico, a bidirectional logic level converter module is included.)
  - **Rotary Encoder**: A=GP27, B=GP28
  - **Left Switches (Selection)**: Up=GP5, Down=GP7
  - **Right Switches (Adjustment)**: Up=GP20, Down=GP8
  - **LEDs (PWM)**: Parameter Select=GP22, Value=GP26, Step Control=GP17

### Software (JUCE Plugin)
- **Engine**: RNBO (Fourier Series Expansion Synthesizer)
- **Communication Protocol**:
  - **SysEx (Manufacturer ID: 0x7D)**: Transmits strings in the format `ParameterName,DisplayValue,MaxSteps`.
  - **MIDI CC**: Parameters are mapped to specific CC numbers (e.g., 20–30, 70–75).

## Getting Started

### 1. Prepare Raspberry Pi Pico
1. Install [CircuitPython](https://circuitpython.org/downloads) on your Raspberry Pi Pico.
2. Copy the contents of `pico_src/code.py` to the Pico's `code.py`.
3. Place the required libraries (like `adafruit_midi`) into the `lib` folder on the Pico.

### 2. Build the JUCE Plugin
1. Build the project using CMake:
   ```bash
   cd JUCE_Pico_Bridge
   git submodule update --init --recursive
   cd build
   cmake ..
   cmake --build .
   ```
2. Load the built plugin into your DAW or a standalone host.

### 3. MIDI Connection
1. Connect the Pico to your PC via USB; it will be recognized as a MIDI device.
2. Enable MIDI input and output for the Pico within your plugin or DAW settings. (Regarding MIDI output from the JUCE plugin, the current specification hardcodes the output device. This will be corrected in the future.)

## Circuit Diagram
<img width="1815" height="1092" alt="JUCE_PIcoBridge_bb" src="https://github.com/user-attachments/assets/9e054a18-b88f-4af0-8322-9f0208b512a7" />

---
This project aims to blur the line between hardware and software in music production, providing a more physical and tactile sound design experience.

## Reference
- [LCD display on the Raspberry Pi Pico with CircuitPython](https://youtu.be/xg-VptjN_Oc?si=p97o7kTZ60eC0n3o)

- [Raspberry Pi Pico CircuitPython I2C LCD1602表示テスト](http://jh7ubc.web.fc2.com/Raspberry_Pi/Raspberry_Pi_Pico/Pi_Pico_CircuitPython_I2C_LCD1602.html)
