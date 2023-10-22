
const int r_led = 11;
const int g_led = 10;
const int b_led = 9;

const int r_pot = A0;
const int g_pot = A1;
const int b_pot = A2;

void setup() 
{
  pinMode(r_led, OUTPUT);
  pinMode(g_led, OUTPUT);
  pinMode(b_led, OUTPUT);

  pinMode(r_pot, INPUT);
  pinMode(g_pot, INPUT);
  pinMode(b_pot, INPUT);
}

void link(int led_pin, int pot_pin)
{
  int pot_value = analogRead(pot_pin);
  int led_value = map(pot_value, 0, 1023, 0, 255);

  analogWrite(led_pin, led_value);
}

void loop() 
{
  link(r_led, r_pot);
  link(b_led, b_pot);
  link(g_led, g_pot);
}
