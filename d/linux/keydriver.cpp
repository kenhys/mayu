// userモードで動作するキーボードドライバ
// 2008/01/31
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include <libusb-1.0/libusb.h>
#include <libudev.h>

#include <sys/epoll.h>
#include <sys/select.h>
#include <inttypes.h>
#include <asm/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <string>
using namespace std;

#include "keydriver.h"
#include "cpu_dependency.h"

// global
static int g_epoll_fd = -1;
static int g_uinput_fd = -1;

#define EVDEV_MINORS	32
static int g_envdev_key_fds[EVDEV_MINORS];
static int g_envdev_key_fds_count = 0;

static bool g_grab_onoff = false;

static udev* g_udev;
static udev_monitor* g_udev_mon;
static int g_udev_fd = -1;
static libusb_context* g_usb_context;

static bool add_fd_to_epoll(int fd)
{
  if (g_epoll_fd < 0)
    return false;
  
  epoll_event e = {0};
  e.events = EPOLLIN;
  e.data.fd = fd;
  return epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, fd, &e) == 0;
}

static void delete_fd_from_epoll(int fd)
{
  if (g_epoll_fd >= 0)
  {
    epoll_event e = {0};
    e.data.fd = fd;
    epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, fd, &e);
  }
}

static bool is_keyboard_device(int fd)
{
  uint8_t evtype_bitmask[EV_MAX/8 + 1];
  struct input_id devinfo;
	
  // check bustype
  if (ioctl(fd, EVIOCGID, &devinfo) != 0)
  {
    //なんぞ
    fprintf(stderr, "error: EVIOCGID ioctl failed.\n");
    return false;
  }

  // allow USB, PS/2, ADB
  switch (devinfo.bustype)
  {
  case BUS_USB:
  case BUS_I8042:
  case BUS_ADB:
    break;					// ok
  default:
    return false;			// unmatch bus type
  }

  // check ev_bit
  if (ioctl(fd, EVIOCGBIT(0, sizeof(evtype_bitmask)), evtype_bitmask) > 0)
  {
    // EV_SYN, EV_KEY, EV_REP ならおｋ
    if (test_bit(EV_SYN, evtype_bitmask) && test_bit(EV_KEY, evtype_bitmask) && test_bit(EV_REP, evtype_bitmask))
    {				
      return true;
    }
  }

  return false;
}

static int open_event_device(int dev_num)
{
  char path[128];
  int fd, version;
  unsigned i;
  const char* endev_path[] = {"/dev/input/event%d", "/dev/event%d"};

  for (i = 0; i < array_size(endev_path); i++)
  {
    sprintf(path, endev_path[i], dev_num);
    fd = open(path, O_RDONLY | O_NDELAY);
    if (fd >= 0)
      break;
  }	
  if (fd == -1)
  {
    return -1;
  }

  if (ioctl(fd, EVIOCGVERSION, &version) == -1)
  {
    fprintf(stderr, "error: EVIOCGVERSION ioctl failed\n");
    close(fd);
    return -1;
  }

  //evdevのデバイスで無いなら取り扱わない
  if (version != EV_VERSION)
  {
    close(fd);
    return -1;
  }
  return fd;
}

// オープンしたデバイスの個数を返す
static int open_keyboard()
{
  int i, fd;
	
  for (i = 0;i < EVDEV_MINORS; i++)
  {
    fd = open_event_device(i);
    if (fd != -1)
    {
      if (is_keyboard_device(fd))
      {
        g_envdev_key_fds[g_envdev_key_fds_count++] = fd;
        add_fd_to_epoll(fd);
      }
      else
      {
        close(fd);
      }
    }
  }
  return g_envdev_key_fds_count;
}

static void close_keyboard()
{
  int i;
  for (i = 0;i < g_envdev_key_fds_count; i++)
  {
    close(g_envdev_key_fds[i]);
    delete_fd_from_epoll(g_envdev_key_fds[i]);
    g_envdev_key_fds[i] = 0;
  }
  g_envdev_key_fds_count = 0;
}

// キーコード出力用のキーボードを破棄
static void destroy_uinput_keyboard()
{
  if (g_uinput_fd != -1)
  {
    ioctl(g_uinput_fd, UI_DEV_DESTROY);
    close(g_uinput_fd);
    g_uinput_fd = -1;
  }
}

