/*
 * a simple reverse polish notation console
 * unsigned integers followed by a single character command
 * example 4000 5000+ results in 9000 (the space between the number pushed the first number on stack)
 * 4000,5000+1000+o sets the offset variable to 10000 (commas also push)
 * 4000,+ results in 8000 (the comma pushed 4000 onto the stack again)
 * so does 2000,2000+,+
 * Note: special characters like space and plus need to be escaped for the URL method
 *       html form posts automatically do it for you
 */

/******console******/
void debugInit(void) {
}

void doDebug(void) {
  char c;
  while (Serial.available()>0) {
    c=Serial.read();
    Serial.print(console(c));
  }
}

char accumChars[80]="";
char * console(char c) {
    static int32_t accumulator;
    static boolean lookingForFirstDigit=true;
    static char response[80];
    static boolean lookingForChars=false;

    response[0]=0;
    
    if (lookingForChars) { //accumulating a string
      if (c=='\"') {
        lookingForChars=false;
      }
      else {
        int index=strlen(accumChars);
        if (index<(80-2)) {
          accumChars[index++]=c;
          accumChars[index]=0;
        }
      }
      return(response);
    }
    
    if (c>='0' && c<='9') { //accumulating a number
      if (lookingForFirstDigit) {
        push(accumulator);
        accumulator=0;
        lookingForFirstDigit=false;
      }
      accumulator*=10;
      accumulator+=c-'0';
    }
    else { //got ourselves a letter
      lookingForFirstDigit=true; //next number we see, the accumulator will push and start over
      switch (c) {
        case '+':
          sprintf(response, "%ld\n",accumulator=pop()+accumulator);
          break;
        case '-':
          sprintf(response,"%ld\n",accumulator=pop()-accumulator);
          break;
        case '*':
          sprintf(response,"%ld\n",accumulator=pop()*accumulator);
          break;
        case '/':
          sprintf(response,"%ld\n",accumulator=pop()/accumulator);
          break;
        case 's': //change sign
          sprintf(response,"%ld\n",accumulator=-accumulator);
          break;
        case 'h': //hex
          push(accumulator);
          sprintf(response,"%ld\n",accumulator=strtol(accumChars,NULL,16));
          break;
        case '?': //show accumulators
          sprintf(response,"%ld\n",accumulator);
          sprintf(response,"%s\n",accumChars);
          break;
        case '\"':
          lookingForChars=true;
          accumChars[0]=0;
          break;
        case ',':
        case ' ':
        case '\n': 
        case '\r': //delimiting chars 
          break;
        default:
          otherCommands(c, &accumulator, response);
          break;
      }
    }
    return(response);
}

//hacky circular stack
int32_t stack[16];
uint8_t sp=0;
int32_t push(int32_t val) {
  stack[sp++]=val;
  sp&=0x0f;
  return(val);
}
int32_t pop(void) {
  sp=--sp&0x0f;
  return(stack[sp]);
}

/********** Application Specific Commands *********/
void otherCommands(char c, int32_t *accPtr, char* response) {
  char *SSID, *PSK, *KEY, *ZONE;
  int numCommas=0;
  
  switch (c) {
    case 'c': //move currentStep position
      //set current to where the pointer is observed and it will
      //step to desired (0)
      //TODO: changes in current may make discontinuous phase jumps
      sprintf(response,"Current position changed\n");
      currentStep=*accPtr;
      doStepper();
      *accPtr=pop();
      break;
    case 'p': //position
      sprintf(response,"currentStep %d\n",currentStep);
      break;
    case 'r':
      ESP.restart();
      break;
    case 't':
      testMode=!testMode;
      break;
    case 'S': //Save WiFi credentials. Usage: "ssid,psk,key,zone"S
      for (SSID=accumChars; *SSID != 0; SSID++) if (*SSID == ',') numCommas+=1; //count the commas
      if (numCommas==3) { // we know we won't go marching off the end of the string looking for commas
        //SSID
        SSID=accumChars;
        //PSK
        PSK=(char*)memchr(SSID,',',sizeof(Creds)); //find comma beginning at the start
        *PSK++=0; //terminate SSID string by overwriting the comma, advance pointer for PSK string
        //KEY
        KEY=(char*)memchr(PSK,',',sizeof(Creds));
        *KEY++=0;
        //ZONE
        ZONE=(char*)memchr(KEY,',',sizeof(Creds));
        *ZONE++=0;
        //Save it
        Serial.printf("credentials parsed: %s %s %s %s\n",SSID,PSK,KEY,ZONE);
        Creds c;
        strcpy(c.AP, SSID);
        strcpy(c.WPA,PSK);
        strcpy(c.KEY,KEY);
        c.TZ=    atoi(ZONE);
        writeCreds(c);
      }
      else sprintf(response,"SSID error, usage: \"ssid,psk,zone\"S\n");
      accumChars[0]=0;
      break;
    default:
      sprintf(response,"   unknown command %c\n",c);
      break;
  }
}
