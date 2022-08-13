// Scalemkr1010
//   Program to measure and store scale weight readings, display maximum flow rate, and to allow downloading stored data
//      via wifi using a MKR1010 Arduino processor, MKR Mem Shield, DS3234 clock shield,  and an HX711 shield.
//
//   Copyright 2022, Jerry Smith
//
//   Pin Usage on MKR1010:
//      0 32KHz square wave from DS3234
//      1 /CS for HiLetGo 1.44" LCD display
//      2 CLK for HX711 shield
//      3 DAT for HX711 shield
//      4 /CS for uSD on MKR Mem Shield
//      5 /CS for W25Q16 flash on MKR Mem Shield
//      A1  Analog channel for detecting button presses
//      19 Toggle showing 100msec period of MainStateTask
//      20 (A5) /CS for DS3234 shield
//      21 A0 (DC) for HiLetGo 1.44" LCD display
//   SPI:  MOSI 8, MISO 10, SCK 9
//   I2C:  SDA 11, SCL 12  (not used)
//
//   After taring the scale, wait until 10mL is measured, then acquire scale measurements
//        at 100 msec intervals. When DONE is pressed, compute & display max flow rate.
//
//   The design uses Cooperative Multitasking, with the following tasks:
//           Button - reads and debounces input A1 to determine when a button is pressed or released
//           MainStateTask - controls the various states including normal operation and commands from SerialTask
//           SerialTask - reads/buffers command inputs from USB or wifi. Some commands are executed directly; others
//               set flags that are monitored by MainStateTask. Commands are:
//                    TIME - Set time
//                    RECAL - Recalibrate
//                    DELETE - Erase the stored files
//                    X - Transfer data to PC via wifi
//                    BUTTONS - Tests buttons
//                    H - Print help message

#include <SD.h>
#include <RtcDateTime.h>
#include <RtcDS3234.h>  //Library is RTC_by_Makuna
//Note: To use RTC_by_Maruna needed to comment out #include "component/rtc.h" from line 265 of
     //C:\Users\jerry\AppData\Local\Arduino15\packages\arduino\tools\CMSIS-Atmel\1.2.0\CMSIS\Device\ATMEL\samd21\include\samd21g18a.h


/*
 Scale Flow Measurement Using HX711
 */


#include <HX711.h>
#include <CooperativeMultitasking.h>
#include <WiFiNINA.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

// Reference for connecting SPI see https://www.arduino.cc/en/Reference/SPI
// CONNECTIONS:
// DS3234 MISO --> MISO
// DS3234 MOSI --> MOSI
// DS3234 CLK  --> CLK (SCK)
// DS3234 CS (SS) --> 5 (pin used to select the DS3234 on the SPI)
// DS3234 VCC --> 3.3v or 5v
// DS3234 GND --> GND
#include <SPI.h>


//#define DEBUG

String copr = "Copyright 2022, Jerry Smith";
/*********************************
 *    PIN DEFINITIONS
 *********************************/

// Pin definitions for 1.44" TFT screen
#define TFT_CS        1   //!js2021-06-2021
#define TFT_RST       -1   //!js2021-06-2021    19 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC        21   //!js2021-06-2021    8 (was 5 for LCD test program)

const uint8_t DS3234_CS_PIN = 20;

// Needed for HX711
#define DOUT 3
#define CLK  2

#define CLOCK_CHECK_PIN  0
// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
const uint8_t chipSelect = SS;

/*********************************
 * 
 * GLOBAL VARIABLES AND DEFINES
 * 
 *******************************/

const char cVersion[] = {"v2.00"};

#define nVcc 255.

#define countof(a) (sizeof(a) / sizeof(a[0]))

struct ConfigStruct  {
  float swVersion;
  int16_t strainGaugeScale;
  int16_t checkSum;
  boolean configErr;
  String configErrMsg;
};

struct ConfigStruct GetConfig();
struct ConfigStruct uFlowCfg;


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
RtcDS3234<SPIClass> Rtc(SPI, DS3234_CS_PIN);

CooperativeMultitasking tasks;
Continuation serialTask;
Continuation mainStateTask;
Continuation buttonTask;

RtcDateTime timeAtStart;

boolean recalRequested = false;
void setRecalRequest(boolean trueOrFalse) {
  recalRequested = trueOrFalse;
}
boolean getRecalRequest() {
  return recalRequested;
}


boolean serialConnected = true;

int KeyTable[15];
int timeBaseCheck;

int selfTestError;
#define WIFI_MODULE_ERROR 1
#define WIFI_FIRMWARE_ERROR 2
#define SD_CARD_ERROR 4
#define TIME_BASE_ERROR 8
#define CONFIG_OPEN_ERROR 0x10


void GenerateKeyTable(float vcc,int* KeyTable);



HX711 scale((byte) DOUT, (byte) CLK, (byte) 128);
long scaleOffset;
float scaleCalib = 1.0;



