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
#include <winnt.h>
#include <oemglobal.h>

// ---------------------------------------------------------------------------
// OEMSaveVFPCtrlRegs: OPTIONAL for ARM, UNUSED for other cpus
//
// This function is called when the kernel needs to save the state of
// extra implementation-defined FPU registers for the current thread.  This
// function is only used on ARM processors that support Vector Floating Point
// (VFP).
//
void OEMSaveVFPCtrlRegs(LPDWORD lpExtra, int nMaxRegs)
{
  // Fill in processor code here.
	DEBUGMSG(1, (TEXT("OEMSaveVFP\r\n")));

  return;
}

// ---------------------------------------------------------------------------
// OEMRestoreVFPCtrlRegs: OPTIONAL for ARM, UNUSED for other cpus
//
// This function is called when the kernel needs to restore the state of
// extra implementation-defined FPU registers for the current thread.  This
// function is only used on ARM processors that support Vector Floating Point
// (VFP).
//
void OEMRestoreVFPCtrlRegs(LPDWORD lpExtra, int nMaxRegs)
{
  // Fill in processor code here.
	DEBUGMSG(1, (TEXT("OEMRestoreVFP\r\n")));

  return;
}

// ---------------------------------------------------------------------------
// OEMHandleVFPException: OPTIONAL for ARM, UNUSED for other cpus
//
// This function is called to handle a floating point expection reported
// by Vector Floating Point (VFP) hardware.  This function is only used on
// ARM processors that support Vector Floating Point (VFP).
//
BOOL OEMHandleVFPException(ULONG fpexc, EXCEPTION_RECORD* pExr, CONTEXT* pContext, DWORD* pdwExcpId, BOOL fInKMode)
{
  // Fill in exception code here.
	DEBUGMSG(1, (TEXT("OEMHandleVFP\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMIsVFPFeaturePresent: OPTIONAL for ARM, UNUSED for other cpus
//
// This function is called to query the OAL for information about
// Vector Floating Point (VFP) hardware.  This function is only used on ARM
// processors that support Vector Floating Point (VFP).
//
BOOL OEMIsVFPFeaturePresent(DWORD dwProcessorFeature)
{
  // Fill in processor feature code here.
	DEBUGMSG(1, (TEXT("OEMVFPPresent %u\r\n"), dwProcessorFeature));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMVFPCoprocControl: OPTIONAL for ARM, UNUSED for other cpus
//
// This function is called to send a command to the Vector Floating Point
// (VFP) hardware.  Typical commands include changing the power state of
// the coprocessor to on, off, or low power.  This function is only used on
// ARM processors that support Vector Floating Point (VFP).
//
BOOL OEMVFPCoprocControl(DWORD dwCommand)
{
	DEBUGMSG(1, (TEXT("OEMVFPCoprocControl\r\n")));
  // Fill in command code here.

  return FALSE;
}


