                                                          Loop Switcher with MIDI Control
                                                          
This device allows to control one(or both Amp pre/post loops) using MIDI Controller. 
Preset changing performed after receiving Program Change message, but can be modified for any type of message.
Also Schematic can be modified for using with monetary switch. Additional Array with control pins must be initiated and monitored in this case.

MIDI listen requires to use Serial1(available in Arduino Mega) to avoid serial output issues. 

For indication ILI9341 based display used.

For Switching loops 2 8-Relay Modules used
