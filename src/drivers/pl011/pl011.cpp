//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include <windows.h>
#include <devload.h>
#include <pegdser.h>

#define ZONE_INIT  DEBUGZONE(0)
#define ZONE_ERROR DEBUGZONE(1)
#define ZONE_WARN  DEBUGZONE(2)
#define ZONE_INFO  DEBUGZONE(3)

#define uint32_t unsigned int
#define mmio_write(reg, data)  *(volatile uint32_t*)(reg) = (data)
#define mmio_read(reg)  *(volatile uint32_t*)(reg)

enum pl011_regs
{
	UART_DR     = 0x00,
	UART_FR     = 0x18,
	UART_LCRH   = 0x2C,
	UART_CR     = 0x30,
	UART_IFLS   = 0x34,
    UART_IMSC   = 0x38
};

BOOL RegGetValue(const WCHAR* ActiveKey, const WCHAR* pwsName, PDWORD pdwValue);

// ---------------------------------------------------------------------------
// DllMain: REQUIRED
//
// This function is the first function that is called when the stream driver
// loads (DLL_PROCESS_ATTACH case).  It is also called when the driver unloads
// (DLL_PROCESS_DETACH case).  It should do minimal handling and setup for the
// driver; the majority of initialization occurs later, in the Init function.
// Any initialization that can be delayed until the Init function, should be.
//
extern "C" BOOL WINAPI DllMain(
  HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved
)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:

      // This function registers the dpCurSettings global variable
      // with the kernel debug system.  This allows the DEBUGMSG
      // macro to be used to output debug information from
      // the driver.
      //
      // If dpCurSettings is not implemented, this line should be removed.
      DEBUGREGISTER((HINSTANCE) hinstDLL);

      // This turns off the DLL_THREAD_ATTACH and DLL_THREAD_ATTACH
      // notifications for the driver.  Unless a driver is interested
      // in tracking resource use at the thread level, it is good
      // practice to call this function.
      DisableThreadLibraryCalls ((HMODULE) hinstDLL);
      break;

	default:
      break;
  }
     
  return TRUE;
}

struct port_state
{
	DWORD io_base;
	DWORD sysintr;
	BOOL is_open;
	HANDLE interrupt;
	HANDLE breakev;
	COMMTIMEOUTS timeouts;
};

// ---------------------------------------------------------------------------
// COM_Init: REQUIRED
//
// This function initializes the device driver.  It is called only once for
// the driver, and is responsible for setting the default state of the device
// as well as allocating and initializing any global variables or resources.
//
// Upon success, this function returns a handle to the driver's state.  This
// is passed to Open and Deinit when they are called.
//
extern "C" DWORD COM_Init(LPCTSTR pContext)
{
	port_state *state = new port_state();

	DEBUGMSG(ZONE_INFO, (TEXT("COM_Init: %u\r\n"), (DWORD)state));

	state->is_open = FALSE;
	memset(&state->timeouts, 0, sizeof(COMMTIMEOUTS));

	if (!RegGetValue(pContext, TEXT("IoBase"), &state->io_base)) {
		delete state;
		return NULL;
	}

	if (!RegGetValue(pContext, TEXT("SysIntr"), &state->sysintr)) {
		delete state;
		return NULL;
	}

	mmio_write(state->io_base + UART_IFLS, 0);
	mmio_write(state->io_base + UART_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	DEBUGMSG(ZONE_INIT, (TEXT("pl011: init ok\r\n")));

	return (DWORD)state;
}

extern "C" BOOL COM_Close(DWORD hOpenContext);
// ---------------------------------------------------------------------------
// COM_Deinit: REQUIRED
//
// This function deinitializes the device driver.  It is called only once for
// the driver, and is responsible for freeing any allocated resources.
//
extern "C" BOOL COM_Deinit(DWORD hDeviceContext)
{
	port_state *state = (port_state*)hDeviceContext;

	if (state->is_open) {
		DEBUGMSG(ZONE_WARN, (TEXT("pl011: opened during deinit!\r\n")));
		COM_Close(hDeviceContext);
	}

	DEBUGMSG(ZONE_INFO, (TEXT("COM_Deinit: %u\r\n"), hDeviceContext));
	delete state;

	return TRUE;
}

// ---------------------------------------------------------------------------
// COM_Open: OPTIONAL
//
// This function prepares the device driver for reading, writing, or both.
// It creates a context that is returned to an application.  The application
// uses this context to call into the driver to perform reads and writes.
//
// The driver may optionally allocate a new context each time this function
// is called.  This allows multiple applications to use the driver
// simultaneously.  This behavior is called multiple access.  COM_Open is
// required for the device driver to support multiple access.  The driver is
// responsible for tracking all of the contexts that it creates so that it
// can deallocate them when Close or Deinit are called.
//
// This function allows applications to create handles to the driver using
// CreateFile, and is called when an application calls CreateFile.
// This function is required for the device driver to be accessed via
// CreateFile.
//
extern "C" DWORD COM_Open(
  DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode
)
{
	port_state *state = (port_state*)hDeviceContext;

	DEBUGMSG(ZONE_INFO, (TEXT("COM_Open: %u\r\n"), hDeviceContext));

	if (state->is_open) {
		DEBUGMSG(ZONE_WARN, (TEXT("COM_Open: already open\r\n")));
		return NULL;
	}

	mmio_write(state->io_base + UART_IMSC, (1 << 4));
	mmio_write(state->io_base + UART_CR, (1 << 0) | (1 << 8) | (1 << 9));

	state->interrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
	state->breakev = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!InterruptInitialize(state->sysintr, state->interrupt, NULL, 0)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("COM_Open: InterruptInitialize failed\r\n")));
		CloseHandle(state->interrupt);
		CloseHandle(state->breakev);
		return NULL;
	}

	state->is_open = TRUE;

	return (DWORD)state;
}

