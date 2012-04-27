#pragma once

#include <linux/input.h>

#include "../../common.h"

// enum
typedef enum KEY_EVENT_VAL_e
{
  KEY_EVENT_BREAK,
  KEY_EVENT_PRESSE,
  KEY_EVENT_REPEAT
} KEY_EVENT_VAL;

extern int start_keyboard();
extern void stop_keyboard();

extern bool send_keyboard_event(int code, KEY_EVENT_VAL value);
extern int send_input_event(int type, int code, int value);
extern bool receive_keyboard_event(struct input_event* event);
extern void keyboard_grab_onoff(bool onoff);
