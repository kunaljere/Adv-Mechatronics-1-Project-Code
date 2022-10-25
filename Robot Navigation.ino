#include "SimpleRSLK.h"

uint16_t sensorVal[LS_NUM_SENSORS];
uint16_t sensorCalVal[LS_NUM_SENSORS];
uint16_t sensorMaxVal[LS_NUM_SENSORS];
uint16_t sensorMinVal[LS_NUM_SENSORS];

enum Navigation_States {UPPER_LEFT, UPPER_MIDDLE, UPPER_RIGHT, LOWER_LEFT, LOWER_MIDDLE, LOWER_RIGHT};
enum Shooting_States {BALL_IN_PLACE, BALL_SHOT};
enum IR_Beacon {LEFT_SENSOR, MIDDLE_SENSOR, RIGHT_SENSOR}; //0, 1, 2
enum Directions {LEFT, MIDDLE, RIGHT};

enum Navigation_States navState, prevState;
enum Shooting_States shoot;
enum Directions dir;
enum IR_Beacon beacon;

// SPEED VARIABLES
//higher numbers are to accomodate weight of the structure
int slowSpeed = 15; //10;
int fastSpeed = 25; //20;
int turnSpeed = 25; //15;
uint32_t bottomSensorVal;

// CALIBRATION VARIABLES
bool isCalibrationComplete = false;

// CALIBRATION FUNCTIONS
void floorCalibration() {
  /* Place Robot On Floor (no line) */
  delay(2000);
  String btnMsg = "Push left button on Launchpad to begin calibration.\n";
  btnMsg += "Make sure the robot is on the floor away from the line.\n";
  /* Wait until button is pressed to start robot */
  waitBtnPressed(LP_LEFT_BTN,btnMsg,RED_LED);

  delay(1000);

  Serial.println("Running calibration on floor");
  simpleCalibrate();
  Serial.println("Reading floor values complete");

  btnMsg = "Push left button on Launchpad to begin line following.\n";
  btnMsg += "Make sure the robot is on the line.\n";
  /* Wait until button is pressed to start robot */
  waitBtnPressed(LP_LEFT_BTN,btnMsg,RED_LED);
  delay(1000);

  enableMotor(BOTH_MOTORS);
}

void simpleCalibrate() {
  /* Set both motors direction forward */
  setMotorDirection(BOTH_MOTORS,MOTOR_DIR_FORWARD);
  /* Enable both motors */
  enableMotor(BOTH_MOTORS);
  /* Set both motors speed 20 */
  setMotorSpeed(BOTH_MOTORS,20);

  for(int x = 0;x<100;x++){
    readLineSensor(sensorVal);
    setSensorMinMax(sensorVal,sensorMinVal,sensorMaxVal);
  }

  /* Disable both motors */
  disableMotor(BOTH_MOTORS);
}

//BOOLEAN VARIABLES
bool stopProgram = false;
bool navHasBegun = false;
bool lineFollowDone = false;
bool aimingDone = false;
bool atBeacon = false;
bool forward;
bool stopMotors;
bool turnRight;
bool turnLeft;
bool shotLeft = false;
bool shotRight = false;
bool shotMiddle = false;

//MOTOR FUNCTIONS
void motors_stop()
{
  //code for stopping motors
  setMotorSpeed(BOTH_MOTORS, 0); 
}
 
void motors_forward(int speed = fastSpeed)
{
  //code for driving forward 
  enableMotor(BOTH_MOTORS);
  setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD);
  setMotorSpeed(BOTH_MOTORS, speed); 
}

void motors_backward()
{
  enableMotor(BOTH_MOTORS);
  setMotorDirection(BOTH_MOTORS, MOTOR_DIR_BACKWARD);
  setMotorSpeed(BOTH_MOTORS, fastSpeed); 
}
 
void turn_right_slight()
{
  //code for turning right
  enableMotor(BOTH_MOTORS);
  setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD);
  setMotorSpeed(LEFT_MOTOR,fastSpeed);
  setMotorSpeed(RIGHT_MOTOR,slowSpeed);
}
 
