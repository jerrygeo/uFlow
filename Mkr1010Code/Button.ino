/*
  Button Task
*   Monitor 3 buttons: NEXT/DOWN, TARE/UP, and BLUE button to 
*     start data download to PC. (4th button is a spare)
*   All buttons are on one analog port: when pressed, buttons pull
*     analog value down to different levels:
*   Button press is acknowledged when button is released.
*                 Vcc
*                 |
*                 _
*                | |
*                |_|  10K
*                 |
*                 |
*           ______________
*           |     |      |
*           _     _      _
*          | |   | |    | |
*          |_|3K |_|8.5K|_|40K
*           |______|______}
*           |      |     |
*          \      \     \ 
*           \      \     \ BUTTONS
*           |______|_____|
*                 |
*                GND
*   The code requires analog value to be stable for 3 samples (60msec)
*     to register a change.
* 
* 
* 
* 
* 
* 

 
 */

#define BUTTON_TASK_REP_RATE 20 /* 20 msec */
#define R1 (10.)
#define R2 (15.) /* Blue button */  
#define R3 (40.)  /* Green (spare) button */
#define R4  (6.65) /* NEXT/DOWN button */
#define R5  (2.1) /* TARE/UP button */
  


#define WIFI_BUTTON 1
#define NEXTorDOWN_KEY 3
#define TAREorUP_KEY  2
#define GREEN_BUTTON 4

String buttonNames[] {
  "SHORTED",
  "BLUE-WIFI",
  "TARE/UP",
  "UNIT/NEXT/DOWN",
  "GREEN-SPARE",
  "NO_BUTTONS_PRESSED"
};


#define NO_BUTTONS_PRESSED 5

enum ButtonState {
  WAITING,
  BUTTON_STABLE,
  NEW_BUTTON,
} buttonState;

struct ButtonStruct {
  int16_t whichKey;     // Negative if button has been read; positive if it has changed but not been read yet
  uint16_t persistence;  // Length of time button has been pressed in units of BUTTON_TASK_REP_RATE
} button;

int16_t lastKey; // Key number 0-5 (5=open, 0 shouldn't happen) that has already been recognized as debounced
int16_t newKey;  // Key number of current ADC value
int16_t sensorValue;


#define MAX_BUTTONS 5 /* Number of keys (buttons) */
void GenerateKeyTable(float vcc,int* array)
{
/*!   Function  GenerateKeyTable
 *      Inputs  vcc - ADC value for analog input measuring resistor divider network with all switches open
 *              array - Array of nominal voltages for each switch
 *      Outputs "array" is updated
 *      Processing - Generate table of nominal voltages for each switch
 */
  float resistor;
 
//////////////1key//////////////////////  
  *array++ = 0;  // First entry is a dummy
  resistor = ((float)R2)/(R1 + R2);
  *array++ = resistor*vcc;

   resistor = ((float)R4)/(R1 + R4);
  *array++ = resistor*vcc;
  
  resistor = ((float)R5)/(R1 + R5);
  *array++ = resistor*vcc;
  
   resistor = ((float)R3)/(R1 + R3);
  *array++ = resistor*vcc;

}
 
unsigned char whichKey(int value)
{
/*!   
 * Function  whichKey
 *      Inputs  value - ADC value measured at button's analog port
 *      Outputs Returns the key number from the keyTable corresponding to ADC measurement
 *      Processing:  Return key number (0-3) if key value is within +/-5 ADC counts of keyTable entry
 *                   If no key matches, return key number 4.
 */
  char tmpChar;
  for (tmpChar=0; tmpChar<5; tmpChar++)
  {
    if (abs(value - KeyTable[tmpChar]) < 5) break;
  }
  return tmpChar;
 
}

int getButtonValue() {  // Returns ADC value - not key number
                        // Used in debugging
/*!   Function  getButtonValue
 *      Inputs  global value sensorValue, the ADC value last read from the button analog port
 *      Outputs Returns the value of sensorValue
 *      Processing:  None
*/
  return sensorValue;
}

struct ButtonStruct getButton() { 
/*!   Function  getButton
 *      Inputs  global struct button
 *      Outputs Returns the value of key that is currently pressed
 *               Value is negative if the key has already been read
 *               Sets value of key negative before returning.
 *      Processing:  see Outputs
*/
  ButtonStruct tempButton = button;
  button.whichKey = abs(button.whichKey); // Key value goes negative to indicate that it has been read
  return tempButton;
}

void buttonTaskInit(){
/*!   Function  buttonTaskInit
 *      Inputs  None
 *      Outputs Set lastKey & newKey to 0
 *              Initialize button struct to persistance=0, buttonState=WAITINGm whichKey=-5
 *      Processing:  see Outputs
*/
  lastKey = -5;
  newKey = -5;
  button.persistence = 0;
  button.whichKey = -5;
  buttonState = WAITING;
}

uint16_t getButtonPressTime ( void ) {
/*!   Function  getButtonPressTime
 *       Getter fuction for button.persistence
 *       This is needed since a long press on TARE key is used to exit
 *         state that checks fluid volume
*/
  return button.persistence;
}

void buttonTask() {
/*!   Function  buttonTask
 *      Inputs  Analog port A1, button struct, globals newKey and lastKey
 *      Outputs button struct, globals newKey and lastKey modified
 *      Processing:  Read the button analog port,  and update the buttonState
 *                    per the state diagram of ButtonStates.odg
 *      
*/
  static boolean blinkOn;
  static int prevKeys[3];

  prevKeys[2] = prevKeys[1];
  prevKeys[1] = prevKeys[0];
  prevKeys[0] = analogRead(A1);
  sensorValue = prevKeys[0];
  sensorValue = sensorValue >> 2;
  newKey = whichKey(sensorValue);
  if (newKey == lastKey) button.persistence++;
  else button.persistence = 0;
  switch( buttonState ) {
    case WAITING:
         if (button.persistence >= 2) buttonState = NEW_BUTTON;
          button.whichKey = newKey;
          break;

    case NEW_BUTTON:
          if (newKey != lastKey) buttonState = WAITING;
          break;
    }
    lastKey = newKey; 
    tasks.after(BUTTON_TASK_REP_RATE,buttonTask);  // Restart after a delay
}

 
