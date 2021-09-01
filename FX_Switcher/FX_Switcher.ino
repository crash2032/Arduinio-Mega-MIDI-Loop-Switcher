/*        Loop Switcher with MIDI Control
Allows to control one(or pre/post loops) using MIDI Controller.
Preset changing performed after receiving Program Change message, but can be modified for any type of message.
Also Schematic can be modified for using with monetary switch. Additional Array with control pins must be initiated and monitored in this case.

MIDI listen requires to use Serial1(available in Arduino Mega) to avoid serial output issues.

NOT FOR COMMERCIAL USE!

Contacts & Media:

e-mail: crash2032@gmail.com
YouTube: https://www.youtube.com/channel/UCwjdsfFlaZnxokcdG-5whaQ
INSTAGRAM: https://www.instagram.com/mykhailo_chuha/
Facebook: https://www.facebook.com/mykhailo.chuha/
*/



#include <GyverEncoder.h>
#include <TFT.h>
#include <SPI.h>
#include <MIDI.h>
#include <EEPROM.h> // Provides reading from and writing to EEPROM for persisting the Presets.
MIDI_CREATE_DEFAULT_INSTANCE();

#define LED 13
#define BufferPIN 6
#define Bypass 0xFF


//Dispay pins INIT

//Connect SDA to 51
//Connect SCK to 52
//Connect CS to 53
//Connect RESET to 8
//Connect A0 to 9

#define CS   53
#define DC   9  //A0
#define RST  8  //Reset

//Encoder pins INIT
#define CLK 2
#define DT 3
#define SW 4

//Init pedals config
const byte maxSupportedPedals = 8;
byte presets[maxSupportedPedals * maxSupportedPedals];
byte presetCurrentIndex = 0; // Default Preset used.
byte presetBeingEditted = 0; // Preset under the editing
byte loopSwitchOutputPins[maxSupportedPedals];

byte bufferAddr = (maxSupportedPedals * maxSupportedPedals) + 1;  //buffer state right after last preset in EEPROM
boolean bufferState;
//  Loop Switcher State
//  Play = Listen for MIDI and switch presets after MIDI PC message.
//  Edit   = Clicking a encoder button toggles a looper's state while editting a preset.
enum SwitcherState
{
  PlayPresetState = 0,
  EditPresetState = 1
};
byte switcherState = PlayPresetState; // Select a Preset RunState by default.

// create an instance of the TFT library
TFT screen = TFT(CS, DC, RST);

// create an instance of the Encoder library
Encoder enc1(CLK, DT, SW);//Encoder pins INIT

//Menu configuration
const int menuSize = maxSupportedPedals;
String menuItems[menuSize];
bool upLastState = LOW;
bool downLastState = LOW;
int currentMenu = 0;
String temp;
char currentPrintOut [15];  //Output lenght