void turn_left_slight()
{
  //code for turning left
  enableMotor(BOTH_MOTORS);
  setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD);
  setMotorSpeed(LEFT_MOTOR,slowSpeed);
  setMotorSpeed(RIGHT_MOTOR,fastSpeed);
}

void turn_right_full()
{
  enableMotor(BOTH_MOTORS);
  setMotorDirection(RIGHT_MOTOR, MOTOR_DIR_BACKWARD);
  setMotorDirection(LEFT_MOTOR, MOTOR_DIR_FORWARD);
  setMotorSpeed(BOTH_MOTORS,turnSpeed);
}

void turn_left_full()
{
  enableMotor(BOTH_MOTORS);
  setMotorDirection(RIGHT_MOTOR, MOTOR_DIR_FORWARD);
  setMotorDirection(LEFT_MOTOR, MOTOR_DIR_BACKWARD);
  setMotorSpeed(BOTH_MOTORS,turnSpeed);
}

//AIMING FUNCTIONS, turn right time was 990
void AimRight(int forwardTime = 560, int turnRightTime = 770, int waitTime = 1000)
{
  if (stopMotors == true)
  {
    motors_stop();
    if (wait(waitTime) == true)
    {
      stopMotors = false;
      turnRight = true;
    }
  }
  else if (forward == true)
  {
    motors_forward(slowSpeed);
    if (wait(forwardTime) == true)
    {
      forward = false;
      stopMotors = true;
      turnRight = false;
    }
  }
  else if (turnRight == true)
  {
    turn_right_full();
    if (wait(turnRightTime) == true)
    {
      turnRight = false;
      forward = true;
      stopMotors = false;
      aimingDone = true;
      return;
    }
  }
}
//turn time was 990
void AimLeft(int forwardTime = 560, int turnLeftTime = 770, int waitTime = 1000)
{
  if (stopMotors == true)
  {
    motors_stop();
    if (wait(waitTime) == true)
    {
      stopMotors = false;
      turnLeft = true;
    }
  }
  else if (forward == true)
  {
    motors_forward(slowSpeed);
    if (wait(forwardTime) == true)
    {
      //put condition for at the beacon here
      forward = false;
      stopMotors = true;
      turnLeft = false;
    }
  }
  else if (turnLeft == true)
  {
    turn_left_full();
    if (wait(turnLeftTime) == true)
    {
      turnLeft = false;
      forward = true;
      stopMotors = false;
      aimingDone = true;
      return;
//        stopMotors = true;
    }
  }
}