// キーコード出力用のキーボードを作成
static int create_uinput_keyboard()
{
  struct uinput_user_dev uinp = {{0}};
  unsigned i = 0;
  int ret;
  int err = ENOENT;
  const char* uinput_path[] = {"/dev/input/uinput", "/dev/uinput"};

  destroy_uinput_keyboard();
  // input deviceを開く
  for (i = 0; i < array_size(uinput_path); i++)
  {
    g_uinput_fd = open(uinput_path[i], O_RDWR);
    if (g_uinput_fd >= 0)
      break;
    else if (errno != ENOENT)
    {
      err = errno;
    }
  }

  if (g_uinput_fd <= 0)
  {
    if (err == ENOENT)
    {
      fprintf(stderr, "error: The uinput not found. Did you load a uinput module?\n");
    }
    else
    {
      fprintf(stderr, "error: Failed to open a uinput. '%s'\n", strerror(err));
    }
    return -1;
  }
	
  strncpy(uinp.name, "mayu uinpt", UINPUT_MAX_NAME_SIZE);
  uinp.id.vendor = 1;
  uinp.id.bustype = BUS_I8042;
  //	uinp.id.bustype = BUS_USB;
  uinp.id.product = 1;
  uinp.id.version = 4;
  // uinput deviceを作成
  ret = ioctl(g_uinput_fd, UI_SET_EVBIT, EV_KEY);
  if(ioctl(g_uinput_fd, UI_SET_EVBIT, EV_SYN) == -1)fprintf(stderr, "%d:ioctl\n", __LINE__);
  ioctl(g_uinput_fd, UI_SET_EVBIT, EV_REP);
  ioctl(g_uinput_fd, UI_SET_EVBIT, EV_REL);
  ioctl(g_uinput_fd, UI_SET_RELBIT, REL_X);
  ioctl(g_uinput_fd, UI_SET_RELBIT, REL_Y);
  for (i=0; i < KEY_MAX; i++) {
    ioctl(g_uinput_fd, UI_SET_KEYBIT, i);
  }
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_MOUSE);
  //	ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_TOUCH);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_MOUSE);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_FORWARD);
  ioctl(g_uinput_fd, UI_SET_KEYBIT, BTN_BACK);
	
  write(g_uinput_fd, &uinp, sizeof(uinp));

  ret = ioctl(g_uinput_fd, UI_DEV_CREATE);
  if (ret)
  {
    fprintf(stderr, "error: Failed to open a uinput.\n");
    return -1;
  }
  return 0;
}

// udev/usb
static bool open_udev_usb_monitors(udev*& udev, udev_monitor*& mon, int& fd)
{
  // udev
  udev = udev_new();
  if (udev == NULL)
    return false;

  mon = udev_monitor_new_from_netlink(udev, "udev");
  if (mon == NULL)
  {
    udev_unref(udev);
    udev = NULL;
    return false;
  }
        
  udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", "usb_device");
  udev_monitor_enable_receiving(mon);
  fd = udev_monitor_get_fd(mon);

  // usb
  libusb_init(&g_usb_context);
  return true;
}

static void close_udev_usb_monitors(udev*& udev, udev_monitor*& mon, int& fd)
{
  // udev
  if (mon)
  {
    udev_monitor_unref(mon);
    mon = NULL;
  }
  if (udev)
  {
    udev_unref(udev);
    udev = NULL;
  }
  // closeの必要はあるのか?
  fd = -1;

  // usb
  libusb_exit(g_usb_context);
}

static int get_udev_usb_vid_pid(udev_monitor* mon, int& vid, int& pid)
{
  udev_device* dev = udev_monitor_receive_device(mon);
  int result = 0;
  if (dev)
  {
    string action = udev_device_get_action(dev);
    if (action == "add" || action == "remove")
    {
      const char* propvalue = udev_device_get_property_value(dev, "ID_VENDOR_ID");
      char* endp;
      if (propvalue)
      {
        vid = ::strtol(propvalue, &endp, 16);
        if (propvalue != endp)
        {
          propvalue = udev_device_get_property_value(dev, "ID_MODEL_ID");
          if (propvalue)
          {
            pid = ::strtol(propvalue, &endp, 16);
            if (propvalue != endp)
            {
              result = 1;
            }
          }
        }
      }
    }
  }
  else
  {
    return -1;
  }
  udev_device_unref(dev);
  return result;
}

static bool is_divice_keyboard_with_vid_pid(libusb_context *usbctx, int vid, int pid)
{
  bool result = false;
  libusb_device_handle* handle = libusb_open_device_with_vid_pid(usbctx, vid, pid);
  if (handle)
  {
    libusb_device* dev = libusb_get_device(handle);
    libusb_device_descriptor devicedesc;
    if (libusb_get_device_descriptor(dev, &devicedesc) == 0)
    {
      if (devicedesc.bDeviceClass == 0) // HIDは0
      {
        bool done = false;
        for (unsigned char confIndex = 0; confIndex < devicedesc.bNumConfigurations && !done; confIndex++)
        {
          libusb_config_descriptor* config;
          if (libusb_get_config_descriptor(dev, confIndex, &config) == 0)
          {
            for (int ifIndex = 0; ifIndex < config->bNumInterfaces && !done; ifIndex++)
            {
              const libusb_interface& interface = config->interface[ifIndex];

              for (int alterIndex = 0; alterIndex < interface.num_altsetting; alterIndex++)
              {
                const libusb_interface_descriptor& ifdescriptor = interface.altsetting[alterIndex];
                if (ifdescriptor.bInterfaceClass == 0x03 // HID
                    && ifdescriptor.bInterfaceProtocol == 0x01) // keyboard
                {
                  result = true;
                  done = true;
                }
              }
            }
            libusb_free_config_descriptor(config);
          }
        }
      }
    }
    libusb_close(handle);
  }
  return result;
}

static void close_keyboard_and_uinput()
{
  keyboard_grab_onoff(false);
  destroy_uinput_keyboard();
  close_keyboard();
}

