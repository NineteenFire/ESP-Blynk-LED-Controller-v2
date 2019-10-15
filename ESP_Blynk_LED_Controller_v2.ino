#define BLYNK_PRINT Serial  // Comment this out to disable prints and save space
//#define BLYNK_DEBUG         // Comment this out to disable prints and save space

#include <FS.h>                   // https://github.com/tzapu/WiFiManager This Library needs to be included FIRST!
#include <BlynkSimpleEsp8266.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <OneWire.h> // network library to communicate with the DallasTemperature sensor, 
#include <DallasTemperature.h>  // library for the Temp sensor itself
//included libraries for WiFiManager - AutoConnectWithFSParameters
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <EEPROM.h>


void updateBlynkSliders(boolean updateAll=true);

byte pSDA=4;
byte pSCL=5;
byte pEnFAN=16;
byte pFANPWM=0;
byte pOneWire=13;
byte pOnBoardLED=2;

// WiFiManager
char blynk_token[34] = "BLYNK_TOKEN";//added from WiFiManager - AutoConnectWithFSParameters
//flag for saving data
bool shouldSaveConfig = false;
//callback notifying the need to save config
void saveConfigCallback () {
  BLYNK_LOG("Should save config");
  shouldSaveConfig = true;
}

bool isFirstConnect = true;
bool smartStartupRun = false;
bool appConnected = false;
bool tempSensorConnected = true;

char Time[16];

unsigned long startsecond = 0;        // time for LEDs to ramp to full brightness
unsigned long stopsecond = 0;         // finish time for LEDs to ramp to dim level from full brightness
unsigned long sunriseSecond = 0;      // time for LEDs to start ramping to dim level
unsigned long sunsetSecond = 0;       // finish time for LEDs to dim off
unsigned long moonStartSecond = 0;      // time for LEDs to start ramping to moonlight level
unsigned long moonStopSecond = 0;       // time for LEDs to ramp off
unsigned long nowseconds = 0;         // time  now  in seconds

//0=not ramping, 1-5 = setting slot
unsigned int fadeInProgress = 0;

unsigned long fadeStartTimeMillis = 0;
unsigned long fadeStartTimeSeconds = 0;
unsigned long fadeTimeSeconds = 0;
unsigned long fadeTimeMillis = 0;

//How many channels and settings we want to maintain
const int numCh = 6;
const int numSettings = 5;

//Struct for storing channel values
struct LEDCh
{
  unsigned int fadeIncrementTime = 0;
  unsigned int currentPWM = 0;
  unsigned int lastPWM = 0;
  unsigned int targetPWM = 0;
};
typedef struct LEDCh LEDs;
LEDs LEDchannels[numCh];

struct LEDSetting // LED channel settings
{
  boolean enabled;
  long settingTime;   //second of day to start, 0-86400
  int settingFade;    //seconds to ramp, 0-3600
  int chValue[numCh]; //PWM value, 0-4095
};
typedef struct LEDSetting settings;
settings LEDsettings[numSettings];  //Array to store time slot settings
settings tempSetting;               //Setting to store values incoming from Blynk before saving

int LEDMode = 1; //1=operating, 2-6=setting parameters

// Data wire is plugged into port 2 on the Wemos
#define ONE_WIRE_BUS D7

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// array to hold device addresses (can increase array length if using more sensors)
DeviceAddress tempSensors[3];
int numTempSensors;
int fanOnTemp = 0;

