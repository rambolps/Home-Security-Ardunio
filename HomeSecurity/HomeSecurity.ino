//Alexander Li & Jefferson Liu & Ryan Rambali
//Mr. Wong
//TEJ4M1-1B
//April 19th, 2021

// include the library code:
#include <LiquidCrystal.h>
#include <IRremote.h>

//Define Pin Names
#define dip_1_pin A3
#define dip_2_pin A2
#define dip_3_pin 11
#define dip_4_pin 12
#define piezo_pin 3
#define door_sensor_pin 10
#define LED_ldr_pin A0
#define LED_distance_sensor_pin A1
#define distance_sensor_pin 13
#define fsr_pin A4
#define ldr_pin A5

//setup IR Constants
int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;
bool IROverride = false;

//Setup LCD Pins
const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Global variable declaration
bool doorOpen = false;
bool oldDoorOpen = false; //Previous state of doorOpen. Used to check for change of state.
bool blinkDistSensorLED = true;
int passcode[2] = {LOW, HIGH};    //Correct passcode
int dip_mode[2] = {LOW, LOW};     //Current state of DIP switches 1 and 2
int old_dip_mode[2] = {LOW, LOW}; //Previous state of DIP switches 1 and 2. Used to check for change of state

//States of DIP switch (use constant names)
const int OFF = 0;
const int AT_HOME = 1;
const int AWAY = 2;

int system_mode = 0; //0 is off, 1 is at home, 2 is away. When comparing with system_mode, use constant names defined above.

bool tasks[6];        //Tracks whether a task should be running or not. Currently, tasks 1-5 are used, and 0 is unused.
long finishTimes[6];  //Tracks at what time a task should end in milliseconds after program start. Corresponding array to tasks.
int currentTask = -1; //Tracks which piezo task is currently running. -1 if no tasks are running.

