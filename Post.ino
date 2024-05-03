HTTPClient tideHTTP; 

void postTide(void) {
  //let the world know we are alive occasionally
  uint32_t now=time(NULL);
  if (now>timeToPost) {
    if (wifiConnect(false)==0) {
      String json="{!time!:!timestr!,!tide!:!world!}";
      json.replace("!","\"");
      json.replace("timestr",String(now));
      json.replace("world",String(rtcBuf));
      if (httpJSON(json)==0)
        timeToPost=now+secondsPerPost-30;
    }
  }
}

int httpJSON(String json) {
    int returnCode=0;
    int getCode;
    json.replace("%","%25");
    json.replace(" ","%20");
    String url="https://delongiot.com/appendJSON.php?file=/tide.dat&row="+json;
    tideHTTP.begin(url);
    getCode=tideHTTP.GET();
    if (getCode == HTTP_CODE_OK) {
      Serial.println(json+" posted");
    }
    else {
      Serial.printf("POST UNSUCCESSFUL: %d\n",getCode);
      returnCode=1;
    }
    tideHTTP.end();
    return(returnCode);
}
