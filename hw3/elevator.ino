const int floors = 3;

const int floor_led[] = { 4, 3, 2 };

const int floor_btn[] = { A0, A1, A2 };
bool btn_state[] = { true, true, true };
unsigned long last_debounce[] = { 0, 0, 0 };

const int buzzer = 6;
const int press_freq = 440;
const int press_duration = 100;
const int cling_freq = 880;
const int cling_duration = 200;
const int move_freq = 220;

const int move_led = 7;
int move_led_state = true;

const unsigned long debounce_delay = 100;
const unsigned long door_delay = 1000;
const unsigned long move_delay = 1000;
const unsigned long blink_delay = 250;

enum ElevatorState
{
  READY = 0,
  CLOSING_DOORS,
  MOVING
} state;

int current_floor = 0;
int target_floor = 0;

unsigned long event_time = 0;
unsigned long blink_time = 0;

void setup()
{
  for(int i = 0; i < floors; i++)
  {
    pinMode(floor_led[i], OUTPUT);
    pinMode(floor_btn[i], INPUT_PULLUP);
  }

  pinMode(buzzer, OUTPUT);
  pinMode(move_led, OUTPUT);
}

bool button_pressed(int i)
{
  bool ret = false;

  if(millis() - last_debounce[i] > debounce_delay)
  {
    last_debounce[i] = millis();

    bool val = digitalRead(floor_btn[i]);

    ret = (btn_state[i] == true && val == false);

    btn_state[i] = val;
  }
  
  return ret;
}

void loop()
{
  switch(state)
  {
    case READY: 
      break;

    case CLOSING_DOORS:
    {
      if(millis() - event_time > door_delay)
      {
        event_time = millis();
        blink_time = event_time;
        state = MOVING;
        tone(buzzer, move_freq);
      }

      break;
    }

    case MOVING:
    {
      if(millis() - event_time > move_delay)
      {
        event_time = millis();

        if(current_floor == target_floor)
        {
          state = READY;
          tone(buzzer, cling_freq, cling_duration);
        }
        else
          current_floor += (current_floor < target_floor) ? 1 : -1;
      }

      if(millis() - blink_time > blink_delay)
      {
        blink_time = millis();
        move_led_state = !move_led_state;
      }

      break;
    }
  }

  for(int i = 0; i < floors; i++)
  {
    if(button_pressed(i) && state == READY && current_floor != i)
    {
      event_time = millis();
      target_floor = i;
      state = CLOSING_DOORS;
      tone(buzzer, press_freq, press_duration);

      break;
    }
  }

  for(int i = 0; i < floors; i++)
    digitalWrite(floor_led[i], i == current_floor);

  digitalWrite(move_led, state == MOVING && move_led_state);
}
