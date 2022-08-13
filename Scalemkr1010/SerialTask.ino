/*   serialTask
 *   
 *    
 *    
 *    
 */
 struct cmdType_ {
  char cmdNumber;
  String cmdName;
  String helpMsg; 
};

#define SERIAL_TASK_REP_RATE 60 /* 60msec */

#define SET_TIME  0
#define SCANI2C   1
#define RECALIBRATE 2
#define XFER 3
#define DELETE 4
#define BUTTONS 5
#define HELP  6


cmdType_ cmdList[] = {
  SET_TIME, String("TIME"),String("TIME - Set date/time: TIME Jan 04 2022 14 08 56"),
  SCANI2C, String("SCAN"),String("SCAN - scan for I2C devices"),
  RECALIBRATE, String("RECAL"),String("RECAL - recalibrate scale"),
  XFER, String("X"), String("X - Transfer data to PC: X <S,U>, S=SUMMARY.CSV, U=UROLFLOW.CSV"),
  DELETE, String("DELETE"), String("DELETE - delete CSV files on uSD card"),
  BUTTONS, String("BUTTONS"), String("BUTTONS - test buttons - power off to stop"),
  HELP, String("H"),String("H - help msg")
 };

extern boolean wifiConnected;

void terminalPrint(String s) {
  Serial.print(s);
  if (wifiConnected) client.print(s);
}

 void helpCmd() {
  int i;
  for (i=0; i<sizeof(cmdList)/sizeof(struct cmdType_); i++) {
    terminalPrint(cmdList[i].helpMsg);
    terminalPrint("\n");
  }
 }

void setNewDateTime(String argv[7]){
  char newDate[15];
  char newTime[15];
  char cTemp[15];
  String sDate = argv[1]+ " " + argv[2] + " " + argv[3];
  String sTime = argv[4] + " " + argv[5] + " " + argv[6];
  sDate.toCharArray(newDate,15);
  sTime.toCharArray(newTime,15);
  RtcDateTime newDateTime = RtcDateTime(newDate,newTime);
  Rtc.SetDateTime(newDateTime);
  newDateTime = Rtc.GetDateTime();
  Serial.print("RTC set to: ");
  printDateTime(newDateTime);
  Serial.print("\n");
}
 
 void doSerialCmd(char *sBuf) {
   const char s[2] = " ";
   const char sNewLine = '\n';
   char *cptr = sBuf;
   int argc = 0;
   char *token;
   int cmdNdx;
   String argv[7];
   String sTemp;
   File XferFile;
   char c;
   
   while (*cptr != 0) {  // Trim out the '\n' at end of line
     if ((*cptr == '\n') || (*cptr == '\r')) *cptr = 0;
     cptr++;
   }
#ifdef DEBUG
   Serial.print("Cmd: ");
   cptr = sBuf;
   while (*cptr != 0) {
    Serial.print(byte(*cptr++));
    Serial.print(" ");
   }
   Serial.print("\n");
#endif
   /* get the first token */
   token = strtok(sBuf, s);
   /* walk through other tokens */
   while( (token != NULL) && (argc < 7) ) {
      argv[argc] = String(token);
      argc++;
      token = strtok(NULL, s);
   }
   argc--;
   int NUM_CMDS = sizeof(cmdList)/sizeof(struct cmdType_);
   for (cmdNdx=0; cmdNdx<NUM_CMDS; cmdNdx++) {
     sTemp = cmdList[cmdNdx].cmdName;
     if (argv[0] == cmdList[cmdNdx].cmdName) {
        break;
     }
   }
   switch(cmdNdx){
      case SET_TIME: 
                     setNewDateTime(argv);
                     break;
      case SCANI2C: terminalPrint("Scan I2C - not implemented\n");
                    break;
      case RECALIBRATE: terminalPrint("Recalibrate\n");
                        setRecalRequest(true);
                        break;
      case XFER:  if (argv[1][0] == 'S') XferFile = SD.open("SUMMARY.CSV");
                  else if (argv[1][0] == 'U') XferFile = SD.open("UROFLOW.CSV");
                  if (XferFile) {
                    while ( XferFile.available() ) {
                      Serial.write(c = XferFile.read());
                      if (wifiConnected) server.write(c);
                    }
                    XferFile.close();
                    terminalPrint("!\n");   // Indicate end of transfer
                  }
                      break;
      case DELETE: SD.remove("SUMMARY.CSV");
                   Serial.print("Deleted SUMMARY.CSV\n");
                   SD.remove("UROFLOW.CSV");
                   Serial.print("Deleted UROFLOW.CSV\n");
                   break;
      case BUTTONS: TestButtons();
                    break;
      case HELP: helpCmd();
                 terminalPrint("\n");
                break;
      default: terminalPrint("No cmd found\n");
   }
   terminalPrint(">");
 }

void TestButtons( void ){
  int k;
  char tmpChar;
  int adcvalue;
  Serial.print("Button table\n");
  for (k=0; k<7; k++) {
    Serial.print("KeyTable: ");
    Serial.print(KeyTable[k]);
    Serial.print("\n");
  }
  for (k=0; k<30; k++) {
    adcvalue = analogRead(A1) >> 2; 
    Serial.print("ADC: ");
    Serial.print(adcvalue);
    for (tmpChar=0; tmpChar<5; tmpChar++)
    {
      if (abs(adcvalue - KeyTable[tmpChar]) < 5) break;
    }

    Serial.print("  Button=" + buttonNames[tmpChar]+"\n");
    delay(500); 
  }
}

void serialTask() {
    static char sBuf[30];
    static int sBufNdx = 0;
    
    if (Serial.available() > 0) {
       sBuf[sBufNdx++] = toupper(Serial.read()); 
    }
    if ( (sBufNdx > 0) && ( (sBuf[sBufNdx-1] == '\n')|| (sBufNdx >= sizeof(sBuf)) ) )  {
      doSerialCmd(sBuf);
      sBufNdx = 0;
    }
    tasks.after(SERIAL_TASK_REP_RATE,serialTask);
  }

 
