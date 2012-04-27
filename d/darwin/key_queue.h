// 2008/06/06

#pragma once

#include "common.h"
#include "driver.h"

// que size
const int KEY_QUE_MAX_SIZE = 64;

// property
unsigned int keyqueue_get_size();

// push
void keyqueue_push(const KEYBOARD_INPUT_DATA* data, unsigned int count);
void keyqueue_push_once(const KEYBOARD_INPUT_DATA data);

// pop
unsigned int keyqueue_pop(KEYBOARD_INPUT_DATA* buffer, unsigned int bufferSize);
KEYBOARD_INPUT_DATA keyqueue_pop_once();

// clear
void keyqueue_clear();
