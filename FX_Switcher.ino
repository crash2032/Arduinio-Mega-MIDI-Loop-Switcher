/*        Loop Switcher with MIDI Control
Allows to control one(or pre/post loops) using MIDI Controller. 
Preset changing performed after receiving Program Change message, but can be modified for any type of message.
Also Schematic can be modified for using with monetary switch. Additional Array with control pins must be initiated and monitored in this case.

MIDI listen requires to use Serial1(available in Arduino Mega) to avoid serial output issues. */

#include <GyverEncoder.h>
#include <TFT.h>
#include <SPI.h>
#include <MIDI.h>
#include <EEPROM.h> // Provides reading from and writing to EEPROM for persisting the Presets.
MIDI_CREATE_DEFAULT_INSTANCE();

#define LED 13  

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


const byte MaxSupportedPedals = 8;
byte Presets[ (MaxSupportedPedals * MaxSupportedPedals) ];
byte PresetCurrentIndex = 0; // Default Preset used. 
byte PresetBeingEditted = 0; // Preset under the editing


byte LoopSwitchOutputPins[ MaxSupportedPedals ];

//  Loop Switcher State
//  Play = Listen for MIDI and switch presets after MIDI PC message. 
//  Edit   = Clicking a encoder button toggles a looper's state while editting a preset.
enum SwitcherState
{
  PlayPresetState = 0, 
  EditPresetState = 1
};
byte SwitcherState = PlayPresetState; // Select a Preset RunState by default.

// create an instance of the TFT library
TFT TFTscreen = TFT(CS, DC, RST);
// create an instance of the Encoder library
Encoder enc1(CLK, DT, SW);//Encoder pins INIT

//Menu configuration
const int menuSize = MaxSupportedPedals;
String menuItems[menuSize];
bool upLastState = LOW;
bool downLastState = LOW;
int currentMenu = 0;
String temp;
char currentPrintOut [15];  //Output lenght

void setup() 
{

  Serial.begin(19200);

  Serial.print( "Starting setup()\n" );

  Serial.print("Starting TFT screen. \n");
  TFTscreen.begin();
  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  //set the text size
  TFTscreen.setTextSize(1);
  TFTscreen.stroke(0, 255, 0);
  TFTscreen.text("Init Loops.", 0, 0);
  
  Serial.print( "Assigning pin numbers. \n" );
  
  LoopSwitchOutputPins[ 0 ] = 33; 
  LoopSwitchOutputPins[ 1 ] = 34; 
  LoopSwitchOutputPins[ 2 ] = 35; 
  LoopSwitchOutputPins[ 3 ] = 36; 
  LoopSwitchOutputPins[ 4 ] = 37;
  LoopSwitchOutputPins[ 5 ] = 38;
  LoopSwitchOutputPins[ 6 ] = 39;
  LoopSwitchOutputPins[ 7 ] = 40;
  
  Serial.print( "Pins 33-40 Assigned. \n" );
  Serial.print( "Assigning pinMode to Output. \n" );

  for( int index = 0; index < MaxSupportedPedals; index++ )
  {
    pinMode( LoopSwitchOutputPins[index], OUTPUT );
  }

  Serial.print( "Loading presets from EEPROM into presets list. \n" );
  TFTscreen.text("Loading Presets.", 0, 10);

  for( int index = 0; index < 0 + (MaxSupportedPedals * MaxSupportedPedals); index++ )
  {
    Presets[ index ] = EEPROM.read( index );

    Serial.print( "Preset read from EEPROM. Adress: " ); 
    Serial.println( index ); 
    SerialPrintPreset( Presets[index] );
  } 
  Serial.print( "64 presets being loaded. \n" );
  TFTscreen.text("Presets Loaded.", 0, 20);

  Serial.print( "Setting start preset. \n" );
  TFTscreen.text("Loading default preset.", 0, 30);

  PresetCurrentIndex = 0;
  ApplyPreset(PresetCurrentIndex);
  
  Serial.print("Making menu items for default preset. \n");
  TFTscreen.text("Init Menu.", 0, 40);
  MakeMenuItemsFromPreset(Presets[PresetCurrentIndex]);

  Serial.print("Starting MIDI. \n");
  TFTscreen.text("Init MIDI", 0, 50);
  MIDI.begin();

  Serial.print("Setup encoder \n");
  TFTscreen.text("Init encoder.", 0, 60);
  enc1.setTickMode(AUTO);
  enc1.setType(TYPE1);
  
  Serial.print("Displaying default preset. \n");
  TFTscreen.text("Welcome to loop switch.", 0, 70);
  delay(1000);
  DisplayPreset(Presets[PresetCurrentIndex]);

  Serial.print( "Loop switch Ready. \n" );
}



