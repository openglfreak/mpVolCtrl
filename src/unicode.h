#pragma once

#if UNICODE && !defined(_UNICODE)
#define _UNICODE
#elif _UNICODE && !defined(UNICODE)
#define UNICODE
#elif UNICODE != _UNICODE
#undef UNICODE
#undef _UNICODE
#define UNICODE 0
#define _UNICODE 0
#endif
