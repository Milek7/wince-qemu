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
#include <oemglobal.h>

void _wfi(void);
// ---------------------------------------------------------------------------
// OEMIdle: REQUIRED
//
// This function is called by the kernel to place the CPU in the idle state
// when there are no threads ready to run.
//
// If you implement OEMIdleEx, you can leave this function in a stub form
// as the kernel will prefer OEMIdleEx (OEMIdleEx has better performance).
//
void OEMIdle(DWORD dwIdleParam)
{
  // Fill in idle code here.
	//DEBUGMSG(1, (TEXT("OEMIdle\r\n")));
	_wfi();
}

// ---------------------------------------------------------------------------
// OEMPowerOff: REQUIRED
//
// The function is responsible for any final power-down state and for
// placing the CPU into a suspend state.
//
void OEMPowerOff(void)
{
	DEBUGMSG(1, (TEXT("OEMPowerOff\r\n")));
  // Fill in power off code here.

  return;
}

// ---------------------------------------------------------------------------
// OEMHaltSystem: OPTIONAL
//
// The function is called when the kernel is about to halt the system.
//
void OEMHaltSystem(void)
{
	DEBUGMSG(1, (TEXT("OEMHaltSystem\r\n")));
  // Fill in halt code here.

  return;
}

// ---------------------------------------------------------------------------
// OEMIdleEx: OPTIONAL
//
// This function is called by the kernel to place the CPU(s) in the idle state
// when there are no threads ready to run.  It is required for
// multiprocessor support if there is more than one CPU to handle per
// CPUIdle.  OEMIdleEx is more performant than OEMIdle, and if you implement
// OEMIdleEx, you can leave OEMIdle as a stub.
//
void OEMIdleEx(LARGE_INTEGER *pliIdleTime)
{
	DEBUGMSG(1, (TEXT("OEMIdleEx\r\n")));
  // fill in idle code here

  return;
}