WidgetRTC rtc;
WidgetLED fanLED(V14);
BlynkTimer timer;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void setup()
{
  Wire.begin(pSDA,pSCL);
  //Wire.setClock(400000L);
  ArduinoOTA.begin();
  Serial.begin(115200);
  BLYNK_LOG("Starting setup routine");
  
  pinMode(pOnBoardLED, OUTPUT);
  pinMode(pFANPWM, OUTPUT);
  pinMode(pEnFAN, OUTPUT);
  digitalWrite(pOnBoardLED, LOW);
  digitalWrite(pFANPWM, LOW);
  digitalWrite(pEnFAN, LOW);
  
  pwm.begin();
  pwm.setPWMFreq(250);

  //Start dallas temperature temp sensors
  sensors.begin();
  // locate devices on the bus
  numTempSensors = sensors.getDeviceCount();
  BLYNK_LOG("Found %d temperature sensors.",numTempSensors);
  for( int i = 0 ; i < numTempSensors ; i++)
  {
    if (!sensors.getAddress(tempSensors[i], i))
    {
      BLYNK_LOG("Unable to find address for Device %d",i);
    }
  }
  if(numTempSensors == 0)
  {
    tempSensorConnected = false;
  }

  //The following code is borrowed from WiFiManager
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  BLYNK_LOG("Mounting FS...");

  if (SPIFFS.begin()) {
    BLYNK_LOG("Mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      BLYNK_LOG("Config file exists");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        BLYNK_LOG("Opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, buf.get());
        if(error)
        {
          // do something if error?
        }
        JsonObject json = doc.as<JsonObject>();
        serializeJson(json, Serial);
        if (!json.isNull())
        {
          BLYNK_LOG("Parsed json");
          strcpy(blynk_token, json["blynk_token"]);
          BLYNK_LOG("Blynk Token in memory: %s",blynk_token);
        } else 
        {
          BLYNK_LOG("Failed to load json config");
        }
        configFile.close();
      }
    }else{
      BLYNK_LOG("File config.json does not exist",blynk_token);
    }
  } else {
    BLYNK_LOG("Failed to mount FS");
  }
  //end read
  
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);
  
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Add blynk parameter here
  wifiManager.addParameter(&custom_blynk_token);

  // Reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality (default: 8%)
  wifiManager.setMinimumSignalQuality(10);

  // Sets timeout until portal gets turned off useful to make it all retry or go to sleep
  //wifiManager.setTimeout(120);

  // Sets debug output based on blynk logging, reduces serial prints if we don't need it
#ifdef BLYNK_PRINT
  wifiManager.setDebugOutput(true);
#else
  wifiManager.setDebugOutput(false);
#endif

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("ESP-BLYNK LED AP")) {
    BLYNK_LOG("Failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  BLYNK_LOG("LED Controller connected :)");

  //read updated parameters
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    BLYNK_LOG("Saving config");
    DynamicJsonDocument doc(512);
    JsonObject json = doc.to<JsonObject>();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      BLYNK_LOG("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);
    configFile.close();
    //end save
  }

  setSyncInterval(1000); //make sure time syncs right away
  Blynk.config(blynk_token);
  while (Blynk.connect() == false)
  {
    // Wait until connected, need to handle case where wifi connects but blynk auth invalid...
    
    if((shouldSaveConfig == false)&&(blynk_token == "BLYNK_TOKEN"))
    {
      //This will erase WiFi settings forcing a new AP where Blynk_token can be entered
      //WiFi.disconnect(true); 
      //ESP.reset();
      //OR
      WiFi.begin("FakeSSID","FakePW");     // Replace current WiFi credentials with fake ones
      delay(1000);
      ESP.restart();
    }
    
  }
  rtc.begin();

  //Load settings from FS, or create new settings files if they don't exist
  for (int i = 0; i < numSettings; i = i + 1)
  {
    if (!loadSettingFile(i))
    {
      BLYNK_LOG("Failed to load setting file %d, creating now",i);
      if(!createSettingFile(i)){
        BLYNK_LOG("Failed to create setting file %d",i);
        Blynk.notify("Failed to load or create setting file, check device FS");
      }
    } else {
      BLYNK_LOG("Setting %d loaded",i);
    }
  }
  
  timer.setInterval(500L, ledFade);           // adjust the led lighting every 500ms
  timer.setInterval(1000L, checkSchedule);    // check every second if fade should start
  timer.setInterval(15000L, checkTemp);       // check heatsink temperature every 15 seconds
  timer.setInterval(60000L, reconnectBlynk);  // check every 60s if still connected to server
  
  digitalWrite(pOnBoardLED, HIGH); //ensure light is OFF
  Blynk.notify("LED Controller ONLINE");
  BLYNK_LOG("End of startup.");
}
void loop()
{
  ArduinoOTA.handle();
  if (Blynk.connected()) {
    Blynk.run();
  }
  timer.run();
}

