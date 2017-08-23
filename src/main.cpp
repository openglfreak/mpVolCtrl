#include "unicode.h"

#include <Windows.h>
#include <tchar.h>
#include <CommCtrl.h>

#include "resource.h"
#include "auto_cleanup.hpp"
#include "errors.hpp"
#include "volume_control.hpp"

#define APPWM_VOLUMEUP (WM_APP+1)
#define APPWM_VOLUMEDOWN (WM_APP+2)
#define APPWM_TRAYICON (WM_APP+3)

static HINSTANCE hInstance;
static HMENU hMenu;
static HWND hMainWindow;
static HHOOK hKeyboardHook;

static bool hookDisabled;
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
				volKeyStates[0] = false;
				PostMessage(hMainWindow, APPWM_VOLUMEUP, 0, 0);
			}
			else
				break;
			return 1;
		case WM_KEYUP:
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
	case APPWM_TRAYICON:
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONUP:
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_RIGHTBUTTON | TPM_HORPOSANIMATION | TPM_VERPOSANIMATION, cursorPos.x, cursorPos.y, 0, hWnd, NULL);
			return 0;
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_TRAY_POPUPMENU_TOGGLE:
			hookDisabled = !hookDisabled;
			ModifyMenu(hMenu, IDM_TRAY_POPUPMENU_TOGGLE, MF_BYCOMMAND | MF_STRING, IDM_TRAY_POPUPMENU_TOGGLE, hookDisabled ? _T("Enable ") _T(PRODUCT_NAME) : _T("Disable ") _T(PRODUCT_NAME));
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

void deleteNotifyIcon(NOTIFYICONDATA& notifyIconData)
{
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}

int CALLBACK _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	::hInstance = hInstance;

	HRESULT hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | (COINIT_SPEED_OVER_MEMORY & 0));
	if (!SUCCEEDED(hResult))
	{
		ShowErrorMessage(hResult, _T("CoInitializeEx error"));
		return 1;
	}
	AutoCleanup<void (*)()> comCleanup(CoUninitialize);

	hMainWindow = CreateWindow(WC_STATIC, NULL, 0x00, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hMainWindow == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("CreateWindow error"));
		return 2;
	}
	AutoDeleter<HWND, BOOL(*)(HWND)> hMainWindowDeleter(hMainWindow, DestroyWindow);
	subclassMainWindow();
	AutoCleanup<void(*)()> subclassCleanup(unsubclassMainWindow);

	hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_TRAY_POPUPMENU));
	if (hMenu == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("LoadMenu error"));
		return 3;
	}
	AutoDeleter<HMENU, BOOL(*)(HMENU)> hMenuDeleter(hMenu, DestroyMenu);

	volKeyStates[0] = GetAsyncKeyState(VK_VOLUME_UP) < 0;
	volKeyStates[1] = GetAsyncKeyState(VK_VOLUME_DOWN) < 0;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, (HMODULE)hInstance, 0);
	if (hKeyboardHook == NULL)
	{
		ShowErrorMessage(GetLastError(), _T("SetWindowsHookEx error"));
		return 4;
	}
	AutoDeleter<HHOOK, BOOL(*)(HHOOK)> hKeyboardHookDeleter(hKeyboardHook, UnhookWindowsHookEx);

	NOTIFYICONDATA notifyIconData = { 0 };
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hMainWindow;
	notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON;
	notifyIconData.uCallbackMessage = APPWM_TRAYICON;
	notifyIconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	notifyIconData.uVersion = NOTIFYICON_VERSION;
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
	AutoDeleter<NOTIFYICONDATA&, void(*)(NOTIFYICONDATA&)> notifyIconDeleter(notifyIconData, deleteNotifyIcon);

	MSG msg;
	BOOL bRet;
	while (bRet = GetMessage(&msg, NULL, 0, 0))
	{
		if (bRet == -1)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	typedef std::vector<MediaPlayerVolumeControlProvider*> vect_type;
	for (vect_type::iterator start = volumeControlProviders.begin(), end = volumeControlProviders.end(); start != end; ++start)
		delete *start;

	return bRet ? (int)bRet : (int)msg.wParam;
}
