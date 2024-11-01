#include <Servo.h>
#include <U8g2lib.h>

const int maxSpeed = 609; // Sets the maximum possible RPM (add 9 to whatever value you want)


const int INTERRUPT_PIN1 = 2;
const int INTERRUPT_PIN2 = 3;
volatile int interruptCount1;
volatile int interruptCount2;
int interruptCount2_ = 0;
float rpmReal[2] = {0, 0};
float rpmSlope = 6.60085;
float rpmbVal = 0;
int speed[2] = {0, 0};


const int pot = 0;
const int RPMSens = 1;

int RPMWant = 0;
int speedWant = 0;

int inputStats[] = {0, 0};
bool notWant = false;

byte motor1Pin = 9;
byte motor2Pin = 10;
Servo motor1;
Servo motor2;

U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0); // Display

void setup() {
  // display setup
  u8g2.setColorIndex(1);
  u8g2.begin();
  
  // rpm sensor
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN1), interruptFired1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN2), interruptFired2, CHANGE);

  motor1.attach(motor1Pin);
  motor2.attach(motor2Pin);
  motor1.writeMicroseconds(1500);
  motor2.writeMicroseconds(1500);

  delay(2000);
  Display();
}


void loop() {
  int sens = analogRead(pot);
  
  // Calculates wanted RPM using potentiometer
  int temp = map(sens, 0, 1023, 0, maxSpeed);

  if (RPMWant/10*10 != temp/10*10 && abs(RPMWant-temp) > 1 && temp != 0) {
    inputStats[0] = millis();
    inputStats[1] = temp;
    checkPot(inputStats);
    
  }

  if (notWant){
    notWant = false;
  }
  else {
    RPMWant = map(analogRead(pot), 0, 1023, 0, maxSpeed);
    speedWant = RPMWant;
    checkRPM();
  }
  

  


}

void blink(int time, int want) {
  unsigned long mill = millis()+time;
  unsigned long mill2;
  int potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed);

    while (millis()<mill) {
      
      // stays on for half a second
      mill2 = millis()+500;
      while ((want/10*10 == potVal/10*10 || abs(want-potVal) < 2) && millis()<mill2 && potVal!=0){
        checkRPM();
        potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed);
        
      }

      // turns off for 500 millis
      mill2 = millis()+500;
      while ((want/10*10 == potVal/10*10 || abs(want-potVal) < 2) && millis()<mill2 && potVal!=0){
        Display(true);        
        // for (int i:Dpins){
        //   digitalWrite(i,HIGH);
        // }
        potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed);
      }



      if ((want/10*10 != potVal/10*10 && abs(want-potVal) > 1) || potVal==0){
      RPMWant = potVal-10;
      notWant = true;
      return;
      }
      
      potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed);
  }
  


  RPMWant = potVal;
}

void checkPot(int inputStat[2]){
  int potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed)/10*10;
  unsigned long mill = millis()+2000;

  while ((inputStat[1]/10*10 == potVal || abs(inputStat[1]-potVal) < 2) && millis()<mill && potVal!=0) {
    checkRPM();
    potVal = map(analogRead(pot), 0, 1023, 0, maxSpeed)/10*10;
  }
  
  if ((inputStat[1]/10*10 != potVal && abs(inputStat[1]-potVal) > 1) || potVal==0) {
    RPMWant = potVal-10;
    notWant = true;
    return;
  }
  blink(5000, potVal);
}

void calcSpeed(int motor) {

  if (speedWant/10*10 == 0){
    speed[motor] = 0;
  }
  else{
    if (rpmReal[motor] < speedWant && speed[motor] < 400+speedWant-rpmReal[motor] && speed[motor]+speedWant-rpmReal[motor] > 0){
      speed[motor]+=roundOut((speedWant-rpmReal[motor])/10.0);
    }
    else if (rpmReal[motor] > speedWant && speed[motor] > 0){
      speed[motor]--;
    }
    else if (rpmReal[motor] > speedWant && speed[motor] < 400){
      speed[motor]++;
    }
  }
  if (motor == 0){motor1.writeMicroseconds(1500+speed[motor]);} // motor 1 speed
  else {motor2.writeMicroseconds(1500-speed[motor]);} // motor 2 speed
}

void checkRPM() {
  noInterrupts();
  interruptCount1 = 0;  // set variable in critical section
  interruptCount2 = 0;
  interrupts();
  dispDelay(100);
  noInterrupts();
  interruptCount2_ = interruptCount2;
  int critical_rpm1 = interruptCount1;  // read variable in critical section 
  int critical_rpm2 = interruptCount2;
  interrupts();
  rpmReal[0] = (critical_rpm1*rpmSlope)+rpmbVal;
  rpmReal[1] = (critical_rpm2*rpmSlope)+rpmbVal;

  calcSpeed(0); // calc speed for motor 1
  calcSpeed(1); // calc speed for motor 2

}

void Display() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);

  /* num1 */
  char str[10]; // convert int to string
  itoa((map(analogRead(pot), 0, 1023, 0, maxSpeed)/10*10), str, 10); 

  int width = u8g2.getStrWidth(str); // center screen
  u8g2.setCursor((128-width)/2, 17); // center screen
  u8g2.print(str);

  u8g2.drawBox(0, 22, 128, 2);
  u8g2.drawBox(63, 22, 2, 42);


  /* num2 */

  u8g2.setFont(u8g2_font_7x14_tf);
  str[10]; // convert int to string
  itoa(rpmReal[0], str, 10); 

  u8g2.setCursor(2, 38);
  u8g2.print(str);


  /* num3 */

  u8g2.setFont(u8g2_font_7x14_tf);
  str[10]; // convert int to string
  itoa(rpmReal[1], str, 10); 

  width = u8g2.getStrWidth(str); // Right screen
  u8g2.setCursor(128-width-4, 38); // Right screen
  u8g2.print(str);




  u8g2.sendBuffer();
  delay(100);
}

void Display(bool a) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tf);

  char str[10]; // convert int to string 

  int width = u8g2.getStrWidth(str);

  u8g2.drawBox(0, 22, 128, 2);
  u8g2.drawBox(63, 22, 2, 42);


  /* num2 */

  u8g2.setFont(u8g2_font_7x14_tf);
  str[10]; // convert int to string
  itoa(rpmReal[0], str, 10); 

  u8g2.setCursor(2, 38);
  u8g2.print(str);


  /* num3 */

  u8g2.setFont(u8g2_font_7x14_tf);
  str[10]; // convert int to string
  itoa(rpmReal[1], str, 10); 

  width = u8g2.getStrWidth(str); // Right screen
  u8g2.setCursor(128-width-4, 38); // Right screen
  u8g2.print(str);

  u8g2.sendBuffer();
  delay(100);
}


void interruptFired1()
{
  interruptCount1++;
}
void interruptFired2()
{
  interruptCount2++;
}

void dispDelay(int time){
  unsigned long mill = millis()+time;
  while (millis()<mill){
    Display();
  }
}

inline double roundOut(double x)
{
  return x < 0 ? floor(x) : ceil(x);
}
