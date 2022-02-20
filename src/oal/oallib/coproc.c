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
// OEMInitCoProcRegs: OPTIONAL
//
// This function is called by the kernel when a thread is created to
// initialize the platform-specific debug registers in the debug coprocessor.
//
void OEMInitCoProcRegs(LPBYTE pArea)
{
  // Fill in initialization code here

  return;
}

// ---------------------------------------------------------------------------
// OEMSaveCoProcRegs: OPTIONAL
//
// This function is called by the kernel to save the platform-specific
// debug registers when a thread switch occurs.
//
void OEMSaveCoProcRegs(LPBYTE pArea)
{
  // Fill in save code here

  return;
}

// ---------------------------------------------------------------------------
// OEMRestoreCoProcRegs: OPTIONAL
//
// This function is called by the kernel to restore the platform-specific
// debug registers when a thread switch occurs.
//
void OEMRestoreCoProcRegs(LPBYTE pArea)
{
  // Fill in restore code here

  return;
}


