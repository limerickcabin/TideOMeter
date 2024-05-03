#include <WiFi.h>
#include <HTTPClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson-v6.19.4.h"

//globals
struct Creds {  //used for wifi access and http login
  char     AP[16];  //access point SSID
  char     WPA[16]; //access point key
  char     KEY[40]; //worldtides api key
  int      TZ;      //timezone
  uint32_t CS;
} 
creds;


#define secsPerJump (1*60)
#define secondsPerPost (60*60)
#define phaseMask 0xfffffffc
uint32_t phaseShift=0;

uint32_t torque=2;                          //20% torque
bool testMode=false;

String locString = "lat=20.75&lon=-105.5";  //Punta Mita

//RTC globals survive sleeps: (not necessary now that we are not sleeping (and store the json in SPIFFS))
#define checksum  (((int64_t)currentStep+desiredStep+sleepTime+timeToGetTime)^0xb08f06b2843978de)
RTC_DATA_ATTR uint32_t currentStep=0;      //where the motor is
RTC_DATA_ATTR uint32_t desiredStep=0;      //where we want the motor
RTC_DATA_ATTR uint32_t sleepTime;          //microseconds sleep
RTC_DATA_ATTR uint32_t timeToGetTime=0;    //time to connect and NTP
RTC_DATA_ATTR uint32_t timeToPost=0;
//RTC_DATA_ATTR int32_t  fudge=initialFudge; //correction factor to correct sleep time
RTC_DATA_ATTR timeval  timeBeforeSleep;    //time just before going to sleep
RTC_DATA_ATTR int64_t  rtcChecksum=0;
RTC_DATA_ATTR char     rtcBuf[80]="";