void loop() 
{
  ReadMIDI();
  if (enc1.isHolded() && SwitcherState == PlayPresetState) 
    {
      Serial.println("Entering Edit Mode");    
      SwitcherState = EditPresetState;
      MenuChanged();
      EditPresetMenu();
    }
}

void ReadMIDI()
{
  if (MIDI.read())                // Is there a MIDI message incoming ?
    {
        switch(MIDI.getType())      // Get the type of the message we caught
        {
            case midi::ProgramChange:       // If it is a Program Change,
                Serial.print("Received preset change to: ");
                PresetCurrentIndex = MIDI.getData1();
                Serial.print(PresetCurrentIndex);
                Serial.print("\n");
                LoadSelectedPreset();
                DisplayPreset(Presets[PresetCurrentIndex]);
                BlinkLed(PresetCurrentIndex);  // blink the LED a number of times
                                            // correponding to the program number
                                            // (0 to 127, it can last a while..)
                break;
            // See the online reference for other message types
            default:
                break;
        }
    }
    
}


void ApplyPreset( const byte& preset )
{
  Serial.print( "Loading Preset: " );

  for( byte looperIndex = 0; looperIndex < MaxSupportedPedals; looperIndex++ )
  {
    // Getting the loops state: 
    byte looperState = bitRead( preset, looperIndex );
    // Put the loops state into the corresponding output pins: 
    digitalWrite( LoopSwitchOutputPins[looperIndex], looperState );
  }
  SerialPrintPreset( preset );
} 

void LoadSelectedPreset()
{
  Serial.print( "Select Current Preset. Current Preset index: " );
  Serial.println( PresetCurrentIndex );
  
  ApplyPreset( Presets[PresetCurrentIndex] );
  MakeMenuItemsFromPreset( Presets[PresetCurrentIndex] );

  Serial.print( "\n" );
}

void MakeMenuItemsFromPreset(const byte& preset)
{
  for(int i = 0; i < MaxSupportedPedals; i++)
  {
    Serial.println("Updating Menu Items.");
    Serial.print("Menu Item added: ");
    byte loopState = bitRead( preset, i );
    menuItems[i] = "Pedal " + String(i+1) + String(loopState ? " OFF" : " ON"); //relay controled with LOW signal
    Serial.print(menuItems[i]);
    Serial.print("\n");
  }
}

