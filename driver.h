//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// driver.h


#ifndef _DRIVER_H
#  define _DRIVER_H

#if defined(_MSC_VER)
#  if defined(_WINNT)

#  include <winioctl.h>

/// mayu device file name
#    define MAYU_DEVICE_FILE_NAME _T("\\\\.\\MayuDetour1")
///
#    define MAYU_DRIVER_NAME _T("mayud")

#  elif defined(_WIN95)

///
#    define MAYU_DEVICE_FILE_NAME _T("\\\\.\\mayud.vxd")

#  endif

# endif

/// Ioctl value
#include "d/ioctl.h"

#include "wintypes.h"  							// linux and mac
 

/// derived from w2kddk/inc/ntddkbd.h
class KEYBOARD_INPUT_DATA
{
public:
  ///
  enum
  {
    /// key release flag
    BREAK = 1,
    /// extended key flag
    E0 = 2,
    /// extended key flag
    E1 = 4,
    /// extended key flag (E0 | E1)
    E0E1 = 6,
    ///
    TERMSRV_SET_LED = 8,
    /// Define the keyboard overrun MakeCode.
    KEYBOARD_OVERRUN_MAKE_CODE_ = 0xFF,
  };

public:
  /** Unit number.  E.g., for \Device\KeyboardPort0 the unit is '0', for
      \Device\KeyboardPort1 the unit is '1', and so on. */
  USHORT UnitId;
  
  /** The "make" scan code (key depression). */
  USHORT MakeCode;
  
  /** The flags field indicates a "break" (key release) and other miscellaneous
      scan code information defined above. */
  USHORT Flags;
  
  ///
  USHORT Reserved;
  
  /** Device-specific additional information for the event. */
  ULONG ExtraInformation;
};


#endif // !_DRIVER_H
