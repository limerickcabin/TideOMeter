void initSpiffs(void) {
    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    writeConsole();
}

#define CREDSFILE "/creds.txt"

void readCreds(void) {
  byte* ptr = (byte*) &creds;
  File fr=SPIFFS.open(CREDSFILE);
  fr.read(ptr,sizeof(Creds));
  fr.close();

  Serial.printf("creds.txt %s %s %s\n",creds.AP,creds.WPA,creds.KEY);
  for (int i=0; i<sizeof(Creds)-4; i++) {
    creds.CS-=*(ptr+i);
  }
  if (creds.CS==0) Serial.println("creds checksum OK");
  else Serial.println("creds checksum bad");
}

void writeCreds(Creds creds) {
  byte* ptr = (byte*) &creds;
  creds.CS=0;
  for (int i=0; i<sizeof(Creds)-4; i++) {
    creds.CS+=*(ptr+i);
  }
  File fw=SPIFFS.open(CREDSFILE,FILE_WRITE);
  fw.write(ptr,sizeof(Creds));
  fw.close();
}

void writeConsole(void) {
  #include "console.h"
  if (SPIFFS.exists(cfilename)) return;
  File fw=SPIFFS.open(cfilename,FILE_WRITE);
  int len=fw.write((byte*)consolestr,strlen(consolestr));
  fw.close();
  //Serial.println(consolestr);
  Serial.printf("console.html written to SPIFFS, %d bytes\n",len);
}