void EditPresetMenu()
{
  PresetBeingEditted = PresetCurrentIndex;  //Temporary assigning preset number for editing
  byte newPreset = Presets[PresetCurrentIndex];
  DisplayPreset(newPreset);
  int selectedLoop = 0;
  while(SwitcherState == EditPresetState)
  {
  
    if(enc1.isLeft() != upLastState)
    {
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
          }
          MenuChanged();
          DisplayPreset(newPreset);
      }

      }

      if(enc1.isRight() != downLastState)
      {
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
          }
          MenuChanged();
          DisplayPreset(newPreset);
      }

      }
      delay(10);
      if(enc1.isClick() && SwitcherState == EditPresetState)
      {
        Serial.println("Switch loop state.");
        Serial.print("Was: ");
        SerialPrintPreset(newPreset);
        boolean newLoopState = !bitRead(newPreset,selectedLoop);
        bitWrite(newPreset, selectedLoop, newLoopState); //menuLooperIndex is a selected looper within the menu
        Serial.print("Became: ");
        SerialPrintPreset(newPreset);
        Serial.println("Updating menu item. ");
        menuItems[currentMenu] = "Pedal " + String(currentMenu+1) + String(newLoopState ? " OFF" : " ON"); //relay controled with LOW signal
        
        temp = String(menuItems[currentMenu]);
        temp.toCharArray(currentPrintOut, 15);
        TFTscreen.background(0, 0, 0);
        TFTscreen.stroke(255, 0, 0);
        TFTscreen.setTextSize(2);
        TFTscreen.text(currentPrintOut, 20, 30);

        DisplayPreset(newPreset);

      }
      if(enc1.isHolded() && SwitcherState == EditPresetState)
      {
        Serial.println("Saving Preset.");
        if(Presets[PresetBeingEditted] != newPreset)
        {
        Presets[PresetBeingEditted] = newPreset;
        EEPROM.write( PresetCurrentIndex, Presets[PresetCurrentIndex] );
        Serial.println("Preset successfully saved on EEPROM.");
        }
        else
        if(Presets[PresetBeingEditted] == newPreset)
        Serial.println("No changes to preset were done.");
        Serial.println("Entering Play Mode");   
        SwitcherState = PlayPresetState;
        TFTscreen.background(0, 0, 0);
        DisplayPreset(Presets[PresetCurrentIndex]);
        return;
      }
  }
}

void MenuChanged()
{
  temp = String(menuItems[currentMenu]);
  temp.toCharArray(currentPrintOut, 15);
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(0, 255, 0);
  TFTscreen.setTextSize(2);
  TFTscreen.text(currentPrintOut, 20, 30);
}

void DisplayPreset(const byte& preset)
{
  Serial.println("Displating graphical preset.");
  TFTscreen.stroke(0, 255, 0);
  temp = "Preset: " + String(PresetCurrentIndex);
  temp.toCharArray(currentPrintOut, 15);
  if(SwitcherState == PlayPresetState)  //Cleanup if invoked during Play Mode
  TFTscreen.background(0, 0, 0);
  
  TFTscreen.setTextSize(3);
  TFTscreen.text(currentPrintOut, 0, 0);

  //Drawing loop groups
  TFTscreen.fill(0,0,0);
  TFTscreen.stroke(55,0,255);
  TFTscreen.rect(0, 50, 79, 50);
  TFTscreen.rect(80, 50, 80, 50);
  //Pre-Post Labels
  TFTscreen.setTextSize(1);
  TFTscreen.stroke(255, 0, 0);
  TFTscreen.text("Post", 30, 85);
  TFTscreen.text("Pre", 110, 85);
  //Drawing loops states
  int offset = 2;
  for (int i=0; i<MaxSupportedPedals; i++)
  {
    
  // set the stroke color to white
  TFTscreen.stroke(255,255,255);

  // set the fill icon to grey if loop is ON
  if (!bitRead(preset, i))
  TFTscreen.fill(127,127,127);
  else
  // set the fill icon to black if loop is OFF
  TFTscreen.fill(0,0,0);

  // draw a rectangle in the center of screen
  TFTscreen.rect(offset, 60, 15, 20);
  offset = offset+20;
  }
}


void SerialPrintPreset( const byte& preset )
{
  Serial.print( "Loaded preset: " );

  for( byte looperIndex = 0; looperIndex < MaxSupportedPedals; looperIndex++ )
  {
    byte looperState = bitRead( preset, looperIndex );
    Serial.print( looperState );
  }
  Serial.print( "\n" );
}

void BlinkLed(byte num)         // Basic blink function. For debug
{
    for (byte i=0;i<num;i++)
    {
        digitalWrite(LED,HIGH);
        delay(100);
        digitalWrite(LED,LOW);
        delay(100);
    }
}
