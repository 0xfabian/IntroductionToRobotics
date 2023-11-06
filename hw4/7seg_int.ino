enum Segments
{
  A = 0, B, C, D, E, F, G, DP, SEG_SIZE  
};

int transition[SEG_SIZE][4] =
{
  { A, G, F, B },
  { A, G, F, B },
  { G, D, E, DP },
  { G, D, E, C },
  { G, D, E, C },
  { A, G, F, B },
  { A, D, G, G },
  { DP, DP, C, DP }
};

void handlePress();

struct Joystick
{
  int vrx;
  int vry;
  int sw;

  int x;
  int y;
  bool btn;

  Joystick(int _vrx, int _vry, int _sw): vrx(_vrx), vry(_vry), sw(_sw) {}

  init()
  {
    pinMode(vrx, INPUT);
    pinMode(vry, INPUT);
    pinMode(sw, INPUT_PULLUP);

    x = analogRead(vrx);
    y = analogRead(vry);
    btn = false;

    attachInterrupt(digitalPinToInterrupt(sw), handlePress, CHANGE);
  }
};

int pins[SEG_SIZE] = { 6, 7, 8, 4, 5, 10, 3, 9 };

Joystick joystick(A0, A1, 2);
unsigned long lastPress = 0;
unsigned long debounceTime = 200000;
unsigned long timeToReset = 2000000;
int midPoint = 512;
int safeZone = 200;

bool blinkState = false;
int blinkDelay = 400;
unsigned long lastBlink = 0;

unsigned char state;
int pos;

void reset()
{
  state = 0;
  pos = DP;
}

void move()
{
  int new_x = analogRead(joystick.vrx);
  int new_y = analogRead(joystick.vry);

  if(abs(midPoint - joystick.x) < safeZone)
  {
    if(new_x < midPoint - safeZone)
      pos = transition[pos][2];
    else if(new_x > midPoint + safeZone)
      pos = transition[pos][3];
  }

  if(abs(midPoint - joystick.y) < safeZone)
  {
    if(new_y < midPoint - safeZone)
      pos = transition[pos][0];
    else if(new_y > midPoint + safeZone)
      pos = transition[pos][1];
  }

  joystick.x = new_x;
  joystick.y = new_y;
}

void handlePress()
{
  if(micros() - lastPress > debounceTime)
  {
    if(digitalRead(joystick.sw) == LOW)
      joystick.btn = true;

    lastPress = micros();
  }
}

void press()
{
  if(joystick.btn)
  {
    state ^= 1 << pos;
    joystick.btn = false;
  }

  if((micros() - lastPress > timeToReset) && digitalRead(joystick.sw) == LOW)
    reset();
}

void blink()
{
  if(millis() - lastBlink > blinkDelay)
  {
    lastBlink = millis();
    blinkState = !blinkState;
  }
}

void draw()
{
  for(int i = 0; i < SEG_SIZE; i++)
    digitalWrite(pins[i], (i == pos) ? blinkState : ((state >> i) & 1));
}

void setup()
{
  for(int i = 0; i < SEG_SIZE; i++)
    pinMode(pins[i], OUTPUT);

  joystick.init();

  reset();
}

void loop()
{
  move();
  press();
  blink();
  draw();
}