//===Functions===

void reconnectBlynk() {
  if (!Blynk.connected())
  {
    //Turn onboard LED on to indiciate there is an issue
    digitalWrite(pOnBoardLED, LOW);
    if (Blynk.connect())
    {
      digitalWrite(pOnBoardLED, HIGH);
      BLYNK_LOG("Reconnected");
    }else
    {
      digitalWrite(pOnBoardLED, LOW);
      BLYNK_LOG("Not reconnected");
    }
  }else
  {
    //Turn onboard LED off
    digitalWrite(pOnBoardLED, HIGH);
    clockDisplay();
  }
}

void checkSchedule()        // check if ramping should start
{
  int i;
  int y;
  int difference;
  if (year() != 1970) 
  {
    nowseconds = ((hour() * 3600) + (minute() * 60) + second());
    
    if(smartStartupRun == false)
    {
      smartLEDStartup();
      smartStartupRun = true;
      setSyncInterval(600000); //set time sync to every 10 minutes
    }
  }else
  {
    nowseconds = 86401; //set seconds out of range to avoid un-wanted ramps
  }

  //Check if the current time is equal to any setting time
  for(i = 0; i < numSettings; i = i + 1)
  {
    //Check if the current second matches setting time (check for up to a minute after the start time to ensure it isn't 
    //missed) and that setting hasn't been triggered
    if((nowseconds >= LEDsettings[i].settingTime)&&(nowseconds <= (LEDsettings[i].settingTime+60))&&(fadeInProgress != i))
    {
      sprintf(Time, "%02d:%02d:%02d", hour() , minute(), second());
      BLYNK_LOG("%s\tStarting setting %d",Time,i);
      fadeInProgress = i;
      fadeStartTimeMillis = millis();
      fadeStartTimeSeconds = now();
      for(y = 0; y < numCh; y = y + 1)
      {
        LEDchannels[y].targetPWM = LEDsettings[i].chValue[y];//LEDchannels[i].dimPWM;
        LEDchannels[y].lastPWM = LEDchannels[y].currentPWM;
        if(LEDchannels[y].targetPWM > LEDchannels[y].lastPWM){difference = (LEDchannels[y].targetPWM - LEDchannels[y].lastPWM);}
        if(LEDchannels[y].targetPWM < LEDchannels[y].lastPWM){difference = (LEDchannels[y].lastPWM - LEDchannels[y].targetPWM);}
        LEDchannels[y].fadeIncrementTime = fadeTimeMillis / difference;
      }
    }
  }
  if(appConnected && (LEDMode == 1))
  {
    //only update one slider per second while connected to app to reduce traffic
    updateBlynkSliders(false);
  }
}

void ledFade()
{
  int i;
  if(fadeInProgress != 0)
  {
    unsigned long timeElapsed = millis() - fadeStartTimeMillis; //time since the start of fade
    for (i = 0; i < numCh; i = i + 1)
    {
      if(LEDchannels[i].currentPWM != LEDchannels[i].targetPWM)
      {
        //if adjustments are necessary
        if(LEDchannels[i].lastPWM < LEDchannels[i].targetPWM)
        {
          //value increasing
          LEDchannels[i].currentPWM = LEDchannels[i].lastPWM + (timeElapsed / LEDchannels[i].fadeIncrementTime);
        }
        if(LEDchannels[i].lastPWM > LEDchannels[i].targetPWM)
        {
          //value decreasing
          LEDchannels[i].currentPWM = LEDchannels[i].lastPWM - (timeElapsed / LEDchannels[i].fadeIncrementTime);
        }
        //if target == last set to target
        if(LEDchannels[i].lastPWM == LEDchannels[i].targetPWM)
        {
          LEDchannels[i].currentPWM = LEDchannels[i].targetPWM;
        }
      }
      LEDchannels[i].currentPWM = constrain(LEDchannels[i].currentPWM,0,4095);
    }
    if(now() > (fadeStartTimeSeconds + fadeTimeSeconds))
    {
      fadeInProgress = 0;
      for (i = 0; i < numCh; i = i + 1) {
        LEDchannels[i].currentPWM = LEDchannels[i].targetPWM;
      }
    }
    if(LEDMode==1)
    {
      writeLEDs();
    }
  }
}

