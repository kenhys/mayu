// 2008/07/02 05:08:33

#pragma once

#define protected public
#define private public
#include <IOKit/hidsystem/IOHIKeyboard.h>
#if defined(PS2)
#include "ApplePS2Controller.h"
#endif
#include "IOHIDKeyboard.h"
#undef private
#undef protected
#include <IOKit/IOService.h>

class KEYBOARD_INPUT_DATA;

class IONotifier;
class IOService;

class MayuDriver : public IOService
{
  OSDeclareDefaultStructors(MayuDriver);
public:
  virtual bool start(IOService *provider);
  virtual void stop(IOService *provider);
	
  bool sendInputData(KEYBOARD_INPUT_DATA* keys, unsigned int count);
  bool grab(bool value);
protected:
  static bool matchedHandler(MayuDriver *self, void *ref, IOService* newService);
  static bool terminatedHandler(MayuDriver *self, void *ref, IOService *newService);

  bool mayuStart();
  void mayuStop();
protected:	
  IONotifier *matchedNotifier_;
  IONotifier *terminatedNotifier_;
};
