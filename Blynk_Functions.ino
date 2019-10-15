// This function will run every time Blynk connection is established
BLYNK_CONNECTED() 
{
  if (isFirstConnect)
  {
    //Blynk.syncVirtual(V10,V11,V12,V13,V22); //Read start/stop/sunrise/sunset times, fan on temp
    Blynk.virtualWrite(V15,1);
    isFirstConnect = false;
    Blynk.run();
  }
}
// This is called when Smartphone App is opened
BLYNK_APP_CONNECTED() {
  BLYNK_LOG("App Connected");
  appConnected = true;
  if(LEDMode == 1)updateBlynkSliders();
}
// This is called when Smartphone App is closed
BLYNK_APP_DISCONNECTED() {
  BLYNK_LOG("App Disconnected");
  appConnected = false;
}
BLYNK_WRITE(V1) {// slider widget to set the maximum led level from the Blynk App.
  int value;// = param.asInt();
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  //If we're not in normal operation
  if(LEDMode != 1)
  {
    tempSetting.chValue[0] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch1 current to %d",value);
  }
}
BLYNK_WRITE(V2) {// slider widget to set the led level from the Blynk App.
  int value;
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  if(LEDMode != 1)
  {
    tempSetting.chValue[1] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch2 current to %d",value);
  }
}
BLYNK_WRITE(V3) {// slider widget to set the led level from the Blynk App.
  int value;
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  if(LEDMode != 1)
  {
    tempSetting.chValue[2] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch3 current to %d",value);
  }
}
BLYNK_WRITE(V4) {// slider widget to set the led level from the Blynk App.
  int value;
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  if(LEDMode != 1)
  {
    tempSetting.chValue[3] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch4 current to %d",value);
  }
}
BLYNK_WRITE(V5) {// slider widget to set the led level from the Blynk App.
  int value;
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  if(LEDMode != 1)
  {
    tempSetting.chValue[4] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch5 current to %d",value);
  }
}
BLYNK_WRITE(V6) {// slider widget to set the led level from the Blynk App.
  int value;
  float valueF = param.asFloat();
  valueF = valueF * 40.95;
  value = valueF;
  value = constrain(value,0,4095);
  if(LEDMode != 1)
  {
    tempSetting.chValue[5] = value;
    pwm.setPWM(0, 0, value);
    BLYNK_LOG("Setting ch6 current to %d",value);
  }
}
BLYNK_WRITE(V10) //Time 
{
  tempSetting.settingTime = param[0].asLong();

  sprintf(Time, "%02d:%02d:%02d", tempSetting.settingTime/3600 , (tempSetting.settingTime / 60) % 60, 0);
  BLYNK_LOG("Time selected is: %s",Time);
}
BLYNK_WRITE(V11) //Ramp duration
{
  tempSetting.settingFade = param[0].asInt();
  tempSetting.settingFade = tempSetting.settingFade * 60; //change to seconds
  BLYNK_LOG("Fade time selected (sec): %d",tempSetting.settingFade);
}
BLYNK_WRITE(V12) //enabled
{
  tempSetting.enabled = param[0].asInt();
  BLYNK_LOG("Setting enabled?: %s",tempSetting.enabled ? "true" : "false");
}
BLYNK_WRITE(V15) {// menu input to select LED mode
  LEDMode = param.asInt();
  int timeslot=0;
  int i;
  float valueF;
  if(LEDMode == 1) //normal operation
  {
    if (year() != 1970)
    {
      //Set LEDs to expected value based on time as long as time is set
      timeslot = smartLEDStartup();
      for (i = 0; i < numCh; i = i + 1)
      {
        tempSetting.chValue[i] = LEDchannels[i].currentPWM;
      }
      tempSetting.settingTime = LEDsettings[timeslot].settingTime;
      tempSetting.settingFade = LEDsettings[timeslot].settingFade;
      tempSetting.enabled = LEDsettings[timeslot].enabled;
    }
  }
  if(LEDMode > 1) //time slot selected, load slot settings
  {
    timeslot = LEDMode-2;
    for (i = 0; i < numCh; i = i + 1)
    {
      tempSetting.chValue[i] = LEDsettings[timeslot].chValue[i];
      pwm.setPWM(i, 0, tempSetting.chValue[i]);
    }
    tempSetting.settingTime = LEDsettings[timeslot].settingTime;
    tempSetting.settingFade = LEDsettings[timeslot].settingFade;
    tempSetting.enabled = LEDsettings[timeslot].enabled;
  }
  BLYNK_LOG("Updating setting tab for setting %d",timeslot);
  for (i = 0; i < numCh; i = i + 1)
  {
    valueF = (float)tempSetting.chValue[i] / (float)40.95;
    BLYNK_LOG("     Channel %d Value: %.1f",i,valueF);
    switch (i) {
      case 0:
        Blynk.virtualWrite(V1, valueF);
        break;
      case 1:
        Blynk.virtualWrite(V2, valueF);
        break;
      case 2:
        Blynk.virtualWrite(V3, valueF);
        break;
      case 3:
        Blynk.virtualWrite(V4, valueF);
        break;
      case 4:
        Blynk.virtualWrite(V5, valueF);
        break;
      case 5:
        Blynk.virtualWrite(V6, valueF);
        break;
    }
    Blynk.run();
  }
  char tz[] = "America/Vancouver";
  Blynk.virtualWrite(V10, tempSetting.settingTime,0,tz);
  Blynk.run();
  BLYNK_LOG("     Time: %d:%d",tempSetting.settingTime/3600,tempSetting.settingTime%60);
  Blynk.virtualWrite(V11, tempSetting.settingFade/60);
  Blynk.run();
  BLYNK_LOG("     Fade duration(s): %d",tempSetting.settingFade);
  Blynk.virtualWrite(V12, tempSetting.enabled);
  Blynk.run();
  BLYNK_LOG("     Enabled: %d",tempSetting.enabled);
}

