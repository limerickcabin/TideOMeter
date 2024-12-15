/*
 * 
 * Methods to retrieve and process tide information from the web
 * I used to use rapidapi but now use worldtides.info. A 7-day query 
 * costs 1 credit. I bought 20,000 credits for $10. We get 3-days at a time so it should
 * last >100 years. Example fetch:
 * http://www.worldtides.info/api/v3?extremes&start=1670720000&lat=20.75&lon=-105.5&days=7&key=YOUR_WORLDTIDES_KEY
 *
 */

//jsonBuf holds the payload (json string) downloaded from API; doc holds the deserialized JSON object
#define BUFSIZE 16000
char jsonBuf[BUFSIZE];
DynamicJsonDocument doc(6000);


//tideArray has the time of the high and low tides
//not really necessary to hold in RTC now that we store the JSON tide information in SPIFFS
#define ARRAYSIZE 40
RTC_DATA_ATTR int64_t tideArray[ARRAYSIZE];
RTC_DATA_ATTR int64_t tideChecksum=0;
RTC_DATA_ATTR uint32_t tideArrayNum;
#define CHECKSUM ((tideArray[0]+tideArray[ARRAYSIZE-1])^0xb08f06b2843978de)

#define DEBUG if (true) Serial.printf

HTTPClient http; 

/*
finds next and previous tide in array
returns: 
  >0 representing where the pointer should be
    0-0.5 is a high->low tide transition; 0.5->1.0 is a low->high tide transition
  <0 means an error getting the data
*/
float getTide() {
  uint64_t now,next,previous;
  uint32_t i;
  int32_t tide=0;
  float rotation=0;

  //if array not initialized, initialize
  if (CHECKSUM!=tideChecksum) if (getArray()) return(-1);

  now=time(NULL);

  //is array current?
  if (abs(tideArray[tideArrayNum-1])<now) {
    Serial.printf("out of data\n");
    if (getArray()) return(-1);           //abort if getArray is unsuccessful
  }

  //step through array and find relevant tides
  for (i=0;i<tideArrayNum;i++) {
    next=abs(tideArray[i]);
    tide=tideArray[i]>0;                 //true if high tide is next
    if (now<next){
      //DEBUG("next tide found, index %d\n",i);
      previous=abs(tideArray[i-1]);
      float completed=(float)
         (now-previous)/(next-previous); //transition completed    (0.0-1.0)
      rotation=completed/2;              //stepper motor location  (0.0-0.5)
      if (tide) rotation+=0.5;           //low to high on the left (0.5-1.0)
      sprintf(rtcBuf,"%s is in %.1f hours %.0f%% there %.0f 60th around",
              tide?"High tide":"Low tide",
              (next-now)/3600.0,
              100*completed,
              60*rotation);
       //Serial.println(rtcBuf);
       break;
    }
  }
  return(rotation);               //successfully processed
}

//connect and fill jsonBuf with json string downloaded from tides API
//parse jsonBuf filling RTC RAM array with time of tides
int getArray() {
  int returncode=0;

  if (wifiConnect(false)==0 && httpJSON(jsonBuf)==0) {
    writeTidesToFile(jsonBuf);
    jsonParse(jsonBuf);
  }
  else {
    returncode=1;
  }

  //WiFi.disconnect();
  return(returncode);
}

//buf contains json string with tide information
//parse "extremes" array and store time of high/low tides in RTC RAM
//RTC RAM survives sleeps
int jsonParse(char* buf){
  DeserializationError error = deserializeJson(doc, buf);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return(0);
  }

  int i=0;
  int64_t timestamp;
  const char* state;
  
  for (i=0;i<ARRAYSIZE;tideArray[i++]=0); //clear array

  for (i=0;i<ARRAYSIZE;i++) {
    //step through extremes json array and populate tideArray
    timestamp = doc["extremes"][i]["dt"]; //returns 0 if it does not exist
    if (!timestamp) break;                //break if done parsing
    state = doc["extremes"][i]["type"];
    if (strcmp(state,"Low")==0) {
      timestamp=-timestamp; //negative indicates low tide
      //Serial.println("low");
    }
    tideArray[i]=timestamp;
  }
  Serial.printf("parsed %d json elements\n",i);
  tideArrayNum=i;
  tideChecksum=CHECKSUM;
  return(i);
}

//fill buf with json String retrieved from worldtides using http
int httpJSON(char* buf) {
    int errorcode=1;

    uint32_t now=time(NULL);

    String duration  = String(7);                         //days
    String timestamp = String(now-24*3600);               //starting yesterday
    String host      = "http://www.worldtides.info/api/v3?extremes";
    String key       = String(creds.KEY);
    String url=host+  "&start="+timestamp+  "&"+locString+  "&days="+duration+  "&key="+key;
    //Serial.println(url);

    http.begin(url);
    int httpError = http.GET();

    if (httpError == HTTP_CODE_OK) {
      Serial.println("https get successful");
      String payload = http.getString();
      payload.toCharArray(buf,BUFSIZE);
      //DEBUG("%s\n",buf);
      errorcode=0;
    }
    else {
      if (httpError > 0) Serial.printf("HTTP GET SERVER ERROR: %d\n",httpError);
      if (httpError==-1) Serial.printf("HTTPC_ERROR_CONNECTION_REFUSED (-1)\n");
      else Serial.printf("HTTP GET ERROR %d\n",httpError);
      errorcode=1;
    }
    http.end();
    return(errorcode);
}

#define tidesfilename "/tides.json"

//write json string to SPIFFS
void writeTidesToFile(char* json) {
    //remember the json string so we don't need to retrieve it from the web everytime we reboot, sleep or crash
    File fw=SPIFFS.open(tidesfilename,FILE_WRITE);
    fw.print(json);
    fw.close();

}

//read and parse the json string stored in SPIFFS
void initTidesFromFile(void){
  File fr=SPIFFS.open(tidesfilename);
  if (fr) {
    int length=fr.read((uint8_t*)jsonBuf,BUFSIZE);
    if (length>0) {
      //Serial.println(jsonBuf);
      Serial.printf("json tide file exists, parsing %ld bytes\n", length);
      jsonParse(jsonBuf);
    }
  }
}
