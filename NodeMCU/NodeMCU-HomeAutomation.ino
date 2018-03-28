#include <EEPROM.h>
#define SCL_PIN D0
#define SDO_PIN D1
byte EPROM_INITILIZE = 1;

byte touchSensorData;
/**https://wiki.eprolabs.com/index.php?title=TTP229_Capacitive_Touch_Module TTP Capacity Touch module with 8 bit Congiruation) */
byte reduceCount = 9;

byte indexVal = 0;

/*Relay output Key Configuration (8 Channel Relay)*/
byte relayControlPins[] = {D2, D3, D4, D6, D5, D7, D8, D9};

/* Total Relay count */
byte pinCount = 8;

//We have this to hold the status of output which drives the relay
int digitalPinStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//We have this to preserve last status of the button
int digitalPinLastStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//Configure delay in main loop NOTE: Smaller you have better the touch works
const int timerDelay = 200;

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  pinMode(SCL_PIN, OUTPUT);  
  pinMode(SDO_PIN, INPUT); 

  for (int relayPin = 0; relayPin < pinCount; relayPin++) {
    pinMode(relayControlPins[relayPin], OUTPUT);
    digitalWrite(relayControlPins[relayPin], HIGH);
  }
}

void loop() {
  if(EPROM_INITILIZE==1) {
    EPROM_INITILIZE = 0;
    for (int index = 0 ; index < 8 ; index++) {
      if(EEPROM.read(index)==1) {
        digitalWrite(relayControlPins[index], LOW);
      }
    }
  }
  /* Read the current state of the keypad */
  touchSensorData = Read_Keypad();
  /* If a key has been pressed output it to the serial port */
  if (touchSensorData) {
    indexVal = touchSensorData - reduceCount;
    if(digitalPinLastStatus[indexVal] == 0) {
      toggleStatusAndSetSwitch(indexVal);
      digitalPinLastStatus[indexVal] = 1;
    } else {
      digitalPinLastStatus[indexVal] = 0;
    }
  }
  delay(timerDelay);
}
/**
 * Toggle the switch status and set the relay as the status
 */
void toggleStatusAndSetSwitch(int index) {
  //Step 1: Toggling the current status
  digitalPinStatus[index] = ! digitalPinStatus[index];

  //Step 2: Set the relay driver to the current status
  setRelay(index);

  //Step 3: Communicate to the computer or further tracking
  /*
   * Computer Format
   * [PinNo | Status]
   * eg: 
   * If the pin 1 toggles to on
   * [1|ON]
   * If the pin 1 toggles to off
   * [1|OFF]
   * 
   */
//  Serial.print( "[" );
//  Serial.print( index + 1 );
//  Serial.print( "|");
//  Serial.println( ( digitalPinStatus[index] ) ? "ON]" : "OFF]" );
}
void setRelay(int index) {
  if(digitalPinStatus[index] == 0) {
    digitalWrite(relayControlPins[index], HIGH);
    EEPROM.write(index, 0);
  }
  else {
    digitalWrite(relayControlPins[index], LOW);
    EEPROM.write(index, 1);
  }
}
/* Read the state of the keypad */
byte Read_Keypad(void)
{
  byte Count;
  byte Key_State = 0;
  /* Pulse the clock pin 16 times (one for each key of the keypad) 
  and read the state of the data pin on each pulse */
  for(Count = 1; Count <= 16; Count++)
  {
    digitalWrite(SCL_PIN, LOW); 
    /* If the data pin is low (active low mode) then store the 
    current key number */
    if (!digitalRead(SDO_PIN))
      Key_State = Count; 
    digitalWrite(SCL_PIN, HIGH);
  }  
  return Key_State; 
}
