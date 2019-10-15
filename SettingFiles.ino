boolean loadSettingFile(int slot)
{
  //Create file name
  char path[20];
  sprintf(path,"/setting%d.txt", slot);
  
  //Open file
  File settingFile = SPIFFS.open(path, "r");
  if (!settingFile) {
    BLYNK_LOG("Failed to open setting file\n");
    return false;
  }
  
  // Allocate a buffer to store contents of the file.
  size_t size = settingFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  
  // Read contents of file
  settingFile.readBytes(buf.get(), size);

  //Create JSON document
  const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(5) + 50;
  DynamicJsonDocument doc(capacity);

  //Parse JSON data
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    BLYNK_LOG("Failed to parse setting file");
    return false;
  }
  serializeJson(doc, Serial);
  Serial.print("\n");

  //Store values into setting array
  LEDsettings[slot].enabled = doc["enabled"];
  LEDsettings[slot].settingTime = doc["time"];
  LEDsettings[slot].settingFade = doc["fade"];
  JsonArray channels = doc["channels"];
  for(int i = 0; i < numCh; i = i + 1)
  {
    LEDsettings[slot].chValue[i] = channels[i];
  }
  int settingSlot = doc["settingSlot"];
  settingFile.close();
}

boolean saveSettingFile(int slot)
{
  const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(5);
  DynamicJsonDocument doc(capacity);
  
  doc["settingSlot"] = slot;
  doc["enabled"] = tempSetting.enabled;
  doc["time"] = tempSetting.settingTime;
  doc["fade"] = tempSetting.settingFade;
  
  JsonArray channels = doc.createNestedArray("channels");
  channels.add(tempSetting.chValue[0]);
  channels.add(tempSetting.chValue[1]);
  channels.add(tempSetting.chValue[2]);
  channels.add(tempSetting.chValue[3]);
  channels.add(tempSetting.chValue[4]);
  channels.add(tempSetting.chValue[5]);

  //Create file name
  char path[20];
  sprintf(path,"/setting%d.txt", slot);
  //Open file
  File settingFile = SPIFFS.open(path, "w");
  if (!settingFile) {
    BLYNK_LOG("Failed to open setting file\n");
    return false;
  }
  //Write JSON string to serial port and setting file
  serializeJson(doc, Serial); //Write to serial port
  Serial.println("");
  serializeJson(doc, settingFile); //Write to setting file
  settingFile.close();
}

boolean createSettingFile(int slot)
{
  const size_t capacity = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(5);
  DynamicJsonDocument doc(capacity);

  //Set settings to default
  LEDsettings[slot].enabled = false;
  LEDsettings[slot].settingTime = 0;
  LEDsettings[slot].settingFade = 1800;
  for(int i = 0; i < numCh; i = i + 1)
  {
    LEDsettings[slot].chValue[i] = 0;
  }
  
  doc["settingSlot"] = slot;
  doc["enabled"] = LEDsettings[slot].enabled;
  doc["time"] = LEDsettings[slot].settingTime;
  doc["fade"] = LEDsettings[slot].settingFade;
  JsonArray channels = doc.createNestedArray("channels");
  channels.add(LEDsettings[slot].chValue[0]);
  channels.add(LEDsettings[slot].chValue[1]);
  channels.add(LEDsettings[slot].chValue[2]);
  channels.add(LEDsettings[slot].chValue[3]);
  channels.add(LEDsettings[slot].chValue[4]);
  channels.add(LEDsettings[slot].chValue[5]);

  //Create file name
  char path[20];
  sprintf(path,"/setting%d.txt", slot);
  //Open file
  File settingFile = SPIFFS.open(path, "w");
  if (!settingFile) {
    BLYNK_LOG("Failed to open setting file\n");
    return false;
  }
  //Write JSON string to serial port and setting file
  serializeJson(doc, Serial); //Write to serial port
  Serial.println("");
  serializeJson(doc, settingFile); //Write to setting file
  settingFile.close();
}
