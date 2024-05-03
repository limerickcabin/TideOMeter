/*
 * Server works as a simple web server serving files from SPIFFS. If the file exists
 * it is returned via http. In addition to serving files, it can respond to specific URI's
 * programmatically. 
 * 
 * In this application:
 * Browser accesses root 
 * Server serves index.html to browser
 */
#include <WebServer.h>
#define FS SPIFFS
#define logprintf Serial.printf
WebServer server(80);
int32_t debugTimer;

// register the callbacks for each URI
void serverSetup(void){
  //basic webserver handlers
  server.on("/",           handleRoot);		//root - serves the webpage
  server.on("/robots.txt", handleRobots);	//tells robots to ignore us
  server.on("/ls",         handleLS);     //ls -l of FS
  server.on("/del",        handleDelete); //del file from FS
  server.on("/edit", HTTP_POST, []() {server.send(200, "text/plain", "upload complete");}, handleFileUpload); //client upload file to FS
  server.onNotFound(       handleDefault); //default - handles file requests
  //application specific handlers
  server.on("/debug",      handleDebug);      //console access
  server.begin();

  logprintf("HTTP server started\n");
}

// called from the round robin loop to service http requests
void doServer(void) {
  debugTimer=millis();
  server.handleClient();
}

void resetServer(void) {
  server.close();
  serverSetup();
}

void streamFile(String filename) {
    uint32_t startMillis=millis();
    char ip[16];
    char fn[32];
    String filetype;

    if      (filename.endsWith(".html")) filetype="text/html";
    else if (filename.endsWith(".js"))   filetype="text/javascript";
    else if (filename.endsWith(".json")) filetype="text/json";
    else if (filename.endsWith(".ico"))  filetype="image/x-icon";
    else if (filename.endsWith(".png"))  filetype="image/png";
    else if (filename.endsWith(".jpg"))  filetype="image/jpg";
    else if (filename.endsWith(".xml"))  filetype="text/xml";
    else                                 filetype="text/plain";
    
    server.client().remoteIP().toString().toCharArray(ip,16);
    filename.toCharArray(fn,32);

    File f=FS.open(filename,"r");
    server.streamFile(f, filetype);
    f.close();

    logprintf("streamed file %s to %s in %ldms\n", fn, ip, millis()-startMillis);
}

// default handler that handles file requests: 
void handleDefault(){
  authenticate();
  //return the file if found
  String filename=server.uri();
  
  if (FS.exists(filename)) {
    streamFile(filename);
  }
  else {
    String message = "File Not Found\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    logprintf(message.c_str());
    //return;
    server.send(404, "text/plain", message);
  }
}

// specific handlers for specific http requests:
void handleRoot() {
    authenticate();
    streamFile("/index.html");
    return;
}

void handleDelete() {
  authenticate();
  String filename="/"+server.arg(0);
  if (FS.exists((char *)filename.c_str())) {
    FS.remove((char *)filename.c_str());
    server.send(200, "text/plain", filename+" deleted");
  }
  else server.send(200,"text/plain", filename+" not found");
}

void handleFileUpload() {
  authenticate();
  static File uploadFile;
  
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    upload.filename="/"+upload.filename;
    if (FS.exists((char *)upload.filename.c_str())) {
      FS.remove((char *)upload.filename.c_str());
    }
    uploadFile = FS.open(upload.filename.c_str(), "wb");
    logprintf("Upload: START, filename: %s\n",upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      logprintf("Upload: WRITE, Bytes: %d\n", upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    logprintf("Upload: END, Size: %d\n", upload.totalSize);
  }
}

void handleLS() {
  // ls -l of file system
  File f;
  String response="";
  File root = FS.open("/");
  while (f=root.openNextFile()) {
    response+=String(f.name())+"\t"+String(f.size())+"\n";
  }
  logprintf(response.c_str());
  server.send(200, "text/plain", response);
}

void handleRobots() {  
  logprintf("robots accessed\n");  
  server.send(200, "text/plain", "User-agent: *\nDisallow: /");
}

/*************************** Application Specific Handlers **********************************/
void handleDebug() {
  // http access to the RPN console
  // example: http://192.168.0.177/debug?n=1c sets capacity to 1
   authenticate();
   String response=server.arg(0)+"\n";

  //un-escape special characters - consider parsing for % and grab the next 2 chars
  response.replace("%2B", "+");               //there are probably others
  
  int i=0;
  while (char c=server.arg(0).charAt(i++)) {  //run each character through the console
    response+=String(console(c));             //gather responses
  }
  logprintf(response.c_str());
  server.send(200, "text/plain", response);
}

void authenticate(void) {
  return;
  if (!server.authenticate(creds.AP, creds.WPA)) { 
    return server.requestAuthentication(DIGEST_AUTH, "Custom Auth Realm", "Go Away");
  }
}

