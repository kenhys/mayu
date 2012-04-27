#pragma once

#if defined(WIN32)
#include <tchar.h>
#elif defined(__GNUC__)

#if defined(UNICODE)
typedef wchar_t		_TCHAR;
#define _T(x)		L ## x

#elif defined(_MBCS)
typedef char		_TCHAR;
#else //contain _SBCS
typedef char		_TCHAR;
typedef _TCHAR		TCHAR;
#define	_tcsicmp	strcasecmp
#define _T(x)		x
#define _sntprintf	snprintf
#endif

// _T functions

// euc‚Æsjis‚Ì‚Ý‘Î‰ž ‚Æ‚è‚ ‚¦‚¸AUTF-8‚Æ‚© wchar_t ‚ÍŒã‰ñ‚µ
// TODO:
inline bool _ismbblead(unsigned char c)
{
	return (0xFE >= c && c >= 0xA1) // euc
		|| (0x9F >= c && c >= 0x81) // sjis
		|| (0xEF >= c && c >= 0xE0) // sjis
		;
}

#define _istalpha(c)	isalpha(c)
#define _istdigit(c)	isdigit(c)
#define _istpunct(c)	ispunct(c)
#define _istspace		isspace
#define _tcschr			strchr
#define _istgraph		isgraph
#define _tcsnicmp		strncasecmp
#define _istprint		isprint
#define _tcstol			strtol
#define _tfopen			fopen
#define _tstati64		stat
#define _istlead   		_ismbblead
#endif