// ---------------------------------------------------------------------------
// COM_Close: OPTIONAL
//
// This function invalidates a context that was created by Open.  It
// deallocates any resources associated with that context.
//
// This function is called when an application calls CloseHandle.
//
extern "C" BOOL COM_Close(DWORD hOpenContext)
{
	port_state *state = (port_state*)hOpenContext;

	DEBUGMSG(ZONE_INFO, (TEXT("COM_Close: %u\r\n"), hOpenContext));

	if (!state->is_open) {
		DEBUGMSG(ZONE_INFO, (TEXT("COM_Close: not open!\r\n")));
		return TRUE;
	}

	mmio_write(state->io_base + UART_IMSC, 0);
	mmio_write(state->io_base + UART_CR, 0);

	if (WaitForSingleObject(state->interrupt, 0) != WAIT_TIMEOUT)
		InterruptDone(state->sysintr);

	SetEvent(state->breakev);

	InterruptDisable(state->sysintr);
	CloseHandle(state->interrupt);
	CloseHandle(state->breakev);
	state->is_open = FALSE;
	
	return TRUE;
}

// ---------------------------------------------------------------------------
// COM_Read: OPTIONAL
//
// This function reads data sequentially from the device.  It is called when
// an application calls ReadFile.
//
extern "C" DWORD COM_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count, PULONG pBytesRead)
{
	DEBUGMSG(ZONE_INFO, (TEXT("COM_Read: Count=%u\r\n"), Count));

	port_state *state = (port_state*)hOpenContext;
	BYTE *buf = (BYTE*)pBuffer;
	DWORD i = 0;

	*pBytesRead = 0;
	if (Count == 0)
		return 0;

	DWORD total_timeout = state->timeouts.ReadTotalTimeoutMultiplier * Count + state->timeouts.ReadTotalTimeoutConstant;
	if (total_timeout == 0)
		total_timeout = INFINITE;

	ResetEvent(state->breakev);
	HANDLE handles[] = { state->interrupt, state->breakev };
	DWORD wait = WaitForMultipleObjects(2, handles, FALSE, total_timeout);
	if (wait == WAIT_TIMEOUT || wait == WAIT_OBJECT_0 + 1)
		return 0;

	while ((mmio_read(state->io_base + UART_FR) & (1 << 4)) == 0) {
		buf[i++] = (BYTE)mmio_read(state->io_base + UART_DR);
		if (i == Count)
			break;
	}

	InterruptDone(state->sysintr);

	*pBytesRead = i;
	return i;
}

// ---------------------------------------------------------------------------
// COM_Write: OPTIONAL
//
// This function writes data sequentially to the device.  It is called when an
// application calls WriteFile.
//
extern "C" DWORD COM_Write(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	DEBUGMSG(ZONE_INFO, (TEXT("COM_Write: Count=%u\r\n"), Count));

	port_state *state = (port_state*)hOpenContext;

	BYTE *buf = (BYTE*)pBuffer;
	for (DWORD i = 0; i < Count; i++)
		mmio_write(state->io_base + UART_DR, buf[i]);

	return Count;
}