void writeLEDs() {
  int i;
  for (i = 0; i < numCh; i = i + 1) {
    pwm.setPWM(i,0,LEDchannels[i].currentPWM);
  }
}

void checkTemp()
{
  int fanPWM = 0;
  int i = 0;
  if(tempSensorConnected)
  {
    //read all sensors and keep track of maximum temperature
    sensors.requestTemperatures();
    float temperature[numTempSensors];
    float maxTemperature = 0.0;
    for(i = 0 ; i < numTempSensors ; i++)
    {
      temperature[i] = sensors.getTempC(tempSensors[i]);
      if(temperature[i] > maxTemperature)maxTemperature=temperature[i];
    }
    
    //write the maximum temperature to Blynk
    Blynk.virtualWrite(V20, maxTemperature);
    
    //if higher than fan set temperature then turn fan on
    int intTemp = maxTemperature; //temp as integer
    if(intTemp > fanOnTemp)
    {
      //Calculate PWM by temp over fanOnTemp, at 0C above run at 10%, 20C above 100%
      fanPWM = intTemp - fanOnTemp;
      fanPWM = constrain(fanPWM,0,20);
      fanPWM = map(fanPWM,0,20,100,1023);
      analogWrite(pFANPWM,fanPWM);
      //turn fan on
      digitalWrite(pEnFAN,HIGH);
      fanLED.on();
    }
    if(intTemp < (fanOnTemp-2))
    {
      //turn fan off
      digitalWrite(pEnFAN,LOW);
      analogWrite(pFANPWM,0);
      fanLED.off();
    }
  }else
  {
    //turn fan on when any channel PWM is higher as % than fanOnTemp (which is 0-100C)
    //get the current highest channel and highest set point
    unsigned int maxCurrentPWM = 0;
    unsigned int maxSetPWM = 0;
    //Get highest current PWM
    for (i = 0; i < numCh; i = i + 1)
    {
      if(LEDchannels[i].currentPWM > maxCurrentPWM)
      {
        maxCurrentPWM = LEDchannels[i].currentPWM;
      }
    }
    //Get the highest setting
    for (int y = 0; y < numSettings; y = y + 1)
    {
      for (i = 0; i < numCh; i = i + 1)
      {
        if(LEDsettings[y].chValue[i] > maxSetPWM)
        {
          maxSetPWM = LEDsettings[y].chValue[i];
        }
      }
    }
    
    //change current PWM value from 12-bit PWM to 0-100 range
    maxCurrentPWM = map(maxCurrentPWM,0,4095,0,100); 
    
    if(maxCurrentPWM > fanOnTemp)
    {
      int fanRange = 0;
      //Calculate fanPWM
      if(maxSetPWM > fanOnTemp)
      {
        fanRange = maxSetPWM - fanOnTemp; //Range to cover 100%PWM to 10%PWM
      }else
      {
        fanRange = 100 - fanOnTemp;
      }
      int fanPWM = maxCurrentPWM - fanOnTemp; //Amount above just turning on
      fanPWM = map(fanPWM,0,fanRange,102,1023);
      analogWrite(pFANPWM,fanPWM);
      //turn fan on
      digitalWrite(pEnFAN,HIGH);
      fanLED.on();
    }else
    {
      //turn fan off
      digitalWrite(pEnFAN,LOW);
      analogWrite(pFANPWM,0);
      fanLED.off();
    }
  }
}

