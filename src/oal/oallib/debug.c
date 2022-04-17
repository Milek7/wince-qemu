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
#include <nkintr.h>
#include <oemglobal.h>
#include "mmio.h"

// Like all OAL functions in the template BSP, there are three function types:
// REQUIRED - you must implement this function for kernel functionality
// OPTIONAL - you may implement this function
// CUSTOM   - this function is a helper function specific to this BSP

// ---------------------------------------------------------------------------
// OEMInitDebugSerial: REQUIRED
//
// This function initializes the debug serial port on the target device,
// useful for debugging OAL bringup.
//
// This is the first OAL function that the kernel calls, before the OEMInit
// function and before the kernel data section is fully initialized.
//
// OEMInitDebugSerial can use global variables; however, these variables might
// not be initialized and might subsequently be cleared when the kernel data
// section is initialized.
//

void OEMWriteDebugByte(BYTE bChar);
void OEMInitDebugSerial(void)
{
	// 8 bit data transmission (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 5) | (1 << 6));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));

	OEMWriteDebugByte('O');
	OEMWriteDebugByte('E');
	OEMWriteDebugByte('M');
	OEMWriteDebugByte('\r');
	OEMWriteDebugByte('\n');
}


// ---------------------------------------------------------------------------
// OEMWriteDebugByte: REQUIRED
//
// This function outputs a byte to the debug monitor port.
//
void OEMWriteDebugByte(BYTE bChar)
{
	mmio_write(UART0_DR, bChar);
}

// ---------------------------------------------------------------------------
// OEMWriteDebugString: REQUIRED
//
// This function writes a byte to the debug monitor port.
//
void OEMWriteDebugString(unsigned short *pszStr)
{
	while (*pszStr != 0) {
		OEMWriteDebugByte((BYTE)*pszStr);
		pszStr++;
	}
}

// ---------------------------------------------------------------------------
// OEMReadDebugByte: OPTIONAL
//
// This function retrieves a byte to the debug monitor port.
//
int OEMReadDebugByte(void)
{
	if (mmio_read(UART0_FR) & (1 << 4))
		return OEM_DEBUG_READ_NODATA;

	return mmio_read(UART0_DR);
}

// ---------------------------------------------------------------------------
// dpCurSettings: REQUIRED
//
// The variable enables the OAL's internal debug zones so that debug messaging
// macros like DEBUGMSG can be used.  dpCurSettings is a set of names for
// debug zones which are enabled or disabled by the bitfield parameter at
// the end.  There are 16 zones, each with a customizable name.
//
#ifdef DEBUG
DBGPARAM dpCurSettings = 
{
  // Name of the debug module
  TEXT("OAL"), 
  {
    // Names of the individual zones
    TEXT("Error"),      TEXT("Warning"),    TEXT("Function"),   TEXT("Info"),
    TEXT("Stub/Keyv/Args"), TEXT("Cache"),  TEXT("RTC"),        TEXT("Power"),
    TEXT("PCI"),        TEXT("Memory"),     TEXT("IO"),         TEXT("Timer"),
    TEXT("IoCtl"),      TEXT("Flash"),      TEXT("Interrupts"), TEXT("Verbose")
  },
  // Bitfield controlling the zones.  1 means the zone is enabled, 0 disabled
  // We'll enable the Error, Warning, and Info zones by default here
  1 | 1 << 1 | 1 << 3
};
#endif
