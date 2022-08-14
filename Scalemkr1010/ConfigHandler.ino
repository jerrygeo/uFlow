// ConfigHandler.ino
//   Module to handle reading and writing the configuartion file that holds the calibration data.

#define CFG_PARAMETER_COUNT 3

/*******************************/
/*    Configuration file (UFLOW.CFG) format
 *       All entries are in ASCII format with Name field, tab, then value
 *       Record # Description            Name        Format       Example
 *          1      Software version,     VERSION      %4.2f          2.01
 *          2      Scale Calibration     CALFACTOR     %d            4900
 *          3      16 bit checksum       CHECKSUM      %d           43327
 *       
 *       
 *******************************/

struct ConfigStruct defaultCfg = {
  2.01,
  40,
  0,
  false,
  "No Error"};


/*!   Function  getCksum
 *      Inputs  ConfigStruct myConfig
 *      Outputs Returns checksum
 *      Processing - Checksum is computed on myConfig as sum modulo 256
*                     of the first 8 bytes of myConfig
 *                   
 */
int16_t getCksum(struct ConfigStruct myConfig){
  int16_t sum=0;
  byte *bptr;

#ifdef DEBUG
  Serial.print("\nConfig in hex:  \n");
#endif
  bptr = (byte *)&myConfig;
  for (int ndx=0; ndx<(sizeof(myConfig.swVersion)+sizeof(myConfig.strainGaugeScale)); ndx++)
  {
#ifdef DEBUG
    Serial.print(ndx); Serial.print(" ");
    Serial.print(*bptr, HEX);
    Serial.print("\n");
#endif
    sum += *bptr++;
  }
  Serial.print("\n  chkSum = "); Serial.print(sum); Serial.print("\n");
  return sum;
}

/*!   Function  writeCfg
 *      Inputs  ConfigStruct newCfg
 *      Outputs Writes newCfg to SD card
 *      Processing - Erase UFLOW.CFG if it exists.
 *                   Try to open new UFLOW.CFG - set selfTestError if we can't.
 *                     This will cause MainStateTask to stop with an error
 *                   Compute checksum and write new config file.
 *                   Returns 0 if successful, -1 if error
 *                   
 */
int writeCfg(struct ConfigStruct newCfg) {
  int writeStatus;
  char cbuf[40];
  File configFile;
  
  writeStatus = -1; // Write failed unless it is OK below
  if (SD.exists("UFLOW.CFG")) {
    SD.remove("UFLOW.CFG");
  }
  configFile = SD.open("UFLOW.CFG", FILE_WRITE);
  if (!configFile) {
    newCfg.configErr = true;
    newCfg.configErrMsg = F("Can't create new config file.\n");
    Serial.print(newCfg.configErrMsg);
    selfTestError |= CONFIG_OPEN_ERROR;
  }
  else { // Create new config file
    snprintf(cbuf, sizeof(cbuf)-1, "SW Version:\t %4.2f\n",newCfg.swVersion);
    configFile.print(cbuf);
    Serial.print(cbuf);
    snprintf(cbuf, sizeof(cbuf)-1, "Scale Factor:\t%d\n",newCfg.strainGaugeScale);
    configFile.print(cbuf);
    Serial.print(cbuf);
    newCfg.checkSum = getCksum(newCfg);
    snprintf(cbuf,sizeof(cbuf)-1,"Checksum:\t%d\n",getCksum(newCfg));
    configFile.print(cbuf);
    Serial.print(cbuf);
    writeStatus = 0;  // Assume write was OK.
    Serial.print(F("Updated cfg file\nScale Factor = "));
    Serial.print(newCfg.strainGaugeScale); Serial.print("\n");
    Serial.print("New checksum: "); Serial.print(newCfg.checkSum); Serial.print("\n");
  }
  configFile.close();
  return writeStatus;
}

/*!   Function  readConfig
 *      Inputs  File cfgFile - file holding config data with scale factor
 *              float *fparams - array that will hold parameters read from file
 *              String *ErrMsg - string to put error message in if there are problems
 *      Outputs Updates array fparams and string Errmsg
 *      Processing - Read CFG_PARAMETER_COUNT parameters from cfgFile, one parameter per
 *                     line in the file.
 *                   If tab character not found each line, set error message & output to
 *                     Serial (USB) port and use the default configuration.
 *                   If any of the parameter values are 0, set error message and use
 *                     default configuration.
 *                   Returns number of parameters read
 *                   
 */