void AimMiddle(int forwardTime = 560)
{
  if (forward == true)
  {
    motors_forward(slowSpeed);
    if (wait(forwardTime) == true)
    {
      forward = true;
      aimingDone = true;
      return;
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  setupRSLK();
  /* Left button on Launchpad */
  setupWaitBtn(LP_LEFT_BTN);
  /* Red led in rgb led */
  setupLed(RED_LED);
  clearMinMax(sensorMinVal,sensorMaxVal);
}

int i = 0;
bool nextBeacon;

void loop() {
  /* Run this setup only once */
  uint8_t lineColor = DARK_LINE;

  if(isCalibrationComplete == false) {
    floorCalibration();
    isCalibrationComplete = true;
  }

  readLineSensor(sensorVal);
  readCalLineSensor(sensorVal,
          sensorCalVal,
          sensorMinVal,
          sensorMaxVal,
          lineColor);

  uint32_t linePos = getLinePosition(sensorCalVal,lineColor);
  int allLines = sensorCalVal[0] + sensorCalVal[1] + sensorCalVal[2] + sensorCalVal[3] + sensorCalVal[4] + sensorCalVal[5] + sensorCalVal[6] + sensorCalVal[7];

  
  if (stopProgram == false)
  {
    if (navHasBegun == false)
    {
      beginNavigation(linePos, allLines);
    }
    else if (navHasBegun == true) 
    {
      if (i == 0)
      {
        beacon = LEFT_SENSOR;
        if (nextBeacon == false)
          navigation(linePos, allLines);
        else if (nextBeacon == true)
        {
          i++;
          shotLeft = true;
          nextBeacon = false;
        }
      }
      else if (i == 1)
      {
        beacon = MIDDLE_SENSOR;
        if (nextBeacon == false)
          navigation(linePos, allLines);
        else if (nextBeacon == true)
        {
          i++;
          shotMiddle = true;
          nextBeacon = false;
        }
      }
      else if (i == 2)
      {
        beacon = RIGHT_SENSOR;
        if (nextBeacon == false) 
          navigation(linePos, allLines);
        else if (nextBeacon == true)
        {
          i++;
          shotRight = true;
          nextBeacon = false;
        }
      }
      else if (i == 3)
      {
        beacon = MIDDLE_SENSOR;
        if (nextBeacon == false) //got here
          navigation(linePos, allLines);
        else if (nextBeacon == true)
        {
          i++;
          shotMiddle = true;
          nextBeacon = false;
        }
      }
      else if (i == 4)
        stopProgram = true;
    }
  }
  else if (stopProgram == true)
    motors_stop();
}

void beginNavigation(uint32_t linePos, int allLines)
{

  followLine(linePos, allLines); //already waits 1 second after reaching cross-section
  
  if (lineFollowDone == true)
  {
    motors_forward(slowSpeed);
    if (wait(500) == true)
    {
      motors_stop();
      navState = UPPER_MIDDLE;
      navHasBegun = true;
      lineFollowDone = false;
      nextBeacon = false;
    }
  }
}

//include logic for when robot is at the beacon - turning to face the beacon
void navigation(uint32_t linePos, int allLines)
{
  switch(navState)
  {
    case UPPER_MIDDLE:
      //might have to calculate sensor value every time you move/turn
      if (beacon == LEFT_SENSOR || beacon == RIGHT_SENSOR || beacon == MIDDLE_SENSOR)
      {
        if (lineFollowDone == false)
        {
          followLine(linePos, allLines); //already waits 1 second after reaching cross-section
        }
        if (lineFollowDone == true)
        {
          //depending on the beacon, turn left or right
          if (aimingDone == false) //&& forward == true
          {
            dir = MIDDLE;
            crossSectionAiming(); // already pointing to correct beacon, to the left
          }
          if (aimingDone == true)
          {
            motors_stop();
            if (wait(1000) == true)
            {
              navState = LOWER_MIDDLE;
              prevState = UPPER_MIDDLE;
//              lineFollowDone = false;
//              aimingDone = false;
            }
          }
        }
      }      
      break;

    case LOWER_MIDDLE:
      if (beacon == LEFT_SENSOR || beacon == RIGHT_SENSOR)
      {
        //don't follow line if shot from lower middle state
        //can immediately start following line from UPPER_MIDDLE case
        if (prevState == UPPER_MIDDLE)
        {
          //LOOK AT THIS CONDITION IF SOMETHING GOES WRONG
          lineFollowDone = false;
        }
        
        if (prevState == LOWER_LEFT || prevState == LOWER_RIGHT)
        {
          //aimingDone is false to allow cross-section aiming
          forward = false;
          stopMotors = true;
          if (beacon == LEFT_SENSOR)
            dir = LEFT;
          else if (beacon == RIGHT_SENSOR)
            dir = RIGHT;
          crossSectionAiming(); //aims towards left or right sensors
          motors_stop();
          if (wait(1000) == true)
          {
            shotMiddle = false;
            lineFollowDone = false;
          }
        }

        //getting here
//        if (beacon == RIGHT_SENSOR)
//          motors_backward();

        if (shotMiddle == false)
        {
          if (lineFollowDone == false)
          {
            //FOLLOWING BOTTOM LINE
            followLine(linePos, allLines); //already waits 1 second after reaching cross-section
            prevState = LOWER_MIDDLE;
          }
          if (lineFollowDone == true)
          {
            if (aimingDone == false)
            {
              if (beacon == LEFT_SENSOR)
                dir = RIGHT;
              if (beacon == RIGHT_SENSOR)
              {
                dir = LEFT;
//                delay(50);
              }
              crossSectionAiming(); //aimed at LEFT or RIGHT beacon at this point
//              prevState = LOWER_MIDDLE;
            }
            if (aimingDone == true)
            {
              motors_stop();
              if (wait(1000) == true)
              {
//                break;
                if (beacon == LEFT_SENSOR)
                  navState = LOWER_LEFT;
                else if (beacon == RIGHT_SENSOR) 
                  navState = LOWER_RIGHT;
              }
            }
          }
        }
      }
      else if (beacon == MIDDLE_SENSOR)
      {
        //here both lineFollowDone and aimingDone are true
        // SHOOT BALL
//        motors_backward(); //got here
        motors_stop();
        if (wait(1000) == true)
        {
          navState = LOWER_MIDDLE;
          aimingDone = false;
          nextBeacon = true; //going to right beacon
          return;
        }
      }
      break;

    case LOWER_LEFT:
      if (beacon == MIDDLE_SENSOR || beacon == RIGHT_SENSOR)
      {

        //turn right first to get back on the line to follow
        if (aimingDone == false && (prevState == LOWER_MIDDLE || prevState == LOWER_RIGHT))
        {
          forward = false;
          stopMotors = true;
          dir = RIGHT;
          crossSectionAiming();
          motors_forward(slowSpeed);
          delay(300);
//          if (wait(300) == true)
//          {
//            aimingDone = true;
//            break;
//          }
        }
        else if (aimingDone == true && (prevState == LOWER_MIDDLE || prevState == LOWER_RIGHT))
        {
          motors_stop(); // stop motors for 1 second
          if (wait(500) == true)   // CHANGE TO 1000
          {
            lineFollowDone = false;
            prevState = LOWER_LEFT; //leaving lower left to follow line
//            motors_backward();
          }
        }
        

        if (lineFollowDone == false) //BACK TO FOLLOWING BOTTOM LINE
        {
          followLine(linePos, allLines); //already waits 1 second after reaching cross-section, makes aimingDone false
        }

        if (lineFollowDone == true && beacon == MIDDLE_SENSOR)
        {
          if (aimingDone == false)
          {
            dir = LEFT;
            crossSectionAiming(); //aims left towards middle sensor
          }
          else if (aimingDone == true)
          {
            motors_stop();
            if (wait(1000) == true)
            {
              navState = LOWER_MIDDLE;
              prevState = LOWER_LEFT;
              //MAY OR MAY NOT NEED lineFollowDone LINE
//              lineFollowDone = false;
            }
          }
        }
        
//        if (lineFollowDone == true && beacon == RIGHT_SENSOR)
//        {
//          motors_forward(slowSpeed);
////          delay(500);
////          lineFollowDone = false;
//          if (wait(700) == true)
//          {
//            lineFollowDone = false;
//          }
//          if (lineFollowDone = false)
//          {
//            followLine(linePos, allLines); //already waits 1 second after reaching cross-section
//          }
//          
//          if (lineFollowDone == true)
//          {
//            if (aimingDone == false)
//            {
//              dir = LEFT;
//              delay(300);
//              crossSectionAiming(); //aims left to right beacon
//            }
//            else if (aimingDone == true)
//            {
//              motors_stop();
//              if (wait(1000) == true)
//              {
//                navState = LOWER_RIGHT;
//                prevState = LOWER_LEFT;
//  //              lineFollowDone = false;  
//              }
//            }
//          }
//        }
      }
      else if (beacon == LEFT_SENSOR)
      {
        // SHOOT BALL
        motors_stop();
        if (wait(1000) == true)
        {
          //it is reaching here
//          motors_backward();
          navState = LOWER_LEFT;
          aimingDone = false;
          lineFollowDone = true;
          nextBeacon = true;
          return;
        }
      }
      break;

    case LOWER_RIGHT:
      if (beacon == MIDDLE_SENSOR || beacon == LEFT_SENSOR)
      {
        if (aimingDone == false && (prevState == LOWER_MIDDLE || prevState == LOWER_LEFT))
        {
          forward = false;
          stopMotors = true;
          dir = LEFT;
          crossSectionAiming();
          motors_forward(slowSpeed);
          delay(300);
//          if (wait(300) == true) //probably don't need
//          {
//            aimingDone = true;
//            break;
//          }
        }
        else if (aimingDone == true && (prevState == LOWER_MIDDLE || prevState == LOWER_LEFT))
        {
          motors_stop(); // stop motors for 1 second
          if (wait(500) == true)   // CHANGE TO 1000
          {
            lineFollowDone = false;
            prevState = LOWER_RIGHT; //leaving lower right to follow line
          }        
        }

        if (lineFollowDone == false)
        {
          followLine(linePos, allLines); //already waits 1 second after reaching cross-section
        }
        if (lineFollowDone == true && beacon == MIDDLE_SENSOR)
        {
          if (aimingDone == false)
          {
            dir = RIGHT;
            crossSectionAiming(); //aims right
          }
          else if (aimingDone == true)
          {
            motors_stop();
            if (wait(1000) == true)
            {
              navState = LOWER_MIDDLE;
              prevState = LOWER_RIGHT;
//              lineFollowDone = false;
            }
          }
        }
//        if (lineFollowDone == true && beacon == LEFT_SENSOR)
//        {
//          while (aimingDone == false)
//            AimMiddle(700); //move forward to get different sensor value
//          //time delay here
//          followLine(linePos, allLines); //already waits 1 second after reaching cross-section
//          
//          if (aimingDone == false)
//          {
//            dir = RIGHT;
//            crossSectionAiming(); //aims right
//          }
//          else if (aimingDone == true)
//          {
//            motors_stop();
//            if (wait(1000) == true)
//            {
//              navState = LOWER_LEFT;
//              prevState = LOWER_RIGHT;
//              lineFollowDone = false;
//              aimingDone = false;
//            }
//          }
//        }
      }
      else if (beacon == RIGHT_SENSOR)
        {
//          motors_backward(); //debug FOR RIGHT SENSOR
          // SHOOT BALL
          motors_stop();
          if (wait(1000) == true)
          {
            navState = LOWER_RIGHT;
            aimingDone = false;
            lineFollowDone = true;
            nextBeacon = true;
            return;
          }
        }
      break;
  }
}

void followLine(uint32_t linePos, int allLines)
{
  //only after calibration
  
  if(linePos > 0 && linePos < 3000) 
    turn_left_slight();
  else if(linePos > 3500)
    turn_right_slight();
  else if(allLines > 7000){//6500
    motors_stop();
    if (wait(1000) == true)
    {
      forward = true;
      lineFollowDone = true;
      aimingDone = false;
      return;
    }
  }
  else {
    motors_forward();
  }
}

bool wait(unsigned long duration)
{
  static unsigned long startTime;
  static bool isStarted = false;

  // if wait period not started yet
  if(isStarted == false)
  {
    // set the start time of the wait period
    startTime = millis();
    // indicate that it's started
    isStarted = true;
    // indicate to caller that wait period is in progress
    return false;
  }

  // check if wait period has lapsed
  if(millis() - startTime >= duration)
  {
    // lapsed, indicate no longer started so next time we call the function it will initialise the start time again
    isStarted = false;
    // indicate to caller that wait period has lapsed
    return true;
  }

  // indicate to caller that wait period is in progress
  return false;
}

//include logic for when robot is at the beacon - turning to face the beacon.
void crossSectionAiming()
{
  // replace numbers with timing variables
  if (dir == MIDDLE)
  {
    if (beacon == RIGHT_SENSOR)
    {
      while (aimingDone == false)
        AimRight();
      atBeacon = false;
      return;
    }
    else if (beacon == LEFT_SENSOR)
    {      
      while (aimingDone == false)
        AimLeft();
      atBeacon = false;
      return;
      
    }
    else if (beacon == MIDDLE_SENSOR)
    {
      while (aimingDone == false)
        AimMiddle();
      forward = true;
      atBeacon = true;
      return;
    }
  }
  else if (dir == LEFT)
  {
    while (aimingDone == false)
      AimLeft();
    return;
  }
  else if (dir == RIGHT)
  {
    while (aimingDone == false)
      AimRight();
    return;
  }
}
