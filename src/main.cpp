#include "unicode.h"

#include <Windows.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <Shlobj.h>

#include <fstream>
#include <memory>

#include "resource.h"
#include "auto_cleanup.hpp"
#include "errors.hpp"
#include "volume_control.hpp"
#include "config.hpp"

#define APPWM_VOLUMEUP (WM_APP+1)
#define APPWM_VOLUMEDOWN (WM_APP+2)
#define APPWM_TRAYICON (WM_APP+3)
#define APPWM_TOGGLENICON (WM_APP+4)

static const TCHAR mainWindowName[] = _T("mpVolCtrl Message Window");

static HINSTANCE hInstance;
static HMENU hMenu;
static HWND hMainWindow;
static HHOOK hKeyboardHook;

NOTIFYICONDATA notifyIconData = { 0 };

static bool hookDisabled = false;
static bool notifyIconInvisible = false;

static bool volKeyStates[2];
LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION && !hookDisabled)
		switch (wParam)
		{
		case WM_KEYDOWN:
			if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_DOWN)
			{
				volKeyStates[1] = true;
				PostMessage(hMainWindow, APPWM_VOLUMEDOWN, 0, 0);
			}
			else if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_UP)
			{
				volKeyStates[0] = true;
				PostMessage(hMainWindow, APPWM_VOLUMEUP, 0, 0);
			}
			else
				break;
			return 1;
		case WM_SYSKEYDOWN:
			if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_DOWN)
			{
				volKeyStates[1] = true;
				PostQuitMessage(0);
			}
			else if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_UP)
			{
				volKeyStates[0] = true;
				PostMessage(hMainWindow, APPWM_TOGGLENICON, 0, 0);
			}
			else
				break;
			return 1;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (volKeyStates[1] && ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_DOWN)
				volKeyStates[1] = false;
			else if (volKeyStates[0] && ((KBDLLHOOKSTRUCT*)lParam)->vkCode == VK_VOLUME_UP)
				volKeyStates[0] = false;
			else
				break;
			return 1;
		}
	return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}

AutoCleanup<void(*)()>* notifyIconDeleter;

void addNotifyIcon()
{
	notifyIconInvisible = false;
	*notifyIconDeleter = false;
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

void deleteNotifyIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
	*notifyIconDeleter = true;
	notifyIconInvisible = true;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case APPWM_VOLUMEUP:
		{
			bool cntrl = GetAsyncKeyState(VK_CONTROL) < 0;
			volume_up(GetAsyncKeyState(VK_SHIFT) < 0 ? cntrl ? .10f : .20f : cntrl ? .01f : .05f);
		}
		return 0;
	case APPWM_VOLUMEDOWN:
		{
			bool cntrl = GetAsyncKeyState(VK_CONTROL) < 0;
			volume_down(GetAsyncKeyState(VK_SHIFT) < 0 ? cntrl ? .10f : .20f : cntrl ? .01f : .05f);
		}
		return 0;
	case APPWM_TOGGLENICON:
		switch (lParam & 3)
		{
		case 1:
			if (notifyIconInvisible)
				addNotifyIcon();
			break;
		case 2:
			if (!notifyIconInvisible)
				deleteNotifyIcon();
			break;
		default:
			if (notifyIconInvisible)
				addNotifyIcon();
			else
				deleteNotifyIcon();
		}
		break;
	case APPWM_TRAYICON:
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONUP:
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetForegroundWindow(hWnd);
			ModifyMenu(hMenu, IDM_TRAY_POPUPMENU_TOGGLE, MF_BYCOMMAND | MF_STRING, IDM_TRAY_POPUPMENU_TOGGLE, hookDisabled ? _T("Enable ") _T(PRODUCT_NAME) : _T("Disable ") _T(PRODUCT_NAME));
			TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_RIGHTBUTTON | TPM_HORPOSANIMATION | TPM_VERPOSANIMATION, cursorPos.x, cursorPos.y, 0, hWnd, NULL);
			return 0;
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_TRAY_POPUPMENU_TOGGLE:
			hookDisabled = !hookDisabled;
			return 0;
		case IDM_TRAY_POPUPMENU_HIDE:
			deleteNotifyIcon();
			return 0;
		case IDM_TRAY_POPUPMENU_EXIT:
			PostQuitMessage(0);
			return 0;
		}
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LONG_PTR prevWndProc;
void subclassMainWindow()
{
	prevWndProc = SetWindowLongPtr(hMainWindow, GWLP_WNDPROC, (LONG_PTR)&WindowProc);
}
void unsubclassMainWindow()
{
	SetWindowLongPtr(hMainWindow, GWLP_WNDPROC, prevWndProc);
}

struct MPVCConfig
{
	static ConfigIO<_TCHAR> configIO;

	static void registerOptions()
	{
		configIO.add_option(_T("Disabled"), _T("Whether the Media Keys redirection is disabled or enabled on start. true for disabled and false for enabled"), hookDisabled);
		configIO.add_option(_T("StartInvisible"), _T("Whether the Notification Area icon is shown or not. true for visible and false for hidden"), notifyIconInvisible);
	}