int readConfig(File cfgFile, float *fparams, String *ErrMsg){
  String errMsgs[] = {
    "Bad SW version\n",
    "Bad scale factor\n",
    "Bad scale checksum\n"
  };
  String recordLine;
  String parameter;
  int parameterNdx;
  int cksum;
  int cfgNdx;
  char cbuf[40];

  Serial.print("Cfg:\n");
   for (cfgNdx=0; cfgNdx<CFG_PARAMETER_COUNT; cfgNdx++) {
    recordLine = cfgFile.readStringUntil('\n');      // Read line of config file
#ifdef DEBUG
    Serial.print("\nRecord ");Serial.print(cfgNdx);Serial.print(": ");Serial.print(recordLine);
#endif
    parameterNdx = recordLine.indexOf('\t'); 
    if (parameterNdx == -1) { //Index of start of parameter
      Serial.print("No data in parameter "); Serial.print(cfgNdx);
      Serial.print("\nRecord: "); Serial.print(recordLine);
      *ErrMsg = errMsgs[cfgNdx];
      Serial.print("\nSetting to default cfg\n");
      writeCfg(defaultCfg);
      break;
    }
    parameter = recordLine.substring(parameterNdx+1);
    Serial.print("\n Parameter "); Serial.print(cfgNdx); Serial.print(" "); Serial.print(parameter);
    fparams[cfgNdx] = parameter.toFloat();
    Serial.print(" Value = "); Serial.print(fparams[cfgNdx]);
    if (fparams[cfgNdx] == 0.) {
      *ErrMsg = errMsgs[cfgNdx];
      break;
    }
  }
  return cfgNdx;
}

/*!   Function  GetConfig
 *      Inputs  .
 *              float *fparams - array that will hold parameters read from file
 *              String *ErrMsg - string to put error message in if there are problems
 *      Outputs Updates array fparams and string Errmsg
 *      Processing - Opens CONFIG.CFG file. If a problem opening file or bad parameters,
 *                    print msg to Serial (USB) and write default config to CONFIG.CFG.
 */  
struct ConfigStruct GetConfig(){
  File configFile = SD.open("UFLOW.CFG", FILE_READ);
  char cbuf[20];
  struct ConfigStruct currentConfig;
  float paramBuf[CFG_PARAMETER_COUNT];
  int calculatedCkSum;
  String errMsg;
  
  currentConfig.configErr = false;
  
  if (!configFile){ // Can't open config file
    Serial.print(F("Can't open config file.\n"));
    currentConfig = defaultCfg;
    if (writeCfg(defaultCfg) == -1) {
       currentConfig.configErr = true;
       currentConfig.configErrMsg = "Cfg file error";
    selfTestError |= CONFIG_OPEN_ERROR;

    }
  }
  else {  // Config file present - read config data 
    Serial.print("Reading file\n");
    if (readConfig(configFile,paramBuf, &errMsg) < CFG_PARAMETER_COUNT) { // Problem reading file
      currentConfig.configErr = true;
      currentConfig.configErrMsg = errMsg;
      Serial.print(currentConfig.configErrMsg);
    }
    else {                      // OK - get the data from the file
      currentConfig.swVersion = paramBuf[0];
      currentConfig.strainGaugeScale = (int) paramBuf[1];
      currentConfig.checkSum = (int) paramBuf[2];
      currentConfig.configErr = false;
      calculatedCkSum = getCksum(currentConfig);
      Serial.print("Calculated cksum: "); Serial.print(calculatedCkSum); Serial.print("\n");
      if (calculatedCkSum == currentConfig.checkSum) {
        currentConfig.configErrMsg = "No Error";
      }
      else {
        Serial.print("Config checksum error - writing default config\n");
        configFile.seek(0); // Rewind
        currentConfig = defaultCfg;
        if (writeCfg(defaultCfg) == -1) {
          currentConfig.configErr = true;
          currentConfig.configErrMsg = "Cfg file error";
          Serial.print("Error writing cfg file.");
        }
      }
    }
  }
  configFile.close();
#ifdef DEBUG 
  Serial.print("In GetConfig:\n");
  sprintf(cbuf,"  Version: %4.2f\n",currentConfig.swVersion);
  Serial.print(cbuf);
  sprintf(cbuf,"  Scale: %4d\n",currentConfig.strainGaugeScale);
  Serial.print(cbuf);
  sprintf(cbuf,"  Cksum: 0x%04x\nCurrentConfig: ",currentConfig.checkSum);
  Serial.print(cbuf);
  byte *pbyte = (byte *)&currentConfig;
  for (int i=0; i<8; i++) {
    Serial.print(*pbyte++);
    Serial.print("  ");
  }
#endif  
 
  return currentConfig;
}
