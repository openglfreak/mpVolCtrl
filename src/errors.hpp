#pragma once
#ifndef __ERRORS_HPP__
#define __ERRORS_HPP__

#include "unicode.h"

#include <Windows.h>
#include <tchar.h>

class GetErrorMessage
{
private:
	static const TCHAR get_error_msg_error[29];

	HRESULT error;
	LPTSTR error_msg;
public:
	GetErrorMessage(HRESULT error) : error(error), error_msg(NULL) { }
	~GetErrorMessage()
	{
		if (error_msg)
			LocalFree(reinterpret_cast<HLOCAL>(error_msg));
	}
private:
	void get_error_msg()
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&error_msg),
			0,
			NULL);
		if (!error_msg)
		{
			error_msg = reinterpret_cast<LPTSTR>(LocalAlloc(LMEM_FIXED, sizeof get_error_msg_error));
			memcpy(error_msg, get_error_msg_error, sizeof get_error_msg_error);
		}
		else
		{
			size_t len = _tcslen(error_msg);
			if (len > 1 && error_msg[len - 1] == _T('\n'))
			{
				error_msg[len - 1] = _T('\0');
				if (error_msg[len - 2] == _T('\r'))
					error_msg[len - 2] = _T('\0');
			}
		}
	}
public:
	operator LPTSTR const&()
	{
		if (!error_msg)
			get_error_msg();
		return error_msg;
	}
};

inline void ShowErrorMessage(DWORD error, LPCTSTR title)
{
	MessageBox(NULL, GetErrorMessage((HRESULT)error), title, MB_OK);
}
inline void ShowErrorMessage(HRESULT hResult, LPCTSTR title)
{
	MessageBox(NULL, GetErrorMessage(hResult), title, MB_OK);
}

#endif // __ERRORS_HPP__
