#include <EEPROM.h>

#define MENU(name, data, parent)    ((Entry){name, true, data, parent})
#define OPTION(name, func, parent)  ((Entry){name, false, func, parent})
#define NULL_ENTRY                  ((Entry){nullptr, false, nullptr, nullptr})

struct Entry
{
  const char* name;
  bool isMenu;
  void* data;
  Entry* parent;
};

Entry mainMenu;
Entry mainEntries[5];
Entry settingsEntries[4];
Entry systemEntries[4];
Entry ledEntries[3];

Entry* currentMenu;
bool firstCall = false;
void (*handleInput)(const char*);

const char* sep = "------------------------------";
const char* toggleOn = "Toggle Automatic [ON]";
const char* toggleOff = "Toggle Automatic [OFF]";

unsigned long lastSample = 0;
bool printReadings;

struct
{
  int dataCount;
  int samplingInterval;

  int ultrasonicMin;
  int ultrasonicMax;

  int ldrMin;
  int ldrMax;

  unsigned char ledColor[3];
  bool automatic;
} globals;

const int bufferSize = 64;
char buffer[bufferSize];
int bufferIndex = 0;

const int trigPin = 3;
const int echoPin = 2;
const int ldrPin = A0;
const int ledPin[] = {9, 10, 11};

struct Data
{
  int ultrasonic;
  int ldr;
};

const int dataSize = 128;
const int dataEnd = dataSize * sizeof(Data);

int collectUltrasonicData()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  int distance = pulseIn(echoPin, HIGH);

  return distance * 0.34 / 2;
}

int collectLDRData()
{
  return analogRead(ldrPin);
}

void writeColor()
{
  for(int i = 0; i < 3; i++)
    analogWrite(ledPin[i], globals.ledColor[i]);
}

void writeColor(int r, int g, int b)
{
  analogWrite(ledPin[0], r);
  analogWrite(ledPin[1], g);
  analogWrite(ledPin[2], b);
}

void collectData()
{
  if (millis() - lastSample > globals.samplingInterval * 1000ul)
  {
    lastSample = millis();

    Data data = {collectUltrasonicData(), collectLDRData()};

    EEPROM.put((globals.dataCount % dataSize) * sizeof(Data), data);
    globals.dataCount++;

    EEPROM.put(dataEnd, globals);

    if (globals.automatic)
    {
      if (data.ultrasonic < globals.ultrasonicMin || data.ultrasonic > globals.ultrasonicMax || data.ldr < globals.ldrMin || data.ldr > globals.ldrMax)
        writeColor(255, 0, 0);
      else
        writeColor(0, 255, 0);
    }

    if (printReadings)
    {
      Serial.print("Ultrasonic: ");
      Serial.print(data.ultrasonic);
      Serial.print("    LDR: ");
      Serial.println(data.ldr);
    }
  }
}

bool getInput()
{
  if (bufferIndex < bufferSize - 1)
  {
    if (Serial.available() > 0)
    {
      char c = Serial.read();

      buffer[bufferIndex++] = c;

      if (c != '\n')
        return false;
    }
    else
      return false;
  }

  buffer[bufferIndex] = '\0';
  bufferIndex = 0;

  return true;
}

void printMenu(Entry* ent)
{
  if (!ent->isMenu)
    return;

  Serial.println(sep);
  Serial.print(ent->name);
  Serial.println(":");

  int i = 1;
  Entry* entry = ent->data;

  for(; entry->name; entry++, i++)
  {
    Serial.print(i);
    Serial.print(". ");
    Serial.println(entry->name);
  }

  if(ent->parent)
  {
    Serial.print(i);
    Serial.print(". ");
    Serial.println("Back");
  }
}

void enterMenu(Entry* menu = currentMenu)
{
  handleInput = defaultHandler;

  printMenu(menu);
  currentMenu = menu;
}

void enterOption(void* func)
{
  if (!func)
    return;

  handleInput = func;

  firstCall = true;
  handleInput(nullptr);
  firstCall = false;
}

void resetData(const char* input)
{
  if(firstCall)
  {
    Serial.println(sep);
    Serial.println("Are you sure?\n1. Yes\n2. No");

    return;
  }

  int opt = atoi(input);

  switch (opt)
  {
    case 1:
    {
      globals.dataCount = 0;

      EEPROM.put(dataEnd, globals);

      Serial.println(sep);
      Serial.println("Data has been reset.");
    }

    case 2:
      enterMenu();
  }
}

void setSamplingInterval(const char* input)
{
  if(firstCall)
  {
    Serial.println(sep);
    Serial.print("Set Sampling Interval:\n> ");
    
    return;
  }

  Serial.print(input);

  int value = atoi(input);

  if (value < 1 || value > 10)
    Serial.print("Value should be in range 1-10.\n> ");
  else
  {
    globals.samplingInterval = value;

    EEPROM.put(dataEnd, globals);

    enterMenu();
  }
}

void setUltrasonicThreshold(const char* input)
{
  if (firstCall)
  {
    Serial.println(sep);
    Serial.print("Set Ultrasonic Threshold:\n> ");

    return;
  }

  Serial.print(input);

  int min, max;

  int num = sscanf(input, "%d-%d", &min, &max);

  if (num != 2)
    Serial.print("Enter min-max values like 100-800\n> ");
  else
  {
    globals.ultrasonicMin = min;
    globals.ultrasonicMax = max;

    EEPROM.put(dataEnd, globals);

    enterMenu();
  }
}

