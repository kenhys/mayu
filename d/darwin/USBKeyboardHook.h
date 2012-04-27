// 2008/07/16 22:07:43
#pragma once

const UInt8 MAYU_KEYCODE_HENKAN = 0x8a;
const UInt8 MAYU_KEYCODE_MUHENKAN = 0x8b;
const UInt8 MAYU_KEYCODE_KATAKANA = 0x88;

// 変換・無変換などの普通では取れないキーコードを、取れるキーコードに置き換える
class USBKeyboardHook
{
public:
  USBKeyboardHook()
  {
    USBKeyboardInfoArray_ = OSArray::withCapacity(4);
  }
	
  virtual ~USBKeyboardHook()
  {
    USBKeyboardInfoArray_->release();
  }

	
  bool start()
  {
    return USBKeyboardInfoArray_ != NULL;
  }

  void stop()
  {
    restoreKeymapAll();
    USBKeyboardInfoArray_->flushCollection();
  }

  void replaceKeymap(IOHIKeyboard* keyboard, bool showKeyCode)
  {
    if (!USBKeyboardInfoArray_)
      return;
		
    IOHIDKeyboard* usbkeyboard = OSDynamicCast(IOHIDKeyboard, keyboard);
    if (usbkeyboard)
    {
      // add
      USBKeyboardInfo temp;
      temp.keyboard_ = usbkeyboard;
      temp.orgKeymaps_[0] = usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_HENKAN];
      temp.orgKeymaps_[1] = usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_MUHENKAN];
      temp.orgKeymaps_[2] = usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_KATAKANA];

      OSData* data = OSData::withBytes(&temp, sizeof(temp));
      if (data)
      {
        USBKeyboardInfoArray_->setObject(data);
        data->release();
      }

      // replace

      // キーコードのIndexを調べる
      // 			{
      // 				for (int i = 0; i < 255; i++)
      // 				{
      // 					usbkeyboard->_usb_2_adb_keymap[i] = i;
      // 				}
      // 			}
      {
        usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_HENKAN] = MAYU_KEYCODE_HENKAN;
        usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_MUHENKAN] = MAYU_KEYCODE_MUHENKAN;
        usbkeyboard->_usb_2_adb_keymap[MAYU_KEYCODE_KATAKANA] = MAYU_KEYCODE_KATAKANA;
      }
    }
  }

  void restoreKeymapAll()
  {
    if (!USBKeyboardInfoArray_)
      return;

    for (unsigned int i = 0; i < USBKeyboardInfoArray_->getCount(); i++)
    {
      OSData* data = OSDynamicCast(OSData, USBKeyboardInfoArray_->getObject(i));
      if (data)
      {
        USBKeyboardInfo* keyboardInfo = (USBKeyboardInfo*)data->getBytesNoCopy();
				
        keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_HENKAN] = keyboardInfo->orgKeymaps_[0];
        keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_MUHENKAN] = keyboardInfo->orgKeymaps_[1];
        keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_KATAKANA] = keyboardInfo->orgKeymaps_[2];
      }
    }
  }

  void restoreKeymap(IOHIKeyboard* keyboard)
  {
    if (!USBKeyboardInfoArray_)
      return;

    for (unsigned int i = 0; i < USBKeyboardInfoArray_->getCount(); i++)
    {
      OSData* data = OSDynamicCast(OSData, USBKeyboardInfoArray_->getObject(i));
      if (data)
      {
        USBKeyboardInfo* keyboardInfo = (USBKeyboardInfo*)data->getBytesNoCopy();
        if (keyboardInfo->keyboard_ == keyboard)
        {
          keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_HENKAN] = keyboardInfo->orgKeymaps_[0];
          keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_MUHENKAN] = keyboardInfo->orgKeymaps_[1];
          keyboardInfo->keyboard_->_usb_2_adb_keymap[MAYU_KEYCODE_KATAKANA] = keyboardInfo->orgKeymaps_[2];
					
          USBKeyboardInfoArray_->removeObject(i);
          break;
					
        }
      }
    }
  }
	
protected:
  struct USBKeyboardInfo
  {
    IOHIDKeyboard* keyboard_;
    UInt8 orgKeymaps_[3]; // {変換, 無変換, カタカナひらがな}
  };
  OSArray* USBKeyboardInfoArray_;
};
