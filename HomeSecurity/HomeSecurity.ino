//Alexander Li & Jefferson Liu & Ryan Rambali
//Mr. Wong
//TEJ4M1-1B
//April 19th, 2021

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
//States of DIP switch (use constant names)
int AT_HOME = 0;
int AWAY = 1;

int system_mode = 0;

boolean[] tasks = new boolean[5];
int currentTask = 0;


void setup() {

  //Set Pin Modes
  pinMode(ldr_pin,INPUT);
  pinMode(LED_ldr_pin,OUTPUT);
  pinMode(door_sensor_pin,INPUT);
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
  } else{
    doorOpen = false;
  }
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
      tone(piezo_pin, 1000, 5000);  //Sound alarm for 5 seconds
    }
    //Pause/end task: blink LED. Or reset a variable that activates that task.
    //Start task: set LED high for 5 seconds. Keep track of time in this method, with finish time as a global variable.
  }
  else{
    //Start task: blink LED. Or set a variable that activates that task.
  }
  
}

void checkTasks(){

}

void loop() {
  delay(100);
  lightSensor();
}