void setLDRThreshold(const char* input)
{
  if (firstCall)
  {
    Serial.println(sep);
    Serial.print("Set LDR Threshold:\n> ");

    return;
  }

  Serial.print(input);

  int min, max;

  int num = sscanf(input, "%d-%d", &min, &max);

  if (num != 2)
    Serial.print("Enter min-max values like 100-800\n> ");
  else
  {
    globals.ldrMin = min;
    globals.ldrMax = max;

    EEPROM.put(dataEnd, globals);

    enterMenu();
  }
}

void showReadings(const char* input)
{
  if(firstCall)
  {
    Serial.println(sep);
    Serial.println("Send any message to stop.");

    printReadings = true;

    return;
  }

  printReadings = false;

  enterMenu();
}

void showSettings(const char* input)
{
  Serial.println(sep);
      
  Serial.print("Sampling Interval:     ");
  Serial.println(globals.samplingInterval);

  Serial.print("Ultrasonic Threshold:  ");
  Serial.print(globals.ultrasonicMin);
  Serial.print("-");
  Serial.println(globals.ultrasonicMax);

  Serial.print("LDR Threshold:         ");
  Serial.print(globals.ldrMin);
  Serial.print("-");
  Serial.println(globals.ldrMax);

  enterMenu();
}

void showData(const char* input)
{
  Serial.println(sep);

  Data data;
  int start = 0;

  if (globals.dataCount >= 10)
    start = globals.dataCount - 10;

  for(int i = start; i < globals.dataCount; i++)
  {
    EEPROM.get((i % dataSize) * sizeof(Data), data);
    
    Serial.print("Ultrasonic: ");
    Serial.print(data.ultrasonic);
    Serial.print("    LDR: ");
    Serial.println(data.ldr);
  }

  enterMenu();
}

void setColor(const char* input)
{
  if (firstCall)
  {
    Serial.println(sep);
    Serial.print("Set Color:\n> ");

    return;
  }

  Serial.print(input);

  int r, g, b;

  int num = sscanf(input, "%d %d %d", &r, &g, &b);

  if (num != 3)
    Serial.print("Enter RGB values like 0 255 0.\n> ");
  else
  {
    globals.ledColor[0] = r;
    globals.ledColor[1] = g;
    globals.ledColor[2] = b;

    EEPROM.put(dataEnd, globals);

    if(!globals.automatic)
      writeColor();

    enterMenu();
  }
}

void toggleAutomatic(const char* input)
{
  globals.automatic = !(globals.automatic & 1);

  EEPROM.put(dataEnd, globals);

  if (!globals.automatic)
    writeColor();

  ledEntries[1].name = globals.automatic ? toggleOn : toggleOff;

  enterMenu();
}

void defaultHandler(const char* input)
{
  int val = atoi(input);

  int i = 1;
  Entry* entry = currentMenu->data;

  for (; entry->name; entry++, i++)
  {
    if (i == val)
    {
      if (entry->isMenu)
        enterMenu(entry);
      else
        enterOption(entry->data);

      return;
    }
  }

  if(i == val && currentMenu->parent)
    enterMenu(currentMenu->parent);
}

void setup() 
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);

  for(int i = 0; i < 3; i++)
    pinMode(ledPin[i], OUTPUT);

  EEPROM.get(dataEnd, globals);

  if (!globals.automatic)
    writeColor();

  Serial.begin(9600);

  mainMenu = MENU("Main Menu", &mainEntries, nullptr);

  mainEntries[0] = MENU("Settings", &settingsEntries, &mainMenu);
  mainEntries[1] = OPTION("Reset Data", resetData, &mainMenu);
  mainEntries[2] = MENU("System Status", &systemEntries, &mainMenu);
  mainEntries[3] = MENU("LED Control", &ledEntries, &mainMenu);
  mainEntries[4] = NULL_ENTRY;

  settingsEntries[0] = OPTION("Set Sampling Interval", setSamplingInterval, &mainEntries[0]);
  settingsEntries[1] = OPTION("Set Ultrasonic Threshold", setUltrasonicThreshold, &mainEntries[0]);
  settingsEntries[2] = OPTION("Set LDR Threshold", setLDRThreshold, &mainEntries[0]);
  settingsEntries[3] = NULL_ENTRY;

  systemEntries[0] = OPTION("Show Readings", showReadings, &mainEntries[2]);
  systemEntries[1] = OPTION("Show Settings", showSettings, &mainEntries[2]);
  systemEntries[2] = OPTION("Show Data", showData, &mainEntries[2]);
  systemEntries[3] = NULL_ENTRY;

  ledEntries[0] = OPTION("Set Color", setColor, &mainEntries[3]);
  ledEntries[1] = OPTION(globals.automatic ? toggleOn : toggleOff, toggleAutomatic, &mainEntries[3]);
  ledEntries[2] = NULL_ENTRY;

  currentMenu = &mainMenu;
  
  enterMenu();
}

void loop() 
{
  collectData();

  if(getInput())
    handleInput(buffer);
}
