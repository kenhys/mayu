#pragma once

#include "device.h"
#include "driver.h"
#if defined(PS2)
#include "PS2KeyboardHook.h"
#endif
#include "USBKeyboardHook.h"

class KeyboardMapperDelegater;

class KeyboardMapperDeleagaterManager
{
public:
  static void retainLastDelegater(KeyboardMapperDelegater* delegater);
  static void sendKeyCode(UInt8 key, bool keyDown);

  static void replace(IOHIKeyboardMapper** map);
  static void remove(const KeyboardMapperDelegater* delegater);
  static void restoreAll();
  static void repairMapperAll();
private:
  static OSArray* array_;
  static KeyboardMapperDelegater* delegaterLast_;
};

#if defined(PS2)
extern PS2KeyboardHook* g_ps2hook;
extern IOHIKeyboard *g_ps2kbd;
#endif
extern USBKeyboardHook g_usbhook;

#define super    IOHIKeyboardMapper
// replace Keyboard Mapper
class KeyboardMapperDelegater : public IOHIKeyboardMapper
{
  OSDeclareDefaultStructors(KeyboardMapperDelegater);
public:
  void replace(IOHIKeyboardMapper*& mapper) const
  {
    restore();
    orgKeymap_ = mapper;
    mapper = (KeyboardMapperDelegater*)this;
    keyboard_ = orgKeymap_->_delegate;
    terminated_ = false;
  }

  void restore() const
  {
    if (orgKeymap_)
    {
      if (orgKeymap_->_delegate->_keyMap == this)
      {
        orgKeymap_->_delegate->_keyMap = orgKeymap_;
      }
    }
  }
	
  void reset() const
  {
    orgKeymap_ = NULL;
    keyboard_ = NULL;
  }

  void terminate()
  {
    terminated_ = true;
  }

  bool isTerminated()
  {
    return terminated_;
  }

  // TODO:
  // 起動直後やログイン時に IOHIKeyboard の keyMap が変わるので、検知して再度replaceする必要がある。
  // ただし、keyMapが変わった事が検知できないこともありうるので、その場合は自分のタイミングでIOHIKeyboardの
  // keyMapをチェックする必要がある。
  // チェックする必要があるかもしれないと思っているタイミングは次の3つ
  // 		・mayuのデバイスファイルのオープン毎
  //		・MayuDriver::defaultKeymapOfLengthが呼ばれたとき
  //		・もしキーボード使用中などでも keyMap の変更される事がある場合は、
  // 		  IOHIKeyboard::_keyboardEventActionにでもHookして、キー入力毎にチェック
	
  // IOHIKeyboard の	keyMapperに変更がかかったか?
  bool checkMapper() const
  {
    return keyboard_ == NULL || keyboard_->_keyMap == this;
  }

  // IOHIKeyboard の keyMapが変わっていれば、切り替える
  void repairMapper() const
  {
    if (!checkMapper())
    {
      replace(keyboard_->_keyMap);
    }
  }

  void orgTranslateKeyCode_const(UInt8 key, bool keyDown) const
  {
    if (orgKeymap_)
    {
      orgKeymap_->translateKeyCode(key, keyDown, keyboard_->_keyState);
    }
  }
  void orgTranslateKeyCode_And_RepairRepeat(UInt8 key, bool keyDown)
  {
    bool org = keyboard_->_isRepeat;
    keyboard_->_isRepeat = true; // setRepeatを抑制

    bool before = keyboard_->_calloutPending;
    AbsoluteTime downRepeatTimeBefore = keyboard_->_downRepeatTime;
    unsigned codeToRepeatBefore = keyboard_->_codeToRepeat;
	
    // 入力に対応するsetRepeatはtranslateKeyCodeで行ない、生成されたものはrepeat不要
    orgTranslateKeyCode_const(key, keyDown);

    if (keyboard_->_keyboardEventAction)
    {
      // 矢印キーなどを入力すると、リピートが消えることがあるので再設定
      // 原因不明。やっつけ対応
      if (before && !keyboard_->_calloutPending)
      {
        keyboard_->_downRepeatTime = downRepeatTimeBefore;
        keyboard_->_codeToRepeat = codeToRepeatBefore;
        keyboard_->scheduleAutoRepeat();
      }
    }
	
    keyboard_->_isRepeat = org;
  }    
public:
  // implement: OSObject
  virtual bool init()
  {
    return OSObject::init();
  }
	
  virtual int getRetainCount() const
  {
    if (orgKeymap_)
      return orgKeymap_->getRetainCount();
    else
      return super::getRetainCount();
  }

  virtual void taggedRetain(const void *tag) const
  {
    if (orgKeymap_)
      orgKeymap_->taggedRetain(tag);
    else
      super::taggedRetain(tag);
  }

