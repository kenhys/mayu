#include "MayuDriver.h"
#include "KeyboardMapperDelegater.h"

#define super    IOHIKeyboardMapper
OSDefineMetaClassAndStructors(KeyboardMapperDelegater, IOHIKeyboardMapper)

OSArray* KeyboardMapperDeleagaterManager::array_ = OSArray::withCapacity(4);
KeyboardMapperDelegater* KeyboardMapperDeleagaterManager::delegaterLast_;

void KeyboardMapperDeleagaterManager::replace(IOHIKeyboardMapper** map)
{
  KeyboardMapperDelegater* delegater = NULL;

  delegater = new KeyboardMapperDelegater;
  if (delegater)
  {
    if (!delegater->init())
    {
      delegater->release();
      return;
    }

    // mapのretain countを変化させないために、array_に入れてからreplace
    array_->setObject(delegater);
    delegater->release();
    delegater->replace(*map);
  }
}

void KeyboardMapperDeleagaterManager::remove(const KeyboardMapperDelegater* delegater)
{
  unsigned int index = array_->getNextIndexOfObject(delegater, 0);
  if (index != -1)
  {
    delegater->reset();
    array_->removeObject(index);
  }
}

void KeyboardMapperDeleagaterManager::restoreAll()
{
  for (unsigned int i = 0; i < array_->getCount(); ++i)
  {
    KeyboardMapperDelegater* delegater = OSDynamicCast(KeyboardMapperDelegater, array_->getObject(i));
    if (delegater)
    {
      if (!delegater->isTerminated())
        delegater->restore();
      delegater->reset();
    }
  }
  array_->flushCollection();
}

void KeyboardMapperDeleagaterManager::repairMapperAll()
{
  for (unsigned int i = 0; i < array_->getCount(); ++i)
  {
    KeyboardMapperDelegater* delegater = OSDynamicCast(KeyboardMapperDelegater, array_->getObject(i));
    if (delegater)
    {
      delegater->repairMapper();
    }
  }
}

void KeyboardMapperDeleagaterManager::retainLastDelegater(KeyboardMapperDelegater* delegater)
{
  delegaterLast_ = delegater;
}

void KeyboardMapperDeleagaterManager::sendKeyCode(UInt8 key, bool keyDown)
{
  if (delegaterLast_)
    delegaterLast_->orgTranslateKeyCode_And_RepairRepeat(key, keyDown);
}
