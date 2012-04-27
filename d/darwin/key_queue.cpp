#include <string.h>
#include "driver.h"
#include "key_queue.h"

#define min(a,b) ((a)<(b)?(a):(b))

static KEYBOARD_INPUT_DATA g_keyQueue[KEY_QUE_MAX_SIZE];
static unsigned int g_keyQueueSize = 0;
static unsigned int g_keyQueueUsedSize = 0;
static unsigned int g_keyQueueIndex = 0;

// property
unsigned int keyqueue_get_size()
{
  return g_keyQueueSize;
}

// clear
void keyqueue_clear()
{
  memset(g_keyQueue, sizeof(g_keyQueue), 0);
  g_keyQueueSize = 0;
  g_keyQueueUsedSize = 0;
  g_keyQueueIndex = 0;
}

// push
void keyqueue_push(const KEYBOARD_INPUT_DATA* data, unsigned int count)
{
  unsigned int i;
  for (i = 0; i < count; i++)
  {
    g_keyQueue[g_keyQueueIndex] = data[i];
    g_keyQueueIndex = (g_keyQueueIndex + 1) & (~KEY_QUE_MAX_SIZE);
  }
  g_keyQueueSize = min(g_keyQueueSize + count, KEY_QUE_MAX_SIZE);
}

void keyqueue_push_once(const KEYBOARD_INPUT_DATA data)
{
  keyqueue_push(&data, 1);
}

// pop
unsigned int keyqueue_pop(KEYBOARD_INPUT_DATA* buffer, unsigned int bufferSize)
{
  unsigned int i;
  unsigned int copy_count = min(bufferSize, g_keyQueueSize);
  for (i = 0; i < copy_count; i++)
  {
    // 後ろから取得していく
    g_keyQueueIndex = (g_keyQueueIndex == 0 ? KEY_QUE_MAX_SIZE - 1 : g_keyQueueIndex - 1);
    buffer[copy_count - i - 1] = g_keyQueue[g_keyQueueIndex];
  }
  g_keyQueueSize -= copy_count;
  return copy_count;
}

KEYBOARD_INPUT_DATA keyqueue_pop_once()
{
  KEYBOARD_INPUT_DATA ret;
  keyqueue_pop(&ret, 1);
  return ret;
}