void setup()
{

  Serial.begin(19200);

  Serial.print("Starting setup()\n");

  Serial.print("Starting TFT screen. \n");

  screen.begin();
  // clear the screen with a black background
  screen.background(0, 0, 0);
  //set the text size
  screen.setTextSize(1);
  screen.stroke(0, 255, 0);
  int screenY = 0;
  screen.text("Init Loops.", 0, screenY);

  Serial.print("Assigning pin numbers. \n");

  loopSwitchOutputPins[0] = 34;
  loopSwitchOutputPins[1] = 35;
  loopSwitchOutputPins[2] = 36;
  loopSwitchOutputPins[3] = 37;
  loopSwitchOutputPins[4] = 38;
  loopSwitchOutputPins[5] = 39;
  loopSwitchOutputPins[6] = 40;
  loopSwitchOutputPins[7] = 41;

  Serial.print("Pins 33-40 Assigned. \n");
  Serial.print("Assigning pinMode to Output. \n");

  for(int index = 0; index < maxSupportedPedals; index++)
  {
    pinMode(loopSwitchOutputPins[index], OUTPUT);
  }

  Serial.print("Assigning Buffer control pin: \n");
  pinMode(BufferPIN, OUTPUT);

  Serial.print("Loading presets from EEPROM into presets list. \n");
  screenY = screenY + 10;
  screen.text("Loading Presets.", 0, screenY);

  for(int index = 0; index < maxSupportedPedals * maxSupportedPedals; index++)
  {
    presets[index] = EEPROM.read(index);

    Serial.print("Preset read from EEPROM. Adress: ");
    Serial.println(index);
    SerialPrintPreset(presets[index]);
  }
  Serial.print("64 presets being loaded. \n");
  screenY = screenY + 10;
  screen.text("Presets Loaded.", 0, screenY);

  //Loading preset emprty preset(bypass)
  Serial.print("Setting start preset. \n");
  screenY = screenY + 10;
  screen.text("Looper bypassed", 0, screenY);
  
  ApplyPreset(Bypass);

  Serial.print("Making menu items for default preset. \n");
  screenY = screenY + 10;
  screen.text("Init Menu.", 0, screenY);

  Serial.print("Starting MIDI. \n");
  screenY = screenY + 10;
  screen.text("Init MIDI", 0, screenY);
  MIDI.begin();

  Serial.print("Setup encoder \n");
  screenY = screenY + 10;
  screen.text("Init encoder.", 0, screenY);
  enc1.setTickMode(AUTO);
  enc1.setType(TYPE1);

  Serial.print("Reading buffer state: \n");
  screenY = screenY + 10;
  screen.text("Setup buffer.", 0, screenY);
  ApplyBufferState();
  
  Serial.print("Displaying default preset. \n");
  screenY = screenY + 10;
  screen.text("Welcome to loop switch.", 0, screenY);
  delay(1000);
  screen.background(0, 0, 0);
  //DisplayPreset(presets[presetCurrentIndex]);   //Use for displaying 0 preset
  DisplayByPassedScreen();
  Serial.print("Loop switch Ready. \n");    //Use for displaying "Bypassed message"
  DisplayHelpText();
  DisplayBufferState();
}

void loop()
{
  ReadMIDI();
  ReadEncoder();
}

void ReadMIDI()
{
  if (MIDI.read())                // Is there a MIDI message incoming ?
  {
    switch(MIDI.getType())        // Get the type of the message we caught
    {
    case midi::ProgramChange:     // If it is a Program Change,
      Serial.print("Received preset change to: ");
      presetCurrentIndex = MIDI.getData1();
      Serial.print(presetCurrentIndex);
      Serial.print("\n");
      LoadSelectedPreset();
      DisplayPreset(presets[presetCurrentIndex]);
      break;
            // See the online reference for other message types
    default:
      break;
    }
  }
}

void ReadEncoder()
{
  if (enc1.isHolded() && switcherState == PlayPresetState)
  {
    Serial.println("Entering Edit Mode");
    switcherState = EditPresetState;
    DisplayHelpText();
    EditPresetMenu();
    
  } 
  if (enc1.isSingle() && switcherState == PlayPresetState)
  {
    ApplyPreset(Bypass);
    Serial.println("Manually bypassed");
    DisplayByPassedScreen();
  }
  if (enc1.isDouble() && switcherState == PlayPresetState)
  {
    Serial.println("Switching buffer state");
    SwitchBufferState();
    //DisplayPreset(presets[presetCurrentIndex]);
    DisplayBufferState();
    }
}


void ApplyPreset(byte preset)
{
  Serial.print("Applying Preset: ");

  for(byte looperIndex = 0; looperIndex < maxSupportedPedals; looperIndex++)
  {
    // Getting the loops state:
    byte looperState = bitRead(preset, looperIndex);
    // Put the loops state into the corresponding output pins:
    digitalWrite(loopSwitchOutputPins[looperIndex], looperState);
  }
  SerialPrintPreset(preset);
}

void LoadSelectedPreset()
{
  Serial.print("Select Current Preset. Current Preset index: ");
  Serial.println(presetCurrentIndex);

  ApplyPreset(presets[presetCurrentIndex]);

  Serial.print("\n");
}

