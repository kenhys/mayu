// 2008/05/01
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <string.h>
#include <miscfs/devfs/devfs.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include "key_queue.h"
#include "driver.h"
#include "MayuDriver.h"
#include "device.h"
#include <pexpert/pexpert.h>

#include <IOKit/IOLocks.h>
#include "../ioctl.h"

#define MAYU_MAJOR -1

static int g_major = MAYU_MAJOR;
static void* g_cdev;
static MayuDriver* g_driver;
static IOLock* g_select_event_lock; // select のイベント通知専用
static IOLock* g_device_lock;		// device lock
static bool g_grab = 0;

static int device_open(dev_t dev, int oflags, int devtype, struct proc *p);
static int device_close(dev_t dev, int flag, int fmt, struct proc *p);
static int device_read(dev_t dev, struct uio *uio, int ioflag);
static int device_write(dev_t dev, struct uio *uio, int ioflag);
static int device_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p);
static int device_select(dev_t dev, int which, void * wql, struct proc *p);

static struct cdevsw nmayu_cdevsw = {
  device_open,
  device_close,
  /* read */	device_read,
  /* write */	device_write,
  device_ioctl,
  eno_stop,
  eno_reset,
  0,
  device_select,
  eno_mmap,
  eno_strat,
  eno_getc,
  eno_putc,
  0
};

class LockObject
{
public:
  LockObject(IOLock* lock)
  {
    IOLockLock(lock);
    _lock = lock;
  }
	
  ~LockObject()
  {
    IOLockUnlock(_lock);
  }
private:
  IOLock* _lock;
};

bool is_grabbed()
{
  return g_grab;
}

// exprot fucntions 
int create_device(const char* device_name, const MayuDriver* driver)
{
  if (g_cdev != NULL)
    return EBUSY;

  g_driver = const_cast<MayuDriver*>(driver);

  g_select_event_lock = IOLockAlloc();
  g_device_lock = IOLockAlloc();

  g_grab = false;

  // register device
  int major = cdevsw_add(MAYU_MAJOR, &nmayu_cdevsw);
  if (major < 0)
  {
    IOLog("mayu:error cdevsw_add\n");
    return -1;
  }
  g_major = major;
	
  // make devicve file
  g_cdev = devfs_make_node(makedev(major, 0), DEVFS_CHAR,
                           UID_ROOT, GID_WHEEL,
                           0660 , device_name);
  if (g_cdev == NULL)
  {
    IOLog("mayu:error devfs_make_node\n");
    destroy_device();
    return -1;
  }

  return 0;  
}

void destroy_device()
{
  {
    LockObject lock(g_device_lock);
    g_grab = false;
  }
	
  if (g_cdev != NULL)
  {
    // remove device file
    devfs_remove(g_cdev);
    g_cdev = NULL;
  }

  // unregister device
  if (g_major >= 0)
  {
    int major = cdevsw_remove(g_major, &nmayu_cdevsw);
    if (major == -1)
    {
      IOLog("mayu:error cdevsw_remove");
    }
    else
    {
      g_major = MAYU_MAJOR;
    }
  }

  if (g_select_event_lock != NULL)
  {
    IOLockFree(g_select_event_lock);
    g_select_event_lock = NULL;
  }

  if (g_device_lock != NULL)
  {
    IOLockFree(g_device_lock);
    g_device_lock = NULL;
  }

  keyqueue_clear();

  g_driver = NULL;
}

// キーイベントの受信通知
void push_keyevent(const KEYBOARD_INPUT_DATA* data, unsigned int count)
{
  if (count == 0)
    return;					// こんなもん入れんなよ・・・

  {
    LockObject lock(g_device_lock);
    keyqueue_push(data, count);
  }
	
  IOLockWakeup(g_select_event_lock, NULL, true); // selectされて眠っているスレッドを起こす
}

// device file event
static int device_open(dev_t dev, int oflags, int devtype, struct proc *p)
{
  int error = 0;
  {
    LockObject lock(g_device_lock);
		
    if (g_grab)
      error = -1;	//多重openエラー
  }
  return error;
}

static int device_close(dev_t dev, int flag, int fmt, struct proc *p)
{
  int error = 0;
  {
    LockObject lock(g_device_lock);

    g_grab = false;
  }
  return error;
}

static int device_read(dev_t dev, struct uio *uio, int ioflag)
{
  int error = 0;
  KEYBOARD_INPUT_DATA buffer[KEY_QUE_MAX_SIZE];
  int bytes_remaining;
	
  {
    LockObject lock(g_device_lock);

    bytes_remaining = min(uio_resid(uio), keyqueue_get_size() * sizeof(KEYBOARD_INPUT_DATA));

    // pop key data
    keyqueue_pop(buffer, bytes_remaining/sizeof(KEYBOARD_INPUT_DATA));

    // trasmit
    error = uiomove((char*)buffer, bytes_remaining, uio);
    if (error != 0)
    {
      // ignore
    }
  }

  return 0;
}

static int device_write(dev_t dev, struct uio *uio, int ioflag)
{
  int error = 0;
  KEYBOARD_INPUT_DATA buffer[32];
  unsigned int count;
	
  {
    LockObject lock(g_device_lock);

    while (uio_resid(uio) > 0 && error == 0)
    {
      int bytes = min(uio_resid(uio), sizeof(buffer));

      error = uiomove((char*)buffer, bytes, uio);
      if (error != 0)
        break;

      count = bytes/sizeof(KEYBOARD_INPUT_DATA);
      if (count == 0)
        break;				// なんぞこれ

      g_driver->sendInputData(buffer, count);
    }
  }

  return error;
}

static int device_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
  int error = 0;

  switch (cmd)
  {
  case CTL_CODE_GRAB_ON:
    {
      LockObject lock(g_device_lock);
      g_grab = true;
      error = g_driver->grab(g_grab) ? 0 : -1;			
    }
    break;

  case CTL_CODE_GRAB_OFF:
    {
      LockObject lock(g_device_lock);
      g_grab = false;
      g_driver->grab(g_grab);
    }
    break;

  default:
    error = ENODEV;
  }
  return error;
}

static int device_select(dev_t dev, int which, void * wql, struct proc *p)
{
  switch (which)
  {
  case FREAD:
    if (keyqueue_get_size() == 0)
    {
      // queueに何か入ってくるまでイベント待ちを行う
      {
        LockObject lock(g_select_event_lock);
			
        if (keyqueue_get_size() == 0) // double checked
        {
          // ゆっくりしていってね！！
          IOLockSleep(g_select_event_lock, NULL, THREAD_ABORTSAFE);
        }
      }
    }
    break;
  case FWRITE:
    // WriteはいつでもOKなんだぜ?
    break;
  }		
  return 1;
}
