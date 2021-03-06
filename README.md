# Loop Switcher with MIDI Control

[![License](https://upload.wikimedia.org/wikipedia/commons/8/86/GPL_v3_Blue_Badge.svg)](LICENSE.md)

# Description

![N|Solid](fx_loopswitch.jpg)

This device allows to control one(or both Amp pre/post loops) using MIDI Controller. 
Preset changing performed after receiving Program Change message, but can be modified for any type of message.
Also Schematic can be modified for using with monetary switch. Additional Array with control pins must be initiated and monitored in this case.

# Relays connection

Here is example of a basic relay setup for 4 pedals. To extend it - connect output of the relay module 1 to input of the relay module 2. 

![N|Solid](relays.jpg)

Each relay power consumption is about 70mA, so use powerful and quality power adapter. Some of you could face some noises when FX engaged - check that you connected all grounds from all input jacks to the - of the power.

Additionaly, apply DC filter to clean-up incoming power. Place one on the main after main power input beforre DC-DC converter and one after DC-DC converter. This will significaly improve total sound quality and reduce power bouncing.

![N|Solid](filter.jpg)

Also, you could enable Buffer to compensate increased wire amount and enabling it via relay

![N|Solid](buffer.jpg)

Important:

  - MIDI listen requires to use Serial1(available in Arduino Mega) to avoid serial output issues. 
  - Use DC-DC converter to power up relays(5v required) as Arduinoi can work from pedal power supply.
  - For indication ILI9341 based display used.
  - For Switching loops 2 8-Relay Modules used
  

### Libraries

FX Switcher requiers 3rd party libs:

| Library | Link |
| ------ | ------ |
| GyverEncoder | https://github.com/AlexGyver/GyverLibs |
| MIDI Library | https://www.arduino.cc/reference/en/libraries/midi-library/ |

                                                          






Small shootout(in russian): https://youtu.be/fvrEu6rek-M