BLYNK_WRITE(V16) {// Save button for LED settings
  int buttonState = param.asInt();
  int timeslot = LEDMode-2;
  if(buttonState == 1)
  {
    //V15 LEDMode: 1=Normal Operation, 2-6=Time slots 1-5
    if(LEDMode > 1)
    {
      timeslot = constrain(timeslot,0,numSettings);
      saveSettingFile(timeslot);//Save temp settings into FS
      loadSettingFile(timeslot);//Load settings from FS
    }
  }
}
BLYNK_WRITE(V22) {
  fanOnTemp = param.asInt();
}

//Updates Blynk sliders to current value, channel is optional and if not passed to function
//the function will update all sliders
void updateBlynkSliders(boolean updateAll)
{
  float valueF;
  int i;
  if(updateAll)
  {
    for (i = 0; i < numCh; i = i + 1)
    {
      valueF = (float)LEDchannels[i].currentPWM / (float)40.95;
      BLYNK_LOG("Channel %d Current: %.1f%",i,valueF);
      switch (i) {
        case 0:
          Blynk.virtualWrite(V1, valueF);
          break;
        case 1:
          Blynk.virtualWrite(V2, valueF);
          break;
        case 2:
          Blynk.virtualWrite(V3, valueF);
          break;
        case 3:
          Blynk.virtualWrite(V4, valueF);
          break;
        case 4:
          Blynk.virtualWrite(V5, valueF);
          break;
        case 5:
          Blynk.virtualWrite(V6, valueF);
          break;
      }
      Blynk.run();
    }
  }else
  {
    unsigned int channel = second() % numCh;
    valueF = (float)LEDchannels[channel].currentPWM / (float)40.95;
    switch (channel) {
        case 0:
          Blynk.virtualWrite(V1, valueF);
          break;
        case 1:
          Blynk.virtualWrite(V2, valueF);
          break;
        case 2:
          Blynk.virtualWrite(V3, valueF);
          break;
        case 3:
          Blynk.virtualWrite(V4, valueF);
          break;
        case 4:
          Blynk.virtualWrite(V5, valueF);
          break;
        case 5:
          Blynk.virtualWrite(V6, valueF);
          break;
      }
  }
}