void tftSplashScreen() {
/*!   Function  tftSplashScreen
 *      Inputs  None
 *      Outputs TFT screen updated
 *      Processing - Initialize screen for landscape mode, black background, yellow text
 *                   Display  UFLOW, version string with date, and "PRESS TARE"
 *                   
 */
  char datestring[12];

  tft.setRotation(1); //Landscape mode
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  tft.print("\n\n    UFlow\n");
  tft.print("    ");
  tft.print(cVersion);
  tft.print("\n\n    ");
  snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u\n"),
            timeAtStart.Month(),
            timeAtStart.Day(),
            timeAtStart.Year() );
    tft.print(datestring);
   tft.setTextSize(2);
   tft.print("\n Press TARE\n");
  
}

void setScaleFactor(int16_t newFactor) {
  uFlowCfg.strainGaugeScale = newFactor;
}


void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}


void RtcSetup() {
/*!   Function  RtcSetup
 *      Inputs  None
 *      Outputs RTC chip (DS3234) may get initialized
 *      Processing - If RTC date is not valid or earlier than compile date, initialize it with the compile date
 *                   Set RTC square wave frequency to 8.192k kHz but turn off the square wave output
 *                   
 */
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();
    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    timeAtStart = Rtc.GetDateTime();
    if  ( (timeAtStart < compiled) || (timeAtStart.Month() > 12) )
    {
        Serial.println("RTC is older than compile time or invalid!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (timeAtStart > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (timeAtStart == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePinClockFrequency(DS3234SquareWaveClock_8kHz);
    Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeClock);   
}

void ClockCheck( void ){  // Interrupt routine for 8kHz clock input on pin 0
  timeBaseCheck++;
}

void setup() {
/*!   Function  setup
 *      Inputs  None
 *      Outputs 
 *      Processing - Initialize serial port to 115200 baud
 *                   Wait up to 1 second to detect that the serial port (USB) is connected to a PC
 *                     If not detected, set sericalConnected=false to inicate that there is no serial port
 *                   Check for the Wifi module - set error flag if not found.
 *                   Check that the wifi library is compatible with the current compiler library.
 *                     If not, set error flag
 *                   Initialize SPI pins and RTC  
 *                   Initialize the LCD
 *                   Check that the uSD card is present, set error flag if not
 *                   Set pin 20 for output mode to be used as /CS for RTC
 *                   Initialize DS3234 If the RTC date is not valid or earlier than the compile date, 
 *                      set the RTC date to the compile date. If the RTC is not running, start it. 
 *                      Set the square wave output of the RTC to 8.192 kHz and check that there are 
 *                      4096 +/ 100 rising edges in 500msec - set error flag if not Ok.
 *                   Read configuration file UFLOW.CFG for scale calibration
 */

    unsigned long serialTimeOut;

    selfTestError = 0;
    pinMode(19, OUTPUT);   // Set up pin that shows timing of MainStateTask
    Serial.begin(115200);                 // Baud rate for Serial output (debug), but you can define any baud rate you want
    serialTimeOut = millis() + 1000;
    while (!Serial) {
      
        if (millis() > serialTimeOut)  // wait up to 1 second for serial port to connect. Needed for native USB port only
        { serialConnected = false;
          break;
        }
    }
                           // Check WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue 
    selfTestError += WIFI_MODULE_ERROR;
   }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware\n Version: ");
    Serial.println(fv);
    selfTestError += WIFI_FIRMWARE_ERROR;
  }
    
    SPI.begin();
    tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab

    Rtc.Begin();
    if ( !SD.begin(SS) ) {
      Serial.println("SD Card failed or not present");
      selfTestError += SD_CARD_ERROR;
      }
      else Serial.println("SD Card OK");
    RtcSetup();
    timeBaseCheck = 0;
    pinMode(CLOCK_CHECK_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(CLOCK_CHECK_PIN), ClockCheck, RISING); 
    Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeClock); // Set for 8.192 kHz above
//    Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeNone); //!For Test of clock error, turn off clock output
    delay(500);
    Serial.print("Clock check: ");
    Serial.print(timeBaseCheck);
    Serial.print("\n");
    if ( abs(timeBaseCheck - 4095) > 100 ) { // Check that time base is accurate to within better than .25%
      Serial.println("Time base error ");
      Serial.println(fv);
      selfTestError += TIME_BASE_ERROR;
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0,0);
      tft.print("\n\n Time Base Problem.\n Check 100msec rate");
      while (true);  //Get stuck here
      }
      Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeNone); // Turn off clock output
 

    uFlowCfg = GetConfig();
    scale.set_scale(uFlowCfg.strainGaugeScale);
    scaleOffset = scale.read_average((byte) 10);

    Serial.print(F(" UFlow\r\n"));
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);
  

  GenerateKeyTable(nVcc,KeyTable);
  int k;
  for (k=0; k<7; k++) {
    Serial.print("KeyTable: ");
    Serial.print(KeyTable[k]);
    Serial.print("\n");
  }
//  selfTestError = 0xf; // FOR TEST

 
  mainStateInit();
//    Start Tasks
//      Note: 
//            For the Cooperative Multitasking, we need to end
//            each task with a "tasks" call to restart the task.
//            This allows the scheduler to run after the code in the task has executed.
  tasks.now(mainStateTask);
  tasks.now(buttonTask);
  tasks.now(serialTask);

}


void loop() 
{
  tasks.run();
}
