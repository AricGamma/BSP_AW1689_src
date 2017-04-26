#pragma once
#include "Driver.h"
#include "Device.h"
#include "Common.h"

#ifdef __cplusplus
extern "C" {
#endif

	void InjectKeyDown(
		UCHAR vk,
		PDEVICE_CONTEXT pDevContext
	);

	void InjectKeyUp(
		UCHAR vk,
		PDEVICE_CONTEXT pDevContext
	);

	void InjectScanKeyDown(
		WORD scanCode,
		PDEVICE_CONTEXT pDevContext
	);
	void InjectUnicode(
		WORD wch,
		PDEVICE_CONTEXT pDevContext
	);
	void InjectScanKeyUp(
		WORD scanCode,
		PDEVICE_CONTEXT pDevContext
	);

	void InjectMouseMove(
		WORD X,
		WORD Y,
		UINT Buttons,
		PDEVICE_CONTEXT pDevContext
	);


	UINT InjectSendInput(
		_In_ UINT    nInputs,
		_In_ LPINPUT pInputs,
		_In_ int     cbSize,
		_In_ PDEVICE_CONTEXT pDevContext
	);


#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002
#if(_WIN32_WINNT >= 0x0500)
#define KEYEVENTF_UNICODE     0x0004
#define KEYEVENTF_SCANCODE    0x0008
#endif /* _WIN32_WINNT >= 0x0500 */

#define MOUSEEVENTF_MOVE        0x0001 /* mouse move */
#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */
#define MOUSEEVENTF_XDOWN       0x0080 /* x button down */
#define MOUSEEVENTF_XUP         0x0100 /* x button down */
#define MOUSEEVENTF_WHEEL                0x0800 /* wheel button rolled */
#if (_WIN32_WINNT >= 0x0600)
#define MOUSEEVENTF_HWHEEL              0x01000 /* hwheel button rolled */
#endif
#if(WINVER >= 0x0600)
#define MOUSEEVENTF_MOVE_NOCOALESCE      0x2000 /* do not coalesce mouse moves */
#endif /* WINVER >= 0x0600 */
#define MOUSEEVENTF_VIRTUALDESK          0x4000 /* map to entire virtual desktop */
#define MOUSEEVENTF_ABSOLUTE             0x8000 /* absolute move */

#define INPUT_MOUSE     0
#define INPUT_KEYBOARD  1
#define INPUT_HARDWARE  2

#ifdef __cplusplus
}	// extern "C"
#endif

