const int analog_max = 1023;
const int digital_max = 255;

const int r_out = 11;
const int g_out = 10;
const int b_out = 9;

const int r_in = A0;
const int g_in = A1;
const int b_in = A2;

void setup() 
{
  pinMode(r_out, OUTPUT);
  pinMode(g_out, OUTPUT);
  pinMode(b_out, OUTPUT);

  pinMode(r_in, INPUT);
  pinMode(g_in, INPUT);
  pinMode(b_in, INPUT);
}

void link(int in_pin, int out_pin)
{
  int in_value = analogRead(in_pin);
  int out_value = map(in_value, 0, analog_max, 0, digital_max);

  analogWrite(out_pin, out_value);
}

void loop() 
{
  link(r_in, r_out);
  link(b_in, b_out);
  link(g_in, g_out);
}
