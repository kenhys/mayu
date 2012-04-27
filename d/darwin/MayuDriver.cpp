// 2008/07/02 05:08:33

#include "MayuDriver.h"

#include "device.h"
#include "driver.h"
#if defined(PS2)
#include "PS2KeyboardHook.h"
#endif
#include "USBKeyboardHook.h"

#include "KeyboardMapperDelegater.h"

#define super    IOService
OSDefineMetaClassAndStructors(MayuDriver, IOService)

#if defined(PS2)
OSDefineMetaClassAndStructors(PS2KeyboardHook, OSObject)

PS2KeyboardHook* g_ps2hook = new PS2KeyboardHook;
IOHIKeyboard *g_ps2kbd;
#endif
USBKeyboardHook g_usbhook;

bool MayuDriver::start(IOService *provider)
{
  if (!super::start(provider))
  {
    return false;
  }

  // create special file
  if (create_device("mayu", this) != 0)
  {
    return false;
  }

  return true;
}

void MayuDriver::stop(IOService *provider)
{
  mayuStop();

#if defined(PS2)
  g_ps2hook->release();
#endif
  // destroy special file
  destroy_device();

  super::stop(provider);
}

bool MayuDriver::mayuStart()
{
  mayuStop();
	
#if defined(PS2)
  // hook ps2keyboard
  g_ps2hook->start();
#endif
	
  // hook usb keyboard
  g_usbhook.start();

  matchedNotifier_ = addNotification(gIOMatchedNotification, serviceMatching("IOHIKeyboard"),
                                     (IOServiceNotificationHandler)MayuDriver::matchedHandler, this, NULL, 0);
  if (matchedNotifier_ == NULL)
  {
    return false;
  }

  terminatedNotifier_ = addNotification(gIOTerminatedNotification, serviceMatching("IOHIKeyboard"),
                                        (IOServiceNotificationHandler)MayuDriver::terminatedHandler, this, NULL, 0);
  if (terminatedNotifier_ == NULL)
  {
    return false;
  }
		
  return true;
}

void MayuDriver::mayuStop()
{
  // restore keyboard
  KeyboardMapperDeleagaterManager::restoreAll();

  // restore usb keyboard map
  g_usbhook.restoreKeymapAll();	

  if (matchedNotifier_)
  {
    matchedNotifier_->remove();
  }
  if (terminatedNotifier_)
  {
    terminatedNotifier_->remove();
  }

  // unhook usbkeyboard
  g_usbhook.stop();
	
#if defined(PS2)
  // unhook ps2keyboard
  g_ps2hook->stop();
#endif
}

bool MayuDriver::matchedHandler(MayuDriver *self, void *ref, IOService *newService)
{
  IOHIKeyboard* keyboard = OSDynamicCast(IOHIKeyboard, newService);	
  if (keyboard)
  {
    const char *name = keyboard->getName();
    if (strcmp(name, "IOHIDKeyboard") == 0
#if defined(PS2)
        || strcmp(name, "ApplePS2Keyboard") == 0
#endif
        || strcmp(name, "AppleADBKeyboard") == 0 // 一応ADBキーボードも動くようにする
        )
    {
      KeyboardMapperDeleagaterManager::replace(&keyboard->_keyMap);
			
      if (strcmp(name, "IOHIDKeyboard") == 0)
      {
        g_usbhook.replaceKeymap(keyboard, false);
      }
#if defined(PS2)
      else if (strcmp(name, "ApplePS2Keyboard") == 0)
      {
        g_ps2kbd = keyboard;
      }
#endif
    }
    return true;
  }
  return false;
}

bool MayuDriver::terminatedHandler(MayuDriver *self, void *ref, IOService *newService)
{
  IOHIKeyboard *keyboard = OSDynamicCast(IOHIKeyboard, newService);

  if (keyboard)
  {
    KeyboardMapperDelegater* delegater = OSDynamicCast(KeyboardMapperDelegater, keyboard->_keyMap);
    if (delegater != NULL)
    {
      // この関数を抜けた後も、delegaterに対してreleaseが呼ばれるので、resetとremoveは行わない
      delegater->terminate();
      delegater->restore();
    }

    g_usbhook.restoreKeymap(keyboard);
  }
  return true;
}

bool MayuDriver::sendInputData(KEYBOARD_INPUT_DATA* keys, unsigned int count)
{
  for (unsigned int i = 0; i < count; ++i)
  {
    unsigned int key = keys[i].MakeCode;
    if (key > 0x80)			// 0x80より大きいキーコードは、Mayu内で適当に割り振ったキーコードなので元に戻す
    {
      key = 0x80;
    }

    KeyboardMapperDeleagaterManager::sendKeyCode(key, !(keys[i].Flags & KEYBOARD_INPUT_DATA::BREAK));
  }
  return true;
}

bool MayuDriver::grab(bool value)
{
  if (value)
  {
    return mayuStart();		
  }
  else
  {
    mayuStop();
    return true;
  }
}