void EditPresetMenu()
{
  presetBeingEditted = presetCurrentIndex;  //Temporary assigning preset number for editing
  byte newPreset = presets[presetCurrentIndex];
  DisplayPreset(newPreset);
  int selectedLoop = 0;
  while(switcherState == EditPresetState)
  {
    if(enc1.isLeft() != upLastState)
    {
      enc1.resetStates();
      upLastState = !upLastState;
      if(!upLastState)
      {
        //Pressed UP Button
        if(currentMenu > 0)
        {
          currentMenu--;
          selectedLoop = currentMenu;
        }
        else
        {
          currentMenu = menuSize - 1;
          selectedLoop = currentMenu;
        }
        DisplayPreset(newPreset);
      }

    }

    if(enc1.isRight() != downLastState)
    {
      enc1.resetStates();
      downLastState = !downLastState;

      if(!downLastState)
      {
        //Pressed DOWN Button
        if(currentMenu < menuSize - 1)
        {
          currentMenu++;
          selectedLoop = currentMenu;
        }
        else
        {
          currentMenu = 0;
          selectedLoop = currentMenu;
        }
        DisplayPreset(newPreset);
      }

    }
    delay(10);
    if(enc1.isClick() && switcherState == EditPresetState)
    {
      Serial.println("Switch loop state.");
      Serial.print("Was: ");
      SerialPrintPreset(newPreset);
      boolean newLoopState = !bitRead(newPreset,selectedLoop);
      bitWrite(newPreset, selectedLoop, newLoopState); //menuLooperIndex is a selected looper within the menu
      Serial.print("Became: ");
      SerialPrintPreset(newPreset);
      //Serial.println("Updating menu item. ");
      //menuItems[currentMenu] = "Pedal " + String(currentMenu+1) + String(newLoopState ? " OFF" : " ON"); //relay controled with LOW signal

      //Applying preset for actual preview
      enc1.resetStates();
      ApplyPreset(newPreset);
      DisplayPreset(newPreset);
      
    }

    if(enc1.isHolded() && switcherState == EditPresetState)
    {
      Serial.println("Saving Preset.");
      if(presets[presetBeingEditted] != newPreset)
      {
        presets[presetBeingEditted] = newPreset;
        EEPROM.write( presetCurrentIndex, presets[presetCurrentIndex] );
        Serial.println("Preset successfully saved on EEPROM.");
        enc1.resetStates();

        //Informing user on display
        DisplaySavedScreen();
        DisplayHelpText();

      }
      else if(presets[presetBeingEditted] == newPreset)
      {
        Serial.println("No changes to preset were done.");
      }

      Serial.println("Entering Play Mode");
      switcherState = PlayPresetState;
      DisplayHelpText();
      enc1.resetStates();
      DisplayPreset(presets[presetCurrentIndex]);
      currentMenu = 0;
      //Clean up midi input buffer if messages were received during editing
      void MidiFlushBuffer();
      delay(1);
      break;
    }
  }
}

void MenuChanged()
{
  temp = String(menuItems[currentMenu]);
  temp.toCharArray(currentPrintOut, 15);
  screen.background(0, 0, 0);
  screen.stroke(0, 255, 0);
  screen.setTextSize(2);
  screen.text(currentPrintOut, 20, 30);
}

void DisplayPreset(byte preset)
{
  Serial.println("Displating graphical preset.");

  //Clean up preset ID
CleanUpPreset();
  
  screen.stroke(0, 255, 0);
  temp = "Preset:" + String(presetCurrentIndex);
  temp.toCharArray(currentPrintOut, 15);

  screen.setTextSize(3);
  screen.text(currentPrintOut, 0, 0);

  //Drawing loop groups
  screen.fill(0,0,0);
  screen.stroke(55,0,255);
  screen.rect(0, 50, 79, 50);
  screen.rect(80, 50, 80, 50);
  //Group Labels
  screen.setTextSize(1);
  screen.stroke(255, 0, 0);
  screen.text("FX Loop", 20, 85);
  screen.text("Input", 105, 85);
  //Drawing loops states
  int offset = 2;
  for (int i=0; i<maxSupportedPedals; i++)
  {
    //highlight selected pedal in edit mode
    if (i == currentMenu && switcherState == EditPresetState )
      screen.stroke(255,0,255);
    else
    // set the stroke color to white
      screen.stroke(255,255,255);

    // set the fill icon to grey if loop is ON
    if (!bitRead(preset, i))
      screen.fill(127,127,127);
    else
    // set the fill icon to black if loop is OFF
      screen.fill(0,0,0);


    // draw pedal icon
    screen.rect(offset, 60, 15, 20);
    // drow pedal knobs
    screen.fill(0,0,0);
    screen.stroke(255,255,0);
    screen.circle(offset + 3, 64, 1);
    screen.circle(offset + 11, 64, 1);
    screen.circle(offset + 7, 64, 1);
    screen.circle(offset + 7, 74, 2);
    offset = offset+20;    
  }
}

