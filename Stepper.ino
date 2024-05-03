/*
 * Stepper motor I got on Amazon: 
 * https://www.amazon.com/dp/B00W8XJ6RK
 * https://www.omc-stepperonline.com/round-nema-14-bipolar-0-9deg-7ncm-9-91oz-in-0-5a-8-5v-36x12mm-4-wires-14hr05-0504s
 *
 * Controller: 
 * https://www.amazon.com.mx/dp/B09MJ4XPXD
 *
 *
 */
 
//pins going to the motor controller
#define ain1 16
#define ain2 17
#define bin1 18
#define bin2 19
#define stby 12
#define pwma 13
#define pwmb 14

//spec sheet shows only 4 phases - driving the a and b coils all the time (one way or the other)
//msbit to lsbit: ain1,bin1 (ain2 and bin2 are opposite)
//const uint8_t phaseArray[4]={0b11,0b01,0b00,0b10};
const uint8_t phaseArray[4]={0b00,0b10,0b11,0b01}; //the motor likes to jump to 0b00 phase at rest

//steps, ms max rate, direction
#define stepsPerRotation 400
#define msBetweenSteps 50
#define mintorque 1

hw_timer_t * timer = NULL;

//PWM in the background - called by the timer
//torque controls the time driving the coils - 1 being 1/10th
void ARDUINO_ISR_ATTR onTimer(){
  static uint32_t pwmCounter=0;
  static uint32_t state;
  state=++pwmCounter%10;
  if (state==0) {
    digitalWrite(pwma,HIGH); //driving
    digitalWrite(pwmb,HIGH); //driving
  }
  if (state==torque) {
    digitalWrite(pwma,LOW); //not driving
    digitalWrite(pwmb,LOW); //not driving
  }
}

void initMotor() {
  pinMode(ain1, OUTPUT);
  pinMode(ain2, OUTPUT);
  pinMode(bin1, OUTPUT);
  pinMode(bin2, OUTPUT);
  pinMode(stby, OUTPUT);
  pinMode(pwma, OUTPUT);
  pinMode(pwmb, OUTPUT);

  digitalWrite(ain1,LOW);  //high impedance
  digitalWrite(ain2,LOW);  
  digitalWrite(bin1,LOW);  
  digitalWrite(bin2,LOW);  
  digitalWrite(stby,HIGH); //enabled
  digitalWrite(pwma,HIGH); //driving
  digitalWrite(pwmb,HIGH); //driving

  //setup the PWM timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 125, true); //microseconds
  timerAlarmEnable(timer);
}

//foreground method to PWM the motor
void doPWM(void){
  static uint32_t pwmCounter=0;
  static uint32_t state;
  state=++pwmCounter%10;
  if (state==0) {
    digitalWrite(pwma,HIGH); //driving
    digitalWrite(pwmb,HIGH); //driving
  }
  else if (state==torque) {
    digitalWrite(pwma,LOW); //not driving
    digitalWrite(pwmb,LOW); //not driving
  }
}

void doPosition(void) {
  float rotation=getTide();
  if (rotation>0) {
    desiredStep=int(rotation*stepsPerRotation+0.5);
    //land on resting phase (the one that is quietest)
    desiredStep&=phaseMask;
    desiredStep+=phaseShift;
    desiredStep%=stepsPerRotation;
  }
}

//move the motor to desiredStep
void doStepper(void)  {
  static bool firstTime=true;

  if (firstTime) {
    firstTime=false;
    currentStep=desiredStep;  //assume we were in the right spot
  }

  desiredStep%=stepsPerRotation;
  if (desiredStep!=currentStep) Serial.printf("desired %ld current %ld\n",desiredStep,currentStep);

  //drive full torque while moving
  digitalWrite(pwma,HIGH);
  digitalWrite(pwmb,HIGH);
  torque=10;                 //if using background PWM

  //step motor to desired
  while (currentStep!=desiredStep) {
    currentStep+=1;
    currentStep%=stepsPerRotation;
    stepMotor(currentStep);
  } //done moving
  
  torque=mintorque;

  //Serial.printf("desired %ld current %ld\n",desiredStep,currentStep);
}

void wiggleMotor(int steps) {
  //drive full torque while moving
  digitalWrite(pwma,HIGH);   //if using foreground PWM
  digitalWrite(pwmb,HIGH);
  torque=10;                 //if using background PWM
  //move
  int i,step;
  for (i=step=0;i<steps;i++) {
    stepMotor(--step);
  }
  for (i=step=0;i<steps*2;i++) {
    stepMotor(++step);
  }
  for (i=step=0;i<steps;i++) {
    stepMotor(--step);
  }
  //done moving
  torque=mintorque;
}

void stepMotor(int i) {
    bool a=phaseArray[i&3]&0b10;
    bool b=phaseArray[i&3]&0b01;
    digitalWrite(ain1, a);
    digitalWrite(bin1, b);
    digitalWrite(ain2,!a);
    digitalWrite(bin2,!b);
    delay(msBetweenSteps);
}

#define msPerJump (60000 / stepsPerRotation * 5)

//goes around once a minute for testing whether PWMing in necessary between jumps
//test does show that unless you start and stop on the "resting" phase, it will not move as desired
void doTest() {
  static uint32_t then=0;
  uint32_t now=millis();
  if (then==0) then=now;
  if (now>then) {
    then+=msPerJump;
    desiredStep+=5;
    desiredStep%=stepsPerRotation;
    doStepper();
    torque=0;
  }
}
