const uint8_t encodings[] = {
  0b11111100,
  0b01100000,
  0b11011010,
  0b11110010,
  0b01100110,
  0b10110110,
  0b10111110,
  0b11100000,
  0b11111110,
  0b11110110,
  0b00011100,
  0b11101110,
  0b11001110,
  0b01111100,
};

const int registerSize = 8;
const int dataPin = 10;
const int latchPin = 9;
const int clockPin = 8;

const int buttonPins[] = { A0, A1, A2 };
const int displayPins[] = { 2, 3, 4, 5 };

const int displayCount = 4;
const int buttonCount = 3;

enum Buttons
{
  START, RESET, LAP
};

enum State 
{
  PAUSED,
  COUNTING,
  LAP_VIEW
} state;

uint8_t displayState[displayCount];
bool lastButtonState[buttonCount] = { true, true, true };

uint32_t totalTime = 0;
uint32_t debounce = 100;
uint32_t lastPress[buttonCount];
uint32_t lastStart = 0;
uint32_t lastLap = 0;
uint32_t lapBlinkTime = 100;
uint32_t lapTitleTime = 500;

const int maxLaps = 9;
uint32_t laps[maxLaps];

int lapIndex = 0;
int lapViewIndex = 0;

void writeReg(uint8_t data)
{
  digitalWrite(latchPin, LOW);

  for(int i = 0; i < registerSize; i++)
  {
    digitalWrite(clockPin, LOW);
    digitalWrite(dataPin, (data >> i) & 1);
    digitalWrite(clockPin, HIGH);
  }

  digitalWrite(latchPin, HIGH);
}

bool buttonPressed(int n)
{
  bool buttonState = digitalRead(buttonPins[n]);

  if((millis() - lastPress[n] > debounce) && buttonState != lastButtonState[n])
  {
    lastPress[n] = millis();
    lastButtonState[n] = buttonState;

    return !buttonState;
  }

  return false;
}

void addLap(uint32_t time)
{
  if(lapIndex == maxLaps)
  { 
    for(int i = 0; i < (maxLaps - 1); i++)
      laps[i] = laps[i + 1];

    lapIndex = maxLaps - 1;
  }

  laps[lapIndex] = time;

  lapIndex++;
}

void printTime(uint32_t time)
{
  time /= 100;

  for(int i = (displayCount - 1); i >= 0; i--)
  {
    displayState[i] = encodings[time % 10];

    time /= 10;
  }

  displayState[2] |= 1;
}

void printLap(int n)
{
  displayState[0] = encodings[10];
  displayState[1] = encodings[11];
  displayState[2] = encodings[12];
  displayState[3] = encodings[n % 10];
}

inline void clear()
{
  *((uint32_t*)&displayState) = 0;
}

void setActiveDisplay(int n)
{
  for(int i = 0; i < displayCount; i++)
    digitalWrite(displayPins[i], i != n);
}

void draw()
{
  for(int i = 0; i < displayCount; i++)
  {
    setActiveDisplay(i);
    writeReg(displayState[i]);
    writeReg(0);
  }
}

void setup()
{
  for(int i = 0; i < buttonCount; i++)
    pinMode(buttonPins[i], INPUT_PULLUP);

  for(int i = 0; i < displayCount; i++)
  {
    pinMode(displayPins[i], OUTPUT);
    digitalWrite(displayPins[i], HIGH);
  }

  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  printTime(totalTime);
}

void loop()
{
  switch(state)
  {
    case PAUSED:
    {
      if(buttonPressed(START))
      {
        state = COUNTING;
        lastStart = millis();
      }
      else if(buttonPressed(RESET))
      {
        totalTime = 0;
        printTime(totalTime);
      }
      else if(buttonPressed(LAP))
      {
        if(lapIndex > 0)
        {
          state = LAP_VIEW;
          lapViewIndex = 0;

          lastLap = millis();
          printLap(lapViewIndex + 1);
        }
      }

      break;
    }

    case COUNTING:
    {
      uint32_t time = totalTime + (millis() - lastStart);

      if(buttonPressed(START))
      {
        state = PAUSED;
        totalTime = time;
      }
      else if(buttonPressed(LAP))
      {
        addLap(time);
        lastLap = millis();
        clear();
      }

      if(millis() - lastLap > lapBlinkTime)
        printTime(time);

      break;
    }

    case LAP_VIEW:
    {
      bool reset = false;

      if(buttonPressed(START) || (reset = buttonPressed(RESET)))
      {
        state = PAUSED;
        printTime(totalTime);

        if(reset)
          lapIndex = 0;

        break;
      }
      else if(buttonPressed(LAP))
      {
        lapViewIndex++;

        if(lapViewIndex == lapIndex)
          lapViewIndex = 0;

        lastLap = millis();
        printLap(lapViewIndex + 1);
      }

      if(millis() - lastLap > lapTitleTime)
        printTime(laps[lapViewIndex]);

      break;
    }
  }

  draw();
}