  // terminateの通知が来た後も、mapperに対して 1回 か 2回 release が呼ばれるので
  // retainCount が 0 になるか、unloadされるまではこのインスタンスは削除しない
  // (そのためキーボードを外してもunloadの時までarray_に残ることもある)
  virtual void taggedRelease(const void *tag, const int when) const
  {
    if (orgKeymap_)
    {
      if (orgKeymap_->getRetainCount() <= when)
      {
        IOHIKeyboardMapper* mapper = orgKeymap_;

        if (terminated_)
        {
          mapper->taggedRelease(tag, when);
          KeyboardMapperDeleagaterManager::remove(this);
        }
        else if (checkMapper())
        {
          restore();
          mapper->taggedRelease(tag, when);
          KeyboardMapperDeleagaterManager::remove(this);
        }
        else
        {
          // IOHIKeyboard の keyMap が変わっていれば、再度replaceする
          repairMapper();
          mapper->taggedRelease(tag, when);
        }
      }
      else
      {
        orgKeymap_->taggedRelease(tag, when);
        // IOHIKeyboard の keyMap が変わっていれば、再度replaceする
        if (!terminated_)
          repairMapper();
      }
    }
    else
    {
      super::taggedRelease(tag, when);
    }
  }

  // implement: IOHIKeyboardMapper
  virtual bool init(IOHIKeyboard * delegate,
                    const UInt8 *  mapping,
                    UInt32         mappingLength,
                    bool           mappingShouldBeFreed)
  {
    if (orgKeymap_)
      return orgKeymap_->init(delegate, mapping, mappingLength, mappingShouldBeFreed);
    else
      return true;
  }

  virtual void free()			// この関数をオーバーライドしておかないとうまく解放されない
  {
    OSObject::free();
  }
		
  virtual const UInt8* mapping()
  {
    return (orgKeymap_ == NULL) ? 0 : orgKeymap_->mapping();
  }
  virtual UInt32 mappingLength()
  {
    return (orgKeymap_ == NULL) ? 0 : orgKeymap_->mappingLength();
  }
  virtual bool serialize(OSSerialize *s) const
  {
    return (orgKeymap_ == NULL) ? false : orgKeymap_->serialize(s);
  }
	
  virtual void translateKeyCode(UInt8 key, bool keyDown, kbdBitVector keyBits)
  {
    //		IOLog("%s key=%d, keyDown=%d, keyBits=%d\n", __func__, key, keyDown, keyBits);
    if (orgKeymap_ != NULL)
    {
      if (is_grabbed())	// Mayu実行中か?
      {
#if defined(PS2)
        // PS2キーボードならキーコードをreplaceする
        if (orgKeymap_->_delegate == g_ps2kbd)
        {
          if (key == 0x80 && (0x7f & g_ps2hook->getLastScanCode()) == 0x79)	// [変換]
          {
            key = MAYU_KEYCODE_HENKAN;
          }
          else if (key == 0x80 && (0x7f & g_ps2hook->getLastScanCode()) == 0x7b)	// [無変換]
          {
            key = MAYU_KEYCODE_MUHENKAN;
          }
          else if (key == 0x37 && (0x7f & g_ps2hook->getLastScanCode()) == 0x70)	// [カタカナひらがな]
          {
            key = MAYU_KEYCODE_KATAKANA;
          }
        }
#endif

        KeyboardMapperDeleagaterManager::retainLastDelegater(this);

        // 初回のリピート設定
        if (!keyboard_->isRepeat())				// 初回
        {
          // setRepeatのためだけに実入力なしのtranslateKeyCode
          KeyboardEventAction action = keyboard_->_keyboardEventAction;
          keyboard_->_keyboardEventAction = NULL;
          orgTranslateKeyCode_const(key, keyDown);
          keyboard_->_keyboardEventAction = action;
        }
				
        KEYBOARD_INPUT_DATA data = {0};
        data.UnitId = 0;
        data.MakeCode = key;
        data.Flags = keyDown ? 0 : KEYBOARD_INPUT_DATA::BREAK;

        push_keyevent(&data, 1);
				
      }
      else
      {
        orgTranslateKeyCode_const(key, keyDown);
      }
    }
  }
	
  virtual UInt8  		getParsedSpecialKey(UInt8 logical)
  {
    return (orgKeymap_ == NULL) ? 0 : orgKeymap_->getParsedSpecialKey(logical);
  }

  virtual	void		setKeyboardTarget (IOService * keyboardTarget)
  {
    if (orgKeymap_ != NULL)
      orgKeymap_->setKeyboardTarget(keyboardTarget);
  }
	
  virtual bool 	    updateProperties (void)
  {
    return (orgKeymap_ == NULL) ? false : orgKeymap_->updateProperties();
  }
	
  virtual IOReturn  	setParamProperties (OSDictionary * dict)
  {
    return (orgKeymap_ == NULL) ? kIOReturnError : orgKeymap_->setParamProperties(dict);
  }
	
  virtual void 	    keyEventPostProcess (void)
  {
    if (orgKeymap_ != NULL)
      orgKeymap_->keyEventPostProcess();
  }
	
  virtual IOReturn message( UInt32 type, IOService * provider, void * argument = 0 )
  {
    return (orgKeymap_ == NULL) ? kIOReturnError : orgKeymap_->message(type, provider, argument);
  }

protected:
  mutable IOHIKeyboardMapper* orgKeymap_;
  mutable IOHIKeyboard* keyboard_;
  mutable bool terminated_;
};

#undef super