void DisplayBufferState()
{
    //clear previous state
    screen.stroke(0,0,0);
    screen.fill(0,0,0);
    screen.rect(78, 106, 160, 30);  
    screen.setTextSize(2);
    screen.stroke(155, 50, 200);
    temp = String(bufferState ? "" : " -[B]- ");
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 78, 106);
}

void DisplayHelpText()
{
    //Clean up
    screen.stroke(0,0,0);
    screen.fill(0,0,0);
    screen.rect(0, 32, 80, 15);  
    screen.rect(0, 105, 80, 20);  
    //Displaying help text
    if(switcherState == PlayPresetState)
    {   
    screen.setTextSize(1);
    screen.stroke(0, 255, 0);
    temp = "Double to";
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 0, 105);
    temp = "Switch buffer";
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 0, 115);
    temp = "Hold for edit";
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 0, 32);   
    }
    else if (switcherState == EditPresetState)
    {  
    screen.setTextSize(1);
    screen.stroke(0, 255, 0);
    temp = "Hold to";
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 0, 105);
    temp = "Save Preset";
    temp.toCharArray(currentPrintOut, 15);
    screen.text(currentPrintOut, 0, 115);     
    
    }
}

void ApplyBufferState()
{
  bufferState = bitRead(EEPROM.read(bufferAddr), 0);
  digitalWrite(BufferPIN, bufferState);
  Serial.println("Buffer is" + String(bufferState ? " OFF" : " ON"));
}

void SwitchBufferState()
{
  byte bufferByte = EEPROM.read(bufferAddr);
  bufferState = bitRead(bufferByte, 0);
  bufferState = !bufferState; //Switch buffer state
  bitWrite(bufferByte, 0, bufferState);
  EEPROM.write( bufferAddr, bufferByte );
  digitalWrite(BufferPIN, bufferState);
  Serial.println("Buffer switched to" + String(bufferState ? " OFF" : " ON"));
  DisplayBufferState();
}

void SerialPrintPreset(byte preset)
{
  Serial.print("Loaded preset: ");

  for(byte looperIndex = 0; looperIndex < maxSupportedPedals; looperIndex++)
  {
    byte looperState = bitRead(preset, looperIndex);
    Serial.print(looperState);
  }
  Serial.print("\n");
}

void MidiFlushBuffer()
{
  while(Serial1.available() && Serial.available() )
  {
  Serial1.read();
  Serial.read();
  }
}

void DisplayByPassedScreen()
{
  Serial.println("Displating bypass screen.");
  DisplayPreset(Bypass);  
  temp = "Bypassed";
  temp.toCharArray(currentPrintOut, 15);
  if(switcherState == PlayPresetState)  //Cleanup if invoked during Play Mode
  //Clean up preset ID
  CleanUpPreset();
  screen.stroke(0, 255, 0);
  enc1.resetStates();
  screen.setTextSize(3);
  screen.text(currentPrintOut, 0, 0);
}

void CleanUpPreset()
{
  //Clean up preset ID
  screen.stroke(0,0,0);
  screen.fill(0,0,0);
  screen.rect(0, 0, 160, 30);
}

void DisplaySavedScreen()
{
  CleanUpPreset();
  Serial.println("Displating saved preset message.");
  screen.stroke(0, 255, 0);
  temp = "Saved";
  temp.toCharArray(currentPrintOut, 15);
  screen.setTextSize(3);
  screen.text(currentPrintOut, 0, 0);
  enc1.resetStates();

  delay(200);
}
