#include "LedControl.h"

const int din_pin = 12;
const int clock_pin = 11;
const int load_pin = 10;
const int vrx = A3;
const int vry = A4;
const int sw = A5;

const int matrix_size = 8;
const int level_size = 15;

enum Tile
{
  NONE,
  HARD_WALL,
  SOFT_WALL,
  FIRE
};

Tile level[level_size][level_size];

bool game_over;

uint8_t player_x;
uint8_t player_y;
uint8_t bomb_x;
uint8_t bomb_y;

bool player_blink;
bool bomb_blink;
bool fire_blink;
bool bomb_down;

unsigned long last_player_update;
unsigned long last_player_blink;
unsigned long last_bomb_blink;
unsigned long last_fire_blink;
unsigned long last_place_time;

const unsigned long player_update_time = 100;
const unsigned long player_blink_time = 50;
const unsigned long bomb_blink_time = 200;
const unsigned long fire_blink_time = 100;
const unsigned long fire_time = 500;
const unsigned long fuse_time = 3000;

const int mid_point = 512;
const int threshold = 200;

const uint8_t game_over_message[] = 
{
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0x3c, 0x42, 0x42, 0x52, 0x52, 0x34, 0,
  0x20, 0x54, 0x54, 0x54, 0x78, 0,
  0x7c, 0x04, 0x7c, 0x04, 0x78, 0,
  0x38, 0x54, 0x54, 0x54, 0x58, 0, 0, 0,
  0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0,
  0x1c, 0x20, 0x40, 0x20, 0x1c, 0,
  0x38, 0x54, 0x54, 0x54, 0x58, 0,
  0x7c, 0x08, 0x04, 0x04, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

LedControl lc(din_pin, clock_pin, load_pin, 1);

void generate_level()
{
  for(int y = 0; y < level_size; y++)
    for(int x = 0; x < level_size; x++)
    {
      bool on_edge = (x == 0) || (x == level_size - 1) || (y == 0) || (y == level_size - 1);
      bool is_even = (x % 2 == 0) && (y % 2 == 0);

      if (on_edge)
        level[y][x] = HARD_WALL;
      else
        level[y][x] = (random(3) > 1) ? SOFT_WALL : NONE;
    }

  if (level[player_y][player_x] == SOFT_WALL)       level[player_y][player_x] = NONE;
  if (level[player_y + 1][player_x] == SOFT_WALL)   level[player_y + 1][player_x] = NONE;
  if (level[player_y - 1][player_x] == SOFT_WALL)   level[player_y - 1][player_x] = NONE;
  if (level[player_y][player_x + 1] == SOFT_WALL)   level[player_y][player_x + 1] = NONE;
  if (level[player_y][player_x - 1] == SOFT_WALL)   level[player_y][player_x - 1] = NONE;
}

int get_joystick_input(int pin)
{
  int read = analogRead(pin) - mid_point;

  if (read > threshold)
    return 1;

  if (read < -threshold)
    return -1;

  return 0;
}

void player_update()
{
  if (millis() - last_player_update > player_update_time)
  {
    last_player_update = millis();

    int off_x = get_joystick_input(vry);
    int off_y = -get_joystick_input(vrx);

    if (level[player_y][player_x + off_x] == NONE)
      player_x += off_x;

    if (level[player_y + off_y][player_x] == NONE)
      player_y += off_y;

    if (level[player_y][player_x] == FIRE)
    {
      game_over = true;
      return;
    }
  }

  if(!bomb_down && digitalRead(sw) == LOW)
  {
    last_place_time = millis();

    bomb_down = true;
    bomb_x = player_x;
    bomb_y = player_y;
  }
}

void bomb_update()
{
  if(bomb_down && (millis() - last_place_time > fuse_time))
  {
    bomb_down = false;
    
    level[bomb_y][bomb_x] = FIRE;

    if (level[bomb_y + 1][bomb_x] != HARD_WALL)     level[bomb_y + 1][bomb_x] = FIRE;
    if (level[bomb_y - 1][bomb_x] != HARD_WALL)     level[bomb_y - 1][bomb_x] = FIRE;
    if (level[bomb_y][bomb_x + 1] != HARD_WALL)     level[bomb_y][bomb_x + 1] = FIRE;
    if (level[bomb_y][bomb_x - 1] != HARD_WALL)     level[bomb_y][bomb_x - 1] = FIRE;
  }
}

void clear_fire()
{
  if (millis() - last_place_time > fuse_time + fire_time)
    for (int y = 0; y < level_size; y++)
      for (int x = 0; x < level_size; x++)
        if (level[y][x] == FIRE)
          level[y][x] = NONE;
}

void blink(unsigned long& last_time, unsigned long blink_time, bool& blink)
{
  if (millis() - last_time > blink_time)
  {
    last_time = millis();
    blink = !blink;
  }
}

void reset()
{
  game_over = false;

  player_x = 1;
  player_y = 1;

  last_player_update = 0;
  last_player_blink = 0;
  last_bomb_blink = 0;
  last_fire_blink = 0;
  last_place_time = 0;

  generate_level();

  for(int y = 0; y < matrix_size; y++)
    for(int x = 0; x < matrix_size; x++)
    {
      lc.setLed(0, y, x, level[y][x]);
      delay(10);
    }
}

void do_game_over_effect()
{
  for (int y = 0; y < matrix_size; y++)
    for (int x = 0; x < matrix_size; x++)
    {
      lc.setLed(0, y, x, false);
      delay(10);
    }

  for (int i = 0; i < sizeof(game_over_message) - matrix_size; i++)
  {
    for (int x = i; x < i + matrix_size; x++)
      for (int y = 0; y < matrix_size; y++)
        lc.setLed(0, y, x - i, (game_over_message[x] >> y) & 1);

    delay(50);
  }
}

void draw()
{
  blink(last_player_blink, player_blink_time, player_blink);
  blink(last_bomb_blink, bomb_blink_time, bomb_blink);
  blink(last_fire_blink, fire_blink_time, fire_blink);

  int draw_off_x = player_x - matrix_size / 2;
  int draw_off_y = player_y - matrix_size / 2;
  int diff = level_size - matrix_size;

  if (draw_off_x < 0)
    draw_off_x = 0;
  else if (draw_off_x > diff)
    draw_off_x = diff;

  if (draw_off_y < 0)
    draw_off_y = 0;
  else if (draw_off_y > diff)
    draw_off_y = diff;

  for(int y = 0; y < matrix_size; y++)
    for(int x = 0; x < matrix_size; x++)
    {
      uint8_t lx = x + draw_off_x;
      uint8_t ly = y + draw_off_y;

      if (lx == player_x && ly == player_y)
        lc.setLed(0, y, x, player_blink);
      else if(bomb_down && lx == bomb_x && ly == bomb_y)
        lc.setLed(0, y, x, bomb_blink);
      else
        lc.setLed(0, y, x, (level[ly][lx] == FIRE) ? fire_blink : level[ly][lx]);
    }
}

void setup() 
{
  Serial.begin(9600);

  pinMode(din_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  pinMode(load_pin, OUTPUT);

  pinMode(vrx, INPUT);
  pinMode(vry, INPUT);
  pinMode(sw, INPUT_PULLUP);

  randomSeed(analogRead(A0));

  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  reset();
}

void loop() 
{
  if (!game_over)
  {
    player_update();
    bomb_update();
    clear_fire();
    draw();
  }
  else
  {
    do_game_over_effect();
    reset();
  }
}
