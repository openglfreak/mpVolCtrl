#include "unicode.h"

#include <Windows.h>
#include <tchar.h>
#include <wchar.h>
#include <CommCtrl.h>
#include <Shlobj.h>

#include <fstream>
#include <memory>
#include <string>

#include "resource.h"
#include "auto_cleanup.hpp"
#include "errors.hpp"
#include "volume_control.hpp"
#include "mpvc_config.hpp"

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

MPVCConfig mpvc_config;

static bool volKeyStates[2];
LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION)
		switch (wParam)
		{
		case WM_KEYDOWN:
			if (mpvc_config.disabled)
				break;
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
	mpvc_config.invisible = false;
	*notifyIconDeleter = false;
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

void deleteNotifyIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
	*notifyIconDeleter = true;
	mpvc_config.invisible = true;
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
			if (mpvc_config.invisible)
				addNotifyIcon();
			break;
		case 2:
			if (!mpvc_config.invisible)
				deleteNotifyIcon();
			break;
		default:
			if (mpvc_config.invisible)
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
			CheckMenuItem(hMenu, IDM_TRAY_POPUPMENU_TOGGLE, MF_BYCOMMAND | (!mpvc_config.disabled ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(hMenu, IDM_TRAY_POPUPMENU_SETTINGS_STARTHIDDEN, MF_BYCOMMAND | (mpvc_config.startHidden ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem(hMenu, IDM_TRAY_POPUPMENU_SETTINGS_STARTDISABLED, MF_BYCOMMAND | (mpvc_config.startDisabled ? MF_CHECKED : MF_UNCHECKED));
			TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_RIGHTBUTTON | TPM_HORPOSANIMATION | TPM_VERPOSANIMATION, cursorPos.x, cursorPos.y, 0, hWnd, NULL);
			return 0;
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_TRAY_POPUPMENU_TOGGLE:
			mpvc_config.disabled = !mpvc_config.disabled;
			return 0;
		case IDM_TRAY_POPUPMENU_HIDE:
			deleteNotifyIcon();
			return 0;
		case IDM_TRAY_POPUPMENU_SETTINGS_STARTHIDDEN:
			mpvc_config.startHidden = !mpvc_config.startHidden;
			return 0;
		case IDM_TRAY_POPUPMENU_SETTINGS_STARTDISABLED:
			mpvc_config.startDisabled = !mpvc_config.startDisabled;
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

	if (!mpvc_config.read_config())
	{
		MessageBox(NULL, _T("Couldn't read config.txt"), _T("Config error"), MB_OK);
		return 5;
	}
	struct __config_write {
		~__config_write() { mpvc_config.write_config(); }
	} __config_write_inst;

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
	if (!mpvc_config.invisible)
		Shell_NotifyIcon(NIM_ADD, &notifyIconData);
	AutoCleanup<void(*)()> notifyIconDeleter(deleteNotifyIcon, mpvc_config.invisible);
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