	static bool readConfig()
	{
		std::basic_string<_TCHAR> configPath(MAX_PATH, _T('\0'));
		while (!SUCCEEDED(SHGetSpecialFolderPath(NULL, &configPath[0], CSIDL_APPDATA, TRUE)) && !SUCCEEDED(SHGetSpecialFolderPath(NULL, &configPath[0], CSIDL_LOCAL_APPDATA, TRUE)) && !SUCCEEDED(SHGetSpecialFolderPath(NULL, &configPath[0], CSIDL_MYDOCUMENTS, TRUE)))
		{
			int o = MessageBox(NULL, _T("Couldn't determine config folder"), _T("SHGetSpecialFolderPath error"), MB_ABORTRETRYIGNORE | MB_DEFBUTTON2);
			if (o == IDABORT)
				return false;
			else if (o == IDRETRY)
				continue;
			else if (o == IDIGNORE)
				return true;
		}
		std::basic_fstream<_TCHAR> fs;
#ifdef _MSC_VER
		configPath.resize(_tcslen(configPath.data()));
		configPath.append(_T("\\mpVolCtrl"));
		CreateDirectory(configPath.c_str(), NULL);
		configPath.append(_T("\\config.txt"));
		fs.open(configPath, std::basic_fstream<_TCHAR>::in);
#else
		DWORD len;
		if ((len = GetShortPathName(&configPath[0], &configPath[0], MAX_PATH)) == 0)
			return true;
		else if (len > MAX_PATH)
		{
			configPath.resize(len);
			DWORD len2;
			if ((len2 = GetShortPathName(&configPath[0], &configPath[0], len)) == 0 || len2 > len)
				return true;
		}
		configPath.resize(len);
		configPath.append(_T("\\mpVolCtrl"));
		CreateDirectory(configPath.c_str(), NULL);
		configPath.append(_T("\\config.txt"));

		std::string configPathNarrow;
		configPathNarrow.reserve(configPath.length());
		std::copy(configPath.begin(), configPath.end(), std::back_inserter(configPathNarrow));
		fs.open(configPathNarrow, std::basic_fstream<_TCHAR>::in);
#endif
		if (fs.fail())
		{
#ifdef _MSC_VER
			fs.open(configPath, std::basic_fstream<_TCHAR>::out);
#else
			fs.open(configPathNarrow, std::basic_fstream<_TCHAR>::out);
#endif
			if (!fs.fail())
			{
				fs << std::noskipws << "# Media Player Volume Control config" << std::endl << "# Generated by Media Player Volume Control " VERSION_STRING << std::endl << std::endl;
				configIO.write_config(fs);
			}
		}
		else
			configIO.read_config(fs >> std::noskipws);
		if (fs.is_open())
			fs.close();
		return true;
	}
};
ConfigIO<_TCHAR> MPVCConfig::configIO;

int CALLBACK _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	::hInstance = hInstance;

	HANDLE hSingleInstanceLockEvent = CreateEvent(NULL, FALSE, FALSE, _T("Local\\mpVolCtrl_lock"));
	AutoDeleter<HANDLE, BOOL(WINAPI *)(HANDLE)> hSingleInstanceLockEventDeleter(hSingleInstanceLockEvent, CloseHandle, hSingleInstanceLockEvent != NULL);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		if ((hMainWindow = FindWindow(WC_STATIC, mainWindowName)))
			SendMessage(hMainWindow, APPWM_TOGGLENICON, 0, 1);
		return 0;
	}

	MPVCConfig::registerOptions();
	if (!MPVCConfig::readConfig())
	{
		MessageBox(NULL, _T("Couldn't read config.txt"), _T("Config error"), MB_OK);
		return 5;
	}

	HRESULT hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("CoInitializeEx error"));
		return 1;
	}
	AutoCleanup<void (WINAPI *)()> comCleanup(CoUninitialize);

	hMainWindow = CreateWindow(WC_STATIC, mainWindowName, 0x00, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hMainWindow == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("CreateWindow error"));
		return 2;
	}
	AutoDeleter<HWND, BOOL (WINAPI *)(HWND)> hMainWindowDeleter(hMainWindow, DestroyWindow);
	subclassMainWindow();
	AutoCleanup<void (*)()> subclassCleanup(unsubclassMainWindow);

	hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_TRAY_POPUPMENU));
	if (hMenu == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("LoadMenu error"));
		return 3;
	}
	AutoDeleter<HMENU, BOOL (WINAPI *)(HMENU)> hMenuDeleter(hMenu, DestroyMenu);

	volKeyStates[0] = GetAsyncKeyState(VK_VOLUME_UP) < 0;
	volKeyStates[1] = GetAsyncKeyState(VK_VOLUME_DOWN) < 0;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, (HMODULE)hInstance, 0);
	if (hKeyboardHook == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("SetWindowsHookEx error"));
		return 4;
	}
	AutoDeleter<HHOOK, BOOL (WINAPI *)(HHOOK)> hKeyboardHookDeleter(hKeyboardHook, UnhookWindowsHookEx);

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hMainWindow;
	notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON;
	notifyIconData.uCallbackMessage = APPWM_TRAYICON;
	notifyIconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	notifyIconData.uVersion = NOTIFYICON_VERSION;
	if (!notifyIconInvisible)
		Shell_NotifyIcon(NIM_ADD, &notifyIconData);
	AutoCleanup<void(*)()> notifyIconDeleter(deleteNotifyIcon, notifyIconInvisible);
	::notifyIconDeleter = &notifyIconDeleter;

	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)))
	{
		if (bRet == -1)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete_volume_controls();

	return bRet ? (int)bRet : (int)msg.wParam;
}
