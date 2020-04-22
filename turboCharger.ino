#include<ESP32Servo.h>
#include <Button.h>

#include<Preferences.h>

#include <WiFi.h>
#include <DNSServer.h>
#include <ESP32WebServer.h>

Preferences preferences;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP32WebServer webServer(80);

//------------------configuration-----------------------
int motor_pin =  4;
int swtch_pin = 15;
int fadingInterval = 2;

int switchMode = 0;
int maxThotle = 1600;
int minThotle = 1540;
int startupThotle = 1540;

boolean stringComplete  =     false;    // whether the string is complete on Serial
String inputString      =     "";

int currentVal = 0;

#ifdef ESP32_Servo_h
Servo motor;
#else
int pwmFrequency = 50;
int pwmResolution = 10;
int pwmChanel = 0;
#endif

void saveConfig() {
  preferences.begin("config", false);
  preferences.putUInt("motor_pin", motor_pin);
  preferences.putUInt("swtch_pin", swtch_pin);
  preferences.putUInt("switchMode", switchMode);
  preferences.putUInt("maxThotle", maxThotle);
  preferences.putUInt("minThotle", minThotle);
  preferences.putUInt("startupThotle", startupThotle);
  preferences.end();
  Serial.println("Restarting..............");
  ESP.restart();
}

#include "webpage.h"
//---------------end of configuration-------------------

TaskHandle_t fadeControlTask; // Task 5

//-----start of button function-----------------
#ifdef BUTTON
TaskHandle_t buttonTask;      // Task 3
Button sw(swtch_pin, DIO);

void press(int sender) {
#ifdef ESP32_Servo_h
  motor.writeMicroseconds(maxThotle);
  printInfo();
#endif
}

void release(int sender) {
#ifdef ESP32_Servo_h
  motor.writeMicroseconds(startupThotle);
  printInfo();
#endif
}

#endif
//-----------end of button function-------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  //read configuration...............
  preferences.begin("config", false);

  motor_pin =  preferences.getUInt("motor_pin", motor_pin);
  swtch_pin = preferences.getUInt("swtch_pin", swtch_pin);
  switchMode = preferences.getUInt("switchMode", switchMode);
  maxThotle = preferences.getUInt("maxThotle", maxThotle);
  minThotle = preferences.getUInt("minThotle", minThotle);
  startupThotle = preferences.getUInt("startupThotle", startupThotle);
  preferences.end();

  Serial.println("----------read from Config");
  Serial.println("minPwm->" + String(minThotle));
  Serial.println("maxPwm->" + String(maxThotle));
  Serial.println("startupPwm->" + String(startupThotle));
  Serial.println("motorPin->" + String(motor_pin));
  Serial.println("SwitchPin->" + String(swtch_pin));
  Serial.println("Turbo Func.->" + String(switchMode));

  //---------------------------------

#ifdef BUTTON
  //set callback function for Button

  sw.eventPress((void*)press);
  sw.eventRelease((void*)release);

  xTaskCreate(&Button_Task, "Button_Task", 2048, NULL, 1, &buttonTask);
#endif

  //xTaskCreate(&FadeControl_TASK, "fadeControltask", 2048, NULL, 1, &fadeControlTask);


#ifdef ESP32_Servo_h
  motor.attach(motor_pin);
  motor.writeMicroseconds(minThotle);
  Serial.println("Starting....");
  delay(2000);
  motor.writeMicroseconds(startupThotle);
  printInfo();
#else
  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  ledcSetup(pwmChanel, pwmFrequency, pwmResolution); // 50Hz PWM, 10-bit resolution
  // Initialize channels
  // channels 0-15
  ledcAttachPin(motor_pin, pwmChanel);
#endif


  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("TurboCharger");

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  initWebServer80();
  Serial.println("Started...");

}
#ifdef BUTTON
void Button_Task(void *p) { //loop2
  while (1) {
    sw.handleButton();
    delay(1);
  }
}
#endif
void FadeControl_TASK(void *p) { //loop5
  while (1) {
    //----------------
    //Task ควบคุมหลอดไฟ......
    for (;;) {

    }
    delay(fadingInterval);
    //Serial.println("fade running...");
  }
}

#ifdef ESP32_Servo_h
void printInfo() {
  Serial.print("[Degree : " + String(motor.read()) + ", ");
  Serial.println("Pulse : " + String(motor.readMicroseconds()) + "]");
}
#endif

void loop() {
  webServer.handleClient();
  // handle serial input
  serialEvent();
  if (stringComplete) {
    int val = inputString.toInt() + 1;
#ifdef ESP32_Servo_h
    //analogWrite(motor,inputString.toInt());
    if (val > 180)
      motor.writeMicroseconds(val);
    else
      motor.write(val);
    printInfo();
#else
    ledcWrite(pwmChanel, val);
    Serial.println("[Pulse : " + String(val) + "]");
#endif
    inputString = "";
    stringComplete = false;
  }
  dnsServer.processNextRequest();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
    }
    inputString += inChar;
  }
}