//This is the setup method it runs once, at the start of the program
void setup()
{

  //This tells the Arduino to get ready to exchange messages with the Serial Monitor at a data rate of 9600 bits per second.
  Serial.begin(9600);

  //Set Pin Modes. Analog pins do not need to be defined. IR and LCD pins are handled when constructing the library objects.
  pinMode(dip_3_pin, INPUT);
  pinMode(dip_4_pin, INPUT);
  pinMode(piezo_pin, OUTPUT);
  pinMode(door_sensor_pin, INPUT);
  pinMode(distance_sensor_pin, INPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  //Enable IR sensor
  irrecv.enableIRIn();

  //Start LED task for distance sensor
  newTask(0, 0);
}

//This function updates the LCD, and displays the current method, when no other tasks are running
void updateDisplayMode()
{

  //This ensures that the function only runs, when no other tasks are running
  if (currentTask == -1)
  {
    //Prints "MODE:" with blank spaces for the entire span of the first line
    lcd.setCursor(0, 0);
    lcd.print("Mode:           ");

    //Prints "OFF" on the first line
    if (system_mode == OFF)
    {
      lcd.setCursor(6, 0);
      lcd.print("OFF");
    }
    //Prints "AT HOME" on the first line
    else if (system_mode == AT_HOME)
    {
      lcd.setCursor(6, 0);
      lcd.print("At Home");
    }
    //Prints "AWAY" on the first line
    else //away
    {
      lcd.setCursor(6, 0);
      lcd.print("Away");
    }
  }
}

//Updates the LCD, and displays information and the time remaining, for the task that is currently running
void updateDisplayAlarm()
{
  //Displays when you enter the wrong passcode
  if (tasks[4])
  {
    lcd.setCursor(0, 0);
    lcd.print("WRONG PASSCODE");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print((finishTimes[4] - millis()) / 1000);
  }
  //Displays when the window opens in away mode
  else if (tasks[5])
  {
    lcd.setCursor(0, 0);
    lcd.print("WINDOW OPEN     ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print((finishTimes[5] - millis()) / 1000);
  }
  //displays when the door opens in the AT HOME or AWAY mode
  else if (tasks[1])
  {
    lcd.setCursor(0, 0);
    lcd.print("ENTER PASSCODE  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print((finishTimes[1] - millis()) / 1000);
  }
  //Displays when the ultrasonic distance sensor is triggered in the AWAY or AT HOME modes
  else if (tasks[3])
  {
    lcd.setCursor(0, 0);
    lcd.print("MOTION DETECTED ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print((finishTimes[3] - millis()) / 1000);
  }
}

//Logic for updating system mode from alarm control panel and IR remote
void updateSystemMode()
{
  //Change the system mode using the IR sensor
  changeIRMode();

  //Only allows chnages using the dip switch if the IR Override is off
  if (!IROverride)
  {
    //Gets input from the dip switch
    readDipMode();

    //Checks if the previous mode the dipswitch was in, is the same as the mode it is currently in
    if (old_dip_mode[0] != dip_mode[0] || old_dip_mode[1] != dip_mode[1])
    {
      //Changes the system mode using the dip switch
      changeDipMode();

      //Sets the old dip switch mode to the new dip switch mode
      old_dip_mode[0] = dip_mode[0];
      old_dip_mode[1] = dip_mode[1];
    }
  }
}

//Gets input from the dip switch
void readDipMode()
{
  if (analogRead(dip_1_pin) > 100)
  {
    dip_mode[0] = HIGH;
  }
  else
  {
    dip_mode[0] = LOW;
  }
  if (analogRead(dip_2_pin) > 100)
  {
    dip_mode[1] = HIGH;
  }
  else
  {
    dip_mode[1] = LOW;
  }
}

//Sets the system mode using the input from the dip switch
void changeDipMode()
{
  if (dip_mode[0] == LOW && dip_mode[1] == LOW)
  {
    system_mode = OFF;
  }
  else if (dip_mode[0] == LOW && dip_mode[1] == HIGH)
  {
    system_mode = AT_HOME;
  }
  else if (dip_mode[0] == HIGH && dip_mode[1] == LOW)
  {
    system_mode = AWAY;
  }
}

//Sets the System mode and controls the IR Override, using input from the IR Sensor
//The IR sensor is controled by the IR Remote
void changeIRMode()
{

  //Runs when the IR sensor gets a signal from the remote and interrupts the program. It converts the signal to hexidecimal
  if (irrecv.decode(&results))
  {
    //continues lisenting for IR signals
    irrecv.resume();

    //Sets the system mode depending on which button on the IR remote is pushed
    if (results.value == 0xFD30CF) //Button 0
    {
      system_mode = OFF;
    }
    else if (results.value == 0xFD08F7) //Button 1
    {
      system_mode = AT_HOME;
    }
    else if (results.value == 0xFD8877) //Button 2
    {
      system_mode = AWAY;
    }
    //This runs when the power button on the IR remote is pushed, it controls the IR Override
    else if (results.value == 0xFD00FF) //Power Button
    {
      //Toggles the IR Override
      IROverride = !IROverride;

      //Runs when there is no task running and IR Override is enabled
      if (IROverride && currentTask == -1)
      {
        //Displays "OVERRIDE ACTIVE" on the second row of the LCD
        lcd.setCursor(0, 1);
        lcd.print("OVERRIDE ACTIVE");
      }
      //Runs when IR Override is disabled and no task is running
      else if (currentTask == -1)
      {
        //Prints blank spaces to the second row of the LCD
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
    }
  }
}

//Window sensor, additional feature. If window is closed, the force sensor detects force.
//If the window is open, the force sensor detects no force and if the system mode is set to away, the alarm is sounded.
void forceSensor()
{
  if (system_mode == AWAY && analogRead(fsr_pin) < 100 && !tasks[4])
  {
    newTask(5, 6000);
  }
}

//this is the code that detects whether or not it is light or dark and operates the LED accordingly
void lightSensor()
{
  if (analogRead(ldr_pin) < 525)
  {                              //voltage of 525/1024*5V occurs when slider is approximately at half brightness
    analogWrite(LED_ldr_pin, 0); //turns off the LED when it detects light
  }
  else
  {
    analogWrite(LED_ldr_pin, 1023); //turns on the LED when it is dark
  }
}

//Logic for checking the door
void checkDoor()
{
  //Runs if the door pin is HIGH
  if (digitalRead(door_sensor_pin) == 1)
  {
    doorOpen = true;
    if ((system_mode == AT_HOME || system_mode == AWAY) && !tasks[1] && !tasks[4] && oldDoorOpen != doorOpen)
    { //Passcode entry task or alarm task are not already active
      newTask(1, 5000);
    }
    else if (system_mode == OFF && oldDoorOpen != doorOpen)
    {
      newTask(2, 500);
    }
  }
  else
  {
    doorOpen = false;
  }
  oldDoorOpen = doorOpen;
  //Note: only begin task 1 (passcode entry) if tasks[4] is false (alarm is NOT currently ringing)
}

//Logic for the distance sensor
void distanceSensor()
{
  pinMode(distance_sensor_pin, OUTPUT);
  digitalWrite(distance_sensor_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(distance_sensor_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(distance_sensor_pin, LOW);
  //Timeout 1 second, pulse duration equal to return time
  pinMode(distance_sensor_pin, INPUT);
  long duration = pulseIn(distance_sensor_pin, HIGH);
  //duration in milliseconds. Distance in m
  //v_s_air = 343.42m/s at 20C, 0% humidity, and 1 atm
  //d = 343.42m/s*(t/2)*(1s/1000000us) = t*0.00017171 = t/5823.77. This was tested to be almost perfectly accurate on the practice assignment using the distance sensor.
  float distance = duration * 0.0307; //This constant is experimentally determined to make the distance sensor work with timer lag. We're not sure if this is system-dependent.
  if (distance > 100 && distance < 200)
  {
    blinkDistSensorLED = false;
    if (system_mode == AWAY && !tasks[3])
    {
      newTask(3, 5000); //Sound alarm for 5 seconds
    }
  }
  else
  {
    blinkDistSensorLED = true;
  }
}

//Handles running of tasks, which are methods that tell the piezo or other components to do something. Other code can also see what tasks are currently running for decisions.
//Only one piezo task (1-5) can run at a time
//The currentTask check can be used to set up task on initialization
//Would do this using a queue or list, but we're unsure if the required libraries (even C++ std libraries) are accessible in Arduino. This also limits how we can design the task system.
//A new task can be instantiated using the method void newTask(int, long)
/* Task documentation
 1: Passcode entry prompt for the door sensor, systen_mode == AT_HOME or AWAY
 2: Chime for door sensor, system_mode = OFF
 3: Alarm for indoor sensor (ultrasonic distance sensor), object between 1 and 2m, system_mode == AWAY
 4: Alarm for door sensor, correct passcode not entered after 5 seconds
 5: Alarm for window sensor (force sensor), force is 0, system_mode == AWAY
*/
void checkTasks()
{
  indoor_sensor_blink_LED();

  //Piezo tasks: only 1 can be running at a time to ensure proper piezo functioning. First tasks in conditional statement have higher run priority.
  if (tasks[4])
  { //door sensor alarm
    if (currentTask != 4)
    {
      resetPiezoPin();
      currentTask = 4;
      digitalWrite(piezo_pin, HIGH);
    }
    door_sensor_incorrect_pass();
  }
  else if (tasks[5])
  { //window sensor alarm
    if (currentTask != 5)
    {
      resetPiezoPin();
      digitalWrite(piezo_pin, HIGH);
      currentTask = 5;
    }
    window_open();
  }
  else if (tasks[1])
  { //door sensor passcode entry prompt
    if (currentTask != 1)
    {
      resetPiezoPin();
      currentTask = 1;
    }
    door_sensor_on();
  }
  else if (tasks[2])
  { //door sensor chime
    if (currentTask != 2)
    {
      resetPiezoPin();
      currentTask = 2;
      digitalWrite(piezo_pin, HIGH); //Chime at B4 for half a second
    }
    door_sensor_off();
  }
  else if (tasks[3])
  { //ultrasonic distance sensor alarm
    if (currentTask != 3)
    {
      resetPiezoPin();
      currentTask = 3;
      digitalWrite(piezo_pin, HIGH); //Alarm at F#2 for 5 seconds
    }
    indoor_sensor_away();
  }
  else
    currentTask = -1;
}

//Sets a task to run.
//int taskID: the task to set to run. Refer to documentation on void checkTasks()
//int finishTime: how long the task should run for, in milliseconds. The finish time of the task is recorded as the current time + finishTime
void newTask(int taskID, long finishTime)
{
  tasks[taskID] = true;
  finishTimes[taskID] = millis() + finishTime;
}

//Note: this had more stuff when using the tone() method. Kept for possible return of functionality.
void resetPiezoPin()
{
  digitalWrite(piezo_pin, LOW);
}

//Task methods

//Controls state of LED for the ultrasonic distance sensor
void indoor_sensor_blink_LED()
{
  //Blink cycle of 1 second. milliseconds: 0->499 is on, 500->999 is off. If blinkDistSensorLED is false, the LED is always on.
  if (!blinkDistSensorLED || millis() % 1000 < 500)
  {
    analogWrite(LED_distance_sensor_pin, 1023);
  }
  else
  {
    analogWrite(LED_distance_sensor_pin, 0);
  }
}

//Task 1: door sensor passcode entry prompt
void door_sensor_on()
{
  long task1_time = millis();

  if (task1_time > finishTimes[1])
  {                   //If correct passcode has not been entered after 6 seconds
    newTask(4, 6000); //Activates the alarm for 6 seconds and disables the passcode entry task
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[1] = false;
  }

  //Controls beeping of piezo during passcode entry, beep cycle of 1 second.
  if (task1_time % 1000 < 500)
  {
    digitalWrite(piezo_pin, HIGH);
  }
  else
  {
    digitalWrite(piezo_pin, LOW);
  }

  //Checks if correct passcode is entered.
  if (digitalRead(dip_3_pin) == passcode[0] && digitalRead(dip_4_pin) == passcode[1])
  {
    digitalWrite(piezo_pin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[1] = false; //The passcode entry task will no longer run
    currentTask = -1;
  }
}

//Task 2: door sensor chime
//Here, just ends the task when the finishTime is passed
void door_sensor_off()
{
  if (millis() > finishTimes[2])
  {
    digitalWrite(piezo_pin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[2] = false;
    currentTask = -1;
  }
}

//Task 3: indoor sensor alarm
//Here, just ends the task when the finishTime is passed
void indoor_sensor_away()
{
  if (millis() > finishTimes[3])
  {
    digitalWrite(piezo_pin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[3] = false;
    currentTask = -1;
  }
}

//Task 4: door sensor alarm
//Here, just ends the task when finish time is passed
void door_sensor_incorrect_pass()
{
  if (millis() > finishTimes[4])
  {
    digitalWrite(piezo_pin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[4] = false;
    currentTask = -1;
  }
}

//Task 5: window sensor alarm
//Here, just ends the task when finish time is passed
void window_open()
{
  if (millis() > finishTimes[5])
  {
    digitalWrite(piezo_pin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                ");
    tasks[5] = false;
    currentTask = -1;
  }
}

//END task methods

//main loop, runs until the program finishes executing
void loop()
{
  updateSystemMode();
  forceSensor();
  lightSensor();
  checkDoor();
  distanceSensor();
  checkTasks();
  updateDisplayAlarm();
  updateDisplayMode();
  delay(100);
}