// 失敗したならエラーコード、成功なら0を返す
static int open_keyboard_and_uinput()
{
  int ret = open_keyboard(); // オープンに成功したデバイスの個数が返る
  if (ret <= 0)
  {
    fprintf(stderr, "error: Cannot open any keyboard event devices. Do you run as root?\n");
    return -1;
  }
	
  ret = create_uinput_keyboard(); // 成功したら0が返る
  if (ret < 0)
  {
    close_keyboard_and_uinput();
    return -1;
  }
  return 0;
}

int start_keyboard()
{
  int ret = 0;
  g_epoll_fd = epoll_create(EVDEV_MINORS);
  if (g_epoll_fd < 0)
    ret = -1;

  if (ret == 0)
  {
    ret = open_keyboard_and_uinput();
  }

  if (ret == 0)
  {
    // udev
    if (open_udev_usb_monitors(g_udev, g_udev_mon, g_udev_fd))
    {
      add_fd_to_epoll(g_udev_fd);
    }
  }
  return ret;
}

void stop_keyboard()
{
  close_keyboard_and_uinput();
  // udev
  if (g_udev)
  {
    close_udev_usb_monitors(g_udev, g_udev_mon, g_udev_fd);
  }

  // epoll
  if (g_epoll_fd > 0)
  {
    close(g_epoll_fd);
    g_epoll_fd = -1;
  }
}

static int restart_keyboard()
{
  bool grabed = g_grab_onoff;
  close_keyboard_and_uinput();
  int ret = open_keyboard_and_uinput();
  if (ret == 0)
  {
    if (grabed)
    {
      keyboard_grab_onoff(grabed);
    }
  }
  return ret;
}

//成功なら0を返す
int send_input_event(int type, int code, int value)
{
  int result;
  input_event event = {{0}};

  if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
    return -1;
	
  gettimeofday(&event.time, NULL);
  event.type = type;
  event.code = code;
  event.value = value;
	
  result = write(g_uinput_fd, &event, sizeof(event));
  return (result < 0 ? errno :
          (result < (int)sizeof(event) ? -1 : 0));
}

bool send_keyboard_event(int code, KEY_EVENT_VAL value)
{
  if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
    return false;
	
  if (send_input_event(EV_KEY, code, value) < 0)
    return false;

  send_input_event(EV_SYN, SYN_REPORT, 0); // エラーが発生しても無視
	
  return true;
}

// error code
bool receive_keyboard_event(struct input_event* event)
{
  epoll_event epoll_e[EVDEV_MINORS + 1];
  struct input_event revent = {{0}};
  bool try_to_repair = false;	
  bool loop_stop = false;

  if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
    return false;
	
  while (!loop_stop)
  {
    int num_of_events = epoll_wait(g_epoll_fd, epoll_e, EVDEV_MINORS + 1, -1);

    for (int i = 0; i < num_of_events; i++)
    {
      int event_fd = epoll_e[i].data.fd;
      // udevをチェック
      if (event_fd == g_udev_fd)
      {
        int vid = 0, pid = 0;
        if (get_udev_usb_vid_pid(g_udev_mon, vid, pid) > 0)
        {
          if (is_divice_keyboard_with_vid_pid(g_usb_context, vid, pid))
          {
            // キーボードデバイスを開き直す
            restart_keyboard();
            // 最初から
            if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
            {
              return false;
            }
          }
        }
      }
      else
      {
        // データが書き込まれたデバイスから読み込む
        if (read(event_fd, &revent, sizeof(revent)) < (int)sizeof(revent))
        {          
          // キーボードが外された時に、ENODEVのエラーになる
          if (errno == ENODEV)
          {
            if (try_to_repair)
            {
              // 開きなおして読み直しでも駄目だった
              return false;
            }
            else
            {
              // 状態修復
              try_to_repair = true;
              // キーボードデバイスを開き直す
              restart_keyboard();
              // 最初から
              if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
              {
                return false;
              }
              continue;
            }
          }
          else
          {
            fprintf(stderr, "error: Failed to read a event file.\n");
            return false;
          }
        }

        // 読み込み成功
        try_to_repair = false;

        // キーボードイベントならhandlerへ通知
        if (revent.type == EV_KEY)
        {
          loop_stop = true;
          *event = revent;
          break;
        }
        else if (revent.type == EV_SYN)	// 無視
        {
        }
        else if (revent.type == EV_MSC)	// 無視
        {
        }
        else
        {
          // キーボードイベント以外は、そのまま出力
          write(g_uinput_fd, &revent, sizeof(revent));
        }
      }
    }
  }	
  return true;
}

void keyboard_grab_onoff(bool onoff)
{
  int i;
	
  if (g_envdev_key_fds_count <= 0 && g_uinput_fd <= 0)
    return;
	
  for (i = 0; i < g_envdev_key_fds_count; i++)
  {
    if (ioctl(g_envdev_key_fds[i], EVIOCGRAB, (int)onoff) == -1)	//grab開始
      fprintf(stderr, "error: EVIOCGRAB ioctl failed\n");
  }
  g_grab_onoff = onoff;
}
