#pragma once

#if !defined(WIN32)

typedef int INT;
typedef unsigned int UINT;

typedef short SHORT;
typedef unsigned short USHORT;

typedef long LONG;
typedef unsigned long ULONG;

typedef unsigned char BYTE;
typedef USHORT WORD;
typedef ULONG DWORD;

typedef INT BOOL;

// for win32 apis
typedef INT HGLOBAL;

typedef INT WPARAM;
typedef INT LPARAM;
typedef INT HWND;

// definition
const int INVALID_HANDLE_VALUE = -1;
#endif