int smartLEDStartup()
{
  int s = 0; //used to track time through a day
  int i = 0;
  int difference;
  int secondsIntoRamp = 0;
  nowseconds = ((hour() * 3600) + (minute() * 60) + second());
  BLYNK_LOG("Starting startup routine...");
  BLYNK_LOG("nowseconds: %d", nowseconds);

  //0-4=setting slot time
  byte LEDStartMode = 99; 

  //go through the schedule up to the current time
  for(s=0 ; s < nowseconds ; s++)
  {
    for(i=0 ; i < numSettings ; i++)//check each setting time for every second
    {
      if((s == LEDsettings[i].settingTime)&&(LEDsettings[i].enabled))
      {
        LEDStartMode = i;
        secondsIntoRamp = 0;
      }
    }
    secondsIntoRamp++;
  }
  if(LEDStartMode == 99)//if we don't hit a ramp, ie if it's 12:01AM, finish the day
  {
    for(s = nowseconds ; s < 86400 ; s++)
    {
      for(i=0 ; i < numSettings ; i++)//check each setting time for every second
      {
        if((s == LEDsettings[i].settingTime)&&(LEDsettings[i].enabled))
        {
          LEDStartMode = i;
          secondsIntoRamp = 0;
        }
      }
      secondsIntoRamp++;
    }
  }
  
  if(LEDStartMode == 99)//if we don't hit a ramp, ie if it's 12:01AM, go backwards
  {
    s = 86399;
    while((LEDStartMode == 99)&&(s > nowseconds))
    {
      for(i=0 ; i < numSettings ; i++)
      {
        if(LEDsettings[i].enabled)
        {
          if(s == (LEDsettings[i].settingTime+LEDsettings[i].settingFade))
          {
            //Hit the end of a ramp
            LEDStartMode = i;
            secondsIntoRamp = LEDsettings[i].settingFade;
          }
          if(s == LEDsettings[i].settingTime)
          {
            //Hit the start of the ramp, seconds till midnight is how far into ramp we are
            LEDStartMode = i;
            secondsIntoRamp = 86400 - s;
          }
        }
      }
      s--;
    }
  }
  //set LEDs based on LEDStartMode, only supports the end of ramp, not midramp
  BLYNK_LOG("Setting lights to setting %d",LEDStartMode);
  for (i = 0; i < numCh; i = i + 1)
  {
    LEDchannels[i].currentPWM = LEDsettings[LEDStartMode].chValue[i];
    
    BLYNK_LOG("     Channel %d: %d",i,LEDchannels[i].currentPWM);
  }
  writeLEDs();
  return LEDStartMode;
}

// returns scaling factor for current day in lunar cycle
byte lunarCycleScaling()
{
  byte scaleFactor[] = {25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,
                      95,90,85,80,75,70,65,60,55,50,45,40,35,30};
  tmElements_t fixedDate = {0,35,20,0,7,1,0};
  long lp = 2551443;
  time_t newMoonCycle = makeTime(fixedDate);
  long phase = (now() - newMoonCycle) % lp;
  long returnValue = ((phase / 86400) + 1);
  return scaleFactor[returnValue];
}

// returns day in lunar cycle 0-29
byte getLunarCycleDay()
{
  tmElements_t fixedDate = {0,35,20,0,7,1,0};
  long lp = 2551443;
  time_t newMoonCycle = makeTime(fixedDate);
  long phase = (now() - newMoonCycle) % lp;
  long returnValue = ((phase / 86400) + 1);
  return returnValue;
}
// Digital clock display of the time
void clockDisplay()
{
  //Write to time display
  /*if(isAM())sprintf(Time, "%d:%02d AM", hourFormat12() , minute());
  if(isPM())sprintf(Time, "%d:%02d PM", hourFormat12() , minute());
  Blynk.virtualWrite(V9, Time);*/
  //Write to TIME setting if running in normal operation
  if(LEDMode == 1)
  {
    char tz[] = "America/Vancouver";
    Blynk.virtualWrite(V10, nowseconds,0,tz);
  }
}
