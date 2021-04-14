//Alexander Li & Jefferson Liu & Ryan Rambali
//Mr. Wong
//TEJ4M1-1B
//April 19th, 2021

// include the library code:
#include <LiquidCrystal.h>
#include <IRremote.h>
#include <IRremoteInt.h>

//Define Pin Names
#define ir_sensor_pin 0
#define dip_1_pin 1
#define dip_2_pin 2
#define dip_3_pin 11
#define dip_4_pin 12
#define piezo_pin 3
#define door_sensor_pin 10
#define LED_ldr_pin A0
#define LED_distance_sensor_pin A1
#define distance_sensor_pin 13
#define fsr_pin A4
#define ldr_pin A5

//Global variable declaration
bool doorOpen = false;
bool blinkDistSensorLED = true;
int passcode[2] = {LOW, HIGH};
//States of DIP switch (use constant names)
const int OFF = 0;
const int AT_HOME = 1;
const int AWAY = 2;

int system_mode = 0;  //0 is off, 1 is at home, 2 is away

bool tasks[6];
long finishTimes[6];
int currentTask = 0;


void setup() {

  //Set Pin Modes
  pinMode(ir_sensor_pin, INPUT);
  pinMode(dip_1_pin, INPUT);
  pinMode(dip_2_pin, INPUT);
  pinMode(dip_3_pin, INPUT);
  pinMode(dip_4_pin, INPUT);
  pinMode(piezo_pin, OUTPUT);
  pinMode(door_sensor_pin,INPUT);
  pinMode(LED_ldr_pin,OUTPUT);
  pinMode(LED_distance_sensor_pin, OUTPUT);
  pinMode(distance_sensor_pin, INPUT);
  pinMode(fsr_pin, INPUT);
  pinMode(ldr_pin,INPUT);
  
  //Handle LCD declaration here
}

//Logic for updating system mode from alarm control panel and IR remote
void updateSystemMode(){
  
}

void forceSensor(){

}


//this is the code that detects whether or not it is light or dark and operates the LED accordingly
void lightSensor (){
  if(analogRead(LDR)>500){
    analogWrite(LED_ldr_pin,1023);//turns off the LED when it detects light
  }
  else{
    analogWrite(LED_ldr_pin,0);//turns on the LED when it is dark
  }
}


void checkDoor(){
  if(digitalRead(door_sensor_pin) == 1){
    doorOpen = true;
    if ((system_mode == AT_HOME || system_mode == AWAY) && !tasks[1] && !tasks[4]){ //Passcode entry task or alarm task are not already active
      newTask(1, micros() + 5000);
    }
    else if (system_mode == OFF){
      newTask(2, micros() + 500);
    }
  } else{
    doorOpen = false;
  }
  //Note: only begin task 1 (passcode entry) if tasks[4] is false (alarm is NOT currently ringing)
}

void distanceSensor(){
  pinMode(distance_sensor_pin, OUTPUT);
  digitalWrite(distance_sensor_pin, LOW);
  delay(2);
  digitalWrite(distance_sensor_pin, HIGH);
  delay(10);
  digitalWrite(distance_sensor_pin, LOW);
  //Timeout 1 second, pulse duration equal to return time
  pinMode(distance_sensor_pin, INPUT);
  long duration = pulseIn(distance_sensor_pin, HIGH);
  //duration in microseconds. Distance in m
  //v_s_air = 343.42m/s at 20C, 0% humidity, and 1 atm
  //d = 343.42m/s*(t/2)*(1s/1000000us)
  float distance = duration/5823.77;
  if (distance > 1 && distance < 2){
    if (system_mode == AWAY){
      newTask(4, micros() + 5000);  //Sound alarm for 5 seconds
      blinkDistSensorLED = false;
    }
  }
  else{
    blinkDistSensorLED = true;
  }
  
}

//Checks if tasks should be run
//Only one piezo task (1-3) can run at a time
//The currentTask check can be used to set up task on initialization
//Would do this using a queue or list, but we're unsure if the required libraries (even C++ std libraries) are accessible in Arduino
//Create a new task using the newTask() method
void checkTasks(){
  indoor_sensor_blink_LED();

  //Piezo tasks: only 1 can be running at a time to ensure proper piezo functioning
  if (tasks[4]){
    if (currentTask != 4){
      resetPiezoPin();
      currentTask = 4;
      tone(piezo_pin, 160, 6000);
    }
  }
  else if (tasks[1]){
    if (currentTask != 1){
      resetPiezoPin();
      currentTask = 1;
    } 
  }
  else if (tasks[2]){
    if (currentTask != 2){
      resetPiezoPin();
      currentTask = 2;
      tone(piezo_pin, 494, 500);  //Chime at B4 for half a second
    }
  }
  else if (tasks[3]){
    if (currentTask != 3){
      resetPiezoPin();
      currentTask = 3;
      tone(piezo_pin, 185, 5000);  //Alarm at F#2 for 5 seconds
    }
  }
  else currentTask = -1;

}

void newTask(int taskID, long finishTime){
  tasks[taskID] = true;
  finishTimes[taskID] = finishTime;
}

void resetPiezoPin(){
  noTone(piezo_pin);
  digitalWrite(piezo_pin, LOW);
}

//Task methods

//Setting finish time to 0 will blink LED, else if the finish time is greater the LED will be on
void indoor_sensor_blink_LED(){
  //Blink cycle of 1 second. milliseconds: 0->499 is on, 500->999 is off
  long task0_time = micros();
  if (!blinkDistSensorLED || task0_time%1000 < 500){
    digitalWrite(LED_distance_sensor_pin, HIGH);
  }
  else{
    digitalWrite(LED_distance_sensor_pin, LOW);
  }
}

void door_sensor_on(){  //Task 1. Feel free to use whatever global variables are needed to make this work
  //INSERT CHECK CORRECT PASSCODE CODE HERE. SET tasks[1] TO FALSE WHEN DONE.
  //Update passcode
  if (digitalRead(dip_3_pin) == passcode[0] && digitalRead(dip_4_pin) == passcode[1]){
    tasks[1] = false;
    //Display appropriate LCD message
  }

  long task1_time = micros();
  if (micros() > finishTimes[1]){
    newTask(4, task1_time + 6000) //Activates the alarm for 6 seconds and disables the passcode entry task
    tasks[1] = false;
    //Display appropriate LCD message
  }
}

void door_sensor_off(){ //Task 2
  if (micros() > finishTimes[2]){
    noTone(3);
    tasks[2] = false;
  }
}

void indoor_sensor_away(){  //Task 3
  if (micros() > finishTimes[3]){
    noTone(3);
    tasks[3] = false;
  }
}

void door_sensor_incorrect_pass(){
  if (micros() > finishTimes[3]){
    noTone(3);
    tasks[4] = false;
  }
}

//END task methods

void loop() {
  updateACP();
  forceSensor();
  lightSensor();
  checkDoor();
  distanceSensor();
  checkTasks();
}
