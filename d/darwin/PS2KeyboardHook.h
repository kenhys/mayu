// 2008/07/15
#pragma once

#define super    OSObject

// PS2に入力のあった最後のスキャンコードを知るためのHookクラス
class PS2KeyboardHook : public OSObject
{
  OSDeclareDefaultStructors(PS2KeyboardHook);
public:
  virtual void taggedRelease(const void *tag, const int when) const
  {
    if (orgInterruptTargetKeyboard_)
    {
      OSObject* orgObj = orgInterruptTargetKeyboard_;
      if (getRetainCount() <= when)
        stop();
      orgObj->taggedRelease(tag, when);
    }
    else
    {
      super::taggedRelease(tag, when);
    }
  }
	
  virtual void taggedRetain(const void *tag) const
  {
    if (orgInterruptTargetKeyboard_)
      orgInterruptTargetKeyboard_->taggedRetain(tag);
    else
      super::getRetainCount();
  }

  virtual int getRetainCount() const
  {
    if (orgInterruptTargetKeyboard_)
      return orgInterruptTargetKeyboard_->getRetainCount();
    else
      return super::getRetainCount();
  }

  virtual bool init()
  {
    PS2Controller_ = NULL;
    orgInterruptActionKeyboard_ = NULL;
    orgInterruptTargetKeyboard_ = NULL;
    lastScanCode_ = 0;
    scancodeRestSize_ = 0;

    return super::init();
  }

public:
  void start()
  {
    lastScanCode_ = 0;
    scancodeRestSize_ = 0;

    // ApplePS2Controllerを探す
    OSIterator* it;
    it = IOService::getMatchingServices(IOService::serviceMatching("ApplePS2KeyboardDevice"));
    if (it) {
      OSObject* machobj = it->getNextObject();
      if (machobj)
      {
        IOService* device = OSDynamicCast(IOService, machobj);
        if (device)
        {
          // PS2Controllerのスキャンコード通知にHook
          ApplePS2Controller* controller = OSDynamicCast(ApplePS2Controller, device->getProvider());
          if (controller)
          {
            // mayuが起動するたびにここを通るので、内容を保存するべきかチェックしてから保存
            if (controller->_interruptActionKeyboard != interruptPS2Keyboard)
              orgInterruptActionKeyboard_ = controller->_interruptActionKeyboard;
            if (controller->_interruptTargetKeyboard != this)
              orgInterruptTargetKeyboard_ = controller->_interruptTargetKeyboard;

            PS2Controller_ = controller;

            controller->_interruptActionKeyboard = (PS2InterruptAction)interruptPS2Keyboard;
            controller->_interruptTargetKeyboard = this;
          }
          else
          {
          }
        }
      }
      it->release();		// もう無いよね?
    }
    else
    {
    }
		
  }

  void stop() const
  {
    if (PS2Controller_)
    {
      // 何かもう別の物に置き換わっている場合は、戻さない
      if (PS2Controller_->_interruptActionKeyboard == interruptPS2Keyboard)
        PS2Controller_->_interruptActionKeyboard = orgInterruptActionKeyboard_;
      if (PS2Controller_->_interruptTargetKeyboard == this)
        PS2Controller_->_interruptTargetKeyboard = orgInterruptTargetKeyboard_;
      orgInterruptActionKeyboard_ = NULL;
      orgInterruptTargetKeyboard_ = NULL;
      PS2Controller_ = NULL;
    }
  }

  UInt32 getLastScanCode() const {return lastScanCode_;}
protected:
  // (PS2InterruptAction)
  static void interruptPS2Keyboard(void* target, UInt8 data)
  {
    PS2KeyboardHook* my = OSDynamicCast(PS2KeyboardHook, (OSObject*)target);
    if (my == NULL)
    {	// !?
      return;
    }
			
    if (my->scancodeRestSize_ == 0)
      my->lastScanCode_ = 0;
    else
      --my->scancodeRestSize_;
		
    // 今のところ大雑把なreplaceでいい (変換と無変換とカタカナしか対象にしていないので)
    switch (data)
    {
    case 0xE0:
      my->scancodeRestSize_ = 1;
      my->lastScanCode_ = data;
      break;
			
    case 0xE1:
      my->scancodeRestSize_ = 2;
      my->lastScanCode_ = data;
      break;
			
    default:
      my->lastScanCode_ = (my->lastScanCode_ << 8) | data;
    }
		
    my->orgInterruptActionKeyboard_(my->orgInterruptTargetKeyboard_, data);
  }

protected:
  // scan code用
  mutable UInt32 lastScanCode_;
  mutable int scancodeRestSize_;
  // replace用
  mutable ApplePS2Controller* PS2Controller_;
  mutable PS2InterruptAction orgInterruptActionKeyboard_;
  mutable OSObject* orgInterruptTargetKeyboard_;
};

#undef super
