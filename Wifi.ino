char w_MDNS[16]="Stepper";

int wifiConnect(boolean updateTime) {
  //if (wifiEnabled==false) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(creds.AP,creds.WPA); 
  int count=0;
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    if (count++>20) {
      Serial.println("could not connect");
      return(1);
      //deepSleep();
    }
  }
  Serial.println("connected to accesspoint");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (updateTime) {
    //clear time so we can see NTP setting it
    timeval now;
    timerclear(&now);
    settimeofday(&now,NULL);
    configTime(0,0,"pool.ntp.org"); //starts NTP
  
    //if (MDNS.begin(w_MDNS)) Serial.println("MDNS responder started");
    //OTAsetup();
  
    int counter=20;
    Serial.print("getting time");
    while (counter--) {    // wait a while until NTP changes the time
      gettimeofday(&now, NULL);
      if (now.tv_sec > 40*365*24*60*60) { // time > 1970 + 40 years
        Serial.printf("got it\n");
        break; 
      }
      delay(1000);
      Serial.printf(".");
    }
    if (now.tv_sec < 40*365*24*60*60) {
      Serial.printf("did not get it\n");
      return(1);
    }
  }
  return(0);
}
