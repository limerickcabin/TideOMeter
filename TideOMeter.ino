
/*
 * Tide-o-meter
 * Gets the tide information from the internet for any location
 * Displays the phase of the tide tide like a clock. 
 * High tide is on top. Low tide is on the bottom. Pointer moves around clockwise.  
 *
 * Runs on a Sparkfun ESP32 Thing
 */

#include "include.h" //avoids apparent Arduino bug that breaks up long lists of includes

uint32_t updateTimer;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.printf("\n**********************\n%s %s %s\n",__FILE__,__DATE__,__TIME__);
  //do a "SSID,WPA,KEY,TZ"S from the debug console to set the WiFi and tides credentials at least once 
  initSpiffs();
  readCreds();
  while (wifiConnect(true));   //try to get the time forever - no point to proceed otherwise
  initMotor();                 //setup the motor
  initTidesFromFile();
  serverSetup();
  wiggleMotor(20);
  updateTimer=time(NULL);
}

void loop() {
  doDebug();
  doUpdate();
  doServer();               //web server
  //doPWM();
  //delay(100);
}

void doUpdate() {
  if (testMode) doTest();
  else if (time(NULL)>updateTimer) {
    updateTimer+=secsPerJump;
    doPosition();             //calculate desired position
    doStepper();              //move there
    postTide();
  }
}