// ---------------------------------------------------------------------------
// COM_IOControl: OPTIONAL
//
// This function sends a command to the device.  The driver may take a
// variety of different actions based upon the command code that is passed in.
// There are some predefined command code used across many drivers.  However,
// custom command codes are often used depending on the device's capabilities.
//
// Examples of predefined command codes are IOCTL_POWER_GET and
// IOCTL_POWER_SET.  These are typically used instead of PowerUp and
// PowerDown to control the power state of the device.
// 
extern "C" BOOL COM_IOControl(
  DWORD hOpenContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn,
  PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut
)
{
	port_state *state = (port_state*)hOpenContext;
	DEBUGMSG(ZONE_INFO, (TEXT("COM_IOControl: %u\r\n"), dwCode));

	switch (dwCode) {
		case IOCTL_SERIAL_SET_BREAK_ON:
		case IOCTL_SERIAL_SET_BREAK_OFF:
		case IOCTL_SERIAL_SET_DTR:
		case IOCTL_SERIAL_CLR_DTR:
		case IOCTL_SERIAL_SET_RTS:
		case IOCTL_SERIAL_CLR_RTS:
		case IOCTL_SERIAL_SET_XOFF:
		case IOCTL_SERIAL_SET_XON:
		case IOCTL_SERIAL_SET_QUEUE_SIZE:
		case IOCTL_SERIAL_SET_DCB:
		case IOCTL_SERIAL_PURGE:
			{
				if (dwLenIn < 4) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				DWORD func = *(DWORD*)pBufIn;
				if (func & PURGE_RXABORT)
					SetEvent(state->breakev);
				return TRUE;
			}

		case IOCTL_SERIAL_GET_COMMSTATUS:
			{
				if (dwLenOut < sizeof(SERIAL_DEV_STATUS)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				memset(pBufOut, 0, sizeof(SERIAL_DEV_STATUS));
				*pdwActualOut = sizeof(SERIAL_DEV_STATUS);

				return TRUE;
			}

		case IOCTL_SERIAL_GET_PROPERTIES:
			{
				if (dwLenOut < sizeof(COMMPROP)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				COMMPROP prop;
				memset(&prop, 0, sizeof(COMMPROP));

				prop.wPacketLength = sizeof(COMMPROP);
				prop.wPacketVersion = 1;
				prop.dwServiceMask = SP_SERIALCOMM;
				prop.dwMaxBaud = BAUD_115200;
				prop.dwProvSubType = PST_RS232;
				prop.dwProvCapabilities = PCF_TOTALTIMEOUTS;
				prop.dwSettableParams = SP_BAUD | SP_DATABITS | SP_STOPBITS;
				prop.dwSettableBaud = BAUD_115200;
				prop.wSettableData = DATABITS_8;
				prop.wSettableStopParity = STOPBITS_10;

				memcpy(pBufOut, &prop, sizeof(COMMPROP));
				*pdwActualOut = sizeof(COMMPROP);

				return TRUE;
			}

		case IOCTL_SERIAL_GET_DCB:
			{
				if (dwLenOut < sizeof(DCB)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				DCB dcb;
				memset(&dcb, 0, sizeof(DCB));

				dcb.DCBlength = sizeof(DCB);
				dcb.BaudRate = CBR_115200;
				dcb.fBinary = 1;
				dcb.fDtrControl = DTR_CONTROL_DISABLE;
				dcb.fRtsControl = RTS_CONTROL_DISABLE;
				dcb.ByteSize = 8;
				dcb.StopBits = ONESTOPBIT;

				memcpy(pBufOut, &dcb, sizeof(DCB));
				*pdwActualOut = sizeof(DCB);

				return TRUE;
			}

		case IOCTL_SERIAL_GET_TIMEOUTS:
			{
				if (dwLenOut < sizeof(COMMTIMEOUTS)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				memcpy(pBufOut, &state->timeouts, sizeof(COMMTIMEOUTS));
				*pdwActualOut = sizeof(COMMTIMEOUTS);

				return TRUE;
			}

		case IOCTL_SERIAL_SET_TIMEOUTS:
			{
				if (dwLenIn < sizeof(COMMTIMEOUTS)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
				memcpy(&state->timeouts, pBufIn, sizeof(COMMTIMEOUTS));
				DEBUGMSG(ZONE_INFO, (TEXT("COM_IOControl: SET_TIMEOUTS: %u %u %u\r\n"),
					state->timeouts.ReadIntervalTimeout,
					state->timeouts.ReadTotalTimeoutMultiplier,
					state->timeouts.ReadTotalTimeoutConstant));
				return TRUE;
			}

		default:
			break;
	}

	DEBUGMSG(ZONE_WARN, (TEXT("COM_IOControl: not supported %u\r\n"), dwCode));
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

// ---------------------------------------------------------------------------
// dpCurSettings: OPTIONAL
//
// The variable enables the drivers internal debug zones so that debug
// messaging macros like DEBUGMSG can be used.  dpCurSettings is a set of
// names for debug zones which are enabled or disabled by the bitfield
// parameter at the end.  There are 16 zones, each with a customizable name.
//
// If dpCurSettings is not implemented, the DEBUGREGISTER macro must be
// removed from the Init function.
//
#ifdef DEBUG
DBGPARAM dpCurSettings = 
{
  // Name of the debug module
  TEXT("pl011"), 
  {
    // Names of the individual zones
    TEXT("Init"),       TEXT("Error"),      TEXT("Warning"),    TEXT("Info")
  },
  // Bitfield controlling the zones.  1 means the zone is enabled, 0 disabled
  0x7
};
#endif