#include <DHT.h>
#include <EEPROM.h>
/*
 * This is programmed to do a home automation which can connect to touch switch and computer to communitate the status of pins.
 * 
 * We are using Arduino Mega for this project
 * Touch Switch - 2 Pins (A4 and A5)
 * Relay Driver - 8 Pins (D2 - D9)
 * 
 * Date: 12/Feb/2917
 * Author: Ponraj Suthanthiramani
 */
#define SCL_PIN A5
#define SDO_PIN A4
DHT dht;

//[Micro Controller Name | Total Touch Switch | Touch Switch Sensor | Digital Temerature Sensor | Light Sensor | PIR Motion Sensor | First name | Last Name | Customer ID | Customer Email | Product Version | License Key]
const String SYS_DATA = "{'SystemName': 'Arduino Uno R3','TotalSwitch': '8','Sensor': ['TouchSensor', 'DTHSensor', 'PIRMotionSensor'],'FirstName': 'Ponraj','LastName':'Suthanthiramani','CustomerID':'1001','CustomerEmail':'mylife.ponraj@gmail.com','CustomerLicense':'56DA84-581EE0-8FF47B-FB81B6-555F14-6CFE48-0625'}";

byte touchSensorData;

/*Relay output Key Configuration (8 Channel Relay)*/
byte relayControlPins[] = {2, 3, 4, 5, 6, 7, 8, 9};

/* Total Relay count */
byte pinCount = 8;

//Configure DTH 11 Pin (Make Sure it is a Ditital Pin)
int DTH_11_PIN = 10;

/**https://wiki.eprolabs.com/index.php?title=TTP229_Capacitive_Touch_Module TTP Capacity Touch module with 8 bit Congiruation) */
byte reduceCount = 9;

//We have this to hold the status of output which drives the relay
int digitalPinStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//We have this to preserve last status of the button
int digitalPinLastStatus[] = {0, 0, 0, 0, 0, 0, 0, 0};

//Configure delay in main loop NOTE: Smaller you have better the touch works
const int timerDelay = 200;

byte index = 0;

/* This flage is used to read Temp */
byte readTemp = 0;

byte EPROM_INITILIZE = 1;

boolean stringComplete = false;  // whether the string is complete

//Configure Command Array
/**
 * GSS => GET System Status
 * GCS => Get Current Status
 * SST => Set Switch To SST 1 0
 * 
 */
const String commands[] = {"GSS", "GSI", "GCS", "SST", "TMP"};

String inputString = "";

// we start, assuming no motion detected
int PIR_PIN = 12; 
int pirState = LOW;
int pirVal = 0;

void setup()
{
  /* Initialize the serial interface */
  Serial.begin(9600);
  /* Configure the clock and data pins */
  pinMode(SCL_PIN, OUTPUT);  
  pinMode(SDO_PIN, INPUT); 
  for (int relayPin = 0; relayPin < pinCount; relayPin++) {
    pinMode(relayControlPins[relayPin], OUTPUT);
    digitalWrite(relayControlPins[relayPin], HIGH);
  }
  
  //Step 4: Setup DTH Sensor Pin
  dht.setup(DTH_11_PIN);
  pinMode(PIR_PIN, INPUT);
}
/* Main loop */
void loop()
{
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
    index = touchSensorData - reduceCount;
    if(digitalPinLastStatus[index] == 0) {
      toggleStatusAndSetSwitch(index);
      digitalPinLastStatus[index] = 1;
    } else {
      digitalPinLastStatus[index] = 0;
    }
  }
  pirVal = digitalRead(PIR_PIN);  // read input value
  if (pirVal == HIGH) {            // check if the input is HIGH
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
  } else {
    if (pirState == HIGH){
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
  // Step 2: Send to serial communicator
  if (stringComplete) {
    processCommands(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  delay(timerDelay);
//  if(readTemp < 20) {
//    readTemp++;
//  }
//  else {
//    readTemp = 0;
//    float humidity = dht.getHumidity();
//    float temperature = dht.getTemperature();
//    Serial.print("[");
//    Serial.print(dht.getStatusString());
//    Serial.print("|");
//    Serial.print(dht.getHumidity(), 1);
//    Serial.print("|");
//    Serial.print(dht.getTemperature(), 1);
//    Serial.print("|");
//    Serial.print(dht.toFahrenheit(temperature), 1);
//    Serial.println("]");
//  }
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void processCommands( String query ) {
  String cmd = query.substring(0,3);
  //GSS (Get System Status)
  if(commands[0] == cmd) {
    Serial.println("STATUS: Connected.");
  }
  //GSI
  else if(commands[1] == cmd) {
    Serial.println(SYS_DATA);
  }
  //GCS (Get Current Status)
  else if(commands[2] == cmd) {
    String response = "{";
    for ( int index = 0 ; index < pinCount ; index ++ ) {
      if( digitalPinStatus[index] ) {
        response += "[";
        response += (index+1);
        response += "|ON]";
      }
    }
    response += "}";
    Serial.print("STATUS:");
    Serial.println(response);
  }
  //SST Set Switch To
  else if(commands[3] == cmd) {
    int pinToSet = query.substring(4, 5).toInt();
    int setValue = query.substring(6, 7).toInt();
    pinToSet = pinToSet - 1;
    digitalPinStatus[pinToSet] = setValue;
    setRelay(pinToSet);
    Serial.println("Set Successful.");
  }
  //TMP (Get Temprature
  else if(commands[4] == cmd) {
      //delay(dht.getMinimumSamplingPeriod());
      float humidity = dht.getHumidity();
      float temperature = dht.getTemperature();
      Serial.print("[");
      Serial.print(dht.getStatusString());
      Serial.print("|");
      Serial.print(dht.getHumidity(), 1);
      Serial.print("|");
      Serial.print(dht.getTemperature(), 1);
      Serial.print("|");
      Serial.print(dht.toFahrenheit(temperature), 1);
      Serial.println("]");
  }
  else {
    Serial.print("Command not Recogonized.");
    Serial.println(cmd);
  }
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
  Serial.print( "[" );
  Serial.print( index + 1 );
  Serial.print( "|");
  Serial.println( ( digitalPinStatus[index] ) ? "ON]" : "OFF]" );
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
