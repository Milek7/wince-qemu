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

// ---------------------------------------------------------------------------
// OEMMpStartAllCPUs: OPTIONAL
//
// This function is called by the kernel before multiprocessing is enabled.
// It runs on the master CPU and starts up all non-master CPUs.  This function
// is required for multiprocessor support.
//
BOOL OEMMpStartAllCPUs(PLONG pnCpus, FARPROC pfnContinue)
{
  // fill in processor init code here
	DEBUGMSG(1, (TEXT("OEMMpStartAllCPUs\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMMpPerCPUInit: OPTIONAL
//
// This function is called by the kernel on each slave CPU.  It is the first
// function the kernel calls on the slave CPU and it returns the hardware
// CPUID.  This function is required for multiprocessor support.
//
DWORD OEMMpPerCPUInit(void)
{
  // fill in processor init code here
	DEBUGMSG(1, (TEXT("OEMMpPerCPUInit\r\n")));

  return 0;
}

// ---------------------------------------------------------------------------
// OEMMpCpuPowerFunc: OPTIONAL
//
// This function is called by the kernel to power on/off a specific CPU for
// power saving purposes, or do a partial power down/up according to the hint
// value provided by the kernel.  It is optional for multiprocessor support.  
//
BOOL OEMMpCpuPowerFunc(DWORD dwProcessor, BOOL fOnOff, DWORD dwHint)
{
  // fill in processor power code here
	DEBUGMSG(1, (TEXT("OEMMpCpuPowerFunc\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMIpiHandler: OPTIONAL
//
// This function handles platform-specific interprocessor interrupts.  It is
// optional for multiprocessor support.
//
void OEMIpiHandler(DWORD dwCommand, DWORD dwData)
{
  // fill in interrupt code here
	DEBUGMSG(1, (TEXT("OEMIpiHandler\r\n")));

  return;
}

// ---------------------------------------------------------------------------
// OEMSendIPI: OPTIONAL
//
// This function sends an interprocessor interrupt.  It is required for
// multiprocessor support.
//
BOOL OEMSendIPI(DWORD dwType, DWORD dwTarget)
{
  // fill in interrupt code here
	DEBUGMSG(1, (TEXT("OEMSendIPI\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMInitInterlockedFunctions: OPTIONAL
//
// This function initializes the interlocked function table for the OAL.  It
// is required for multiprocessor support.  However, a default implementation
// is provided by NKStub for each CPU - only override this function if you
// need platform-specific interlocked function initialization.
//
void OEMInitInterlockedFunctions(void)
{
  // fill in function table code here
	DEBUGMSG(1, (TEXT("OEMInitInterlockedFunctions\r\n")));

  return;
}


