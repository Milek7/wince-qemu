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
#include <kitl.h>

// Init.c
// The comments in this file will vary from OS version to version.
//
// All KITL functions in the template BSP fall into one of three categories:
// REQUIRED - you must implement this function for KITL functionality
// OPTIONAL - you may implement this function to enable specific functionality
// CUSTOM   - this function is a helper function specific to this BSP
//

//------------------------------------------------------------------------------
//
// OEMKitlStartup: REQUIRED
//
// This function is the first OEM code that executes in kitl.dll.  It is called
// by the kernel after the BSP calls KITLIoctl( IOCTL_KITL_STARTUP, ... ) in
// OEMInit().  This function should set up internal state, read any boot
// arguments, and call the KitlInit function to initialize KITL in active
// (immediate load) or passive (delay load) mode.
//
BOOL OEMKitlStartup()
{
  BOOL mode = FALSE; // TRUE for active mode, FALSE for passive mode

  // Fill in startup code here.

  return KitlInit(mode);
}

// ---------------------------------------------------------------------------
// OEMKitlInit: REQUIRED
//
// This function is called from the kernel to initialize the KITL device and
// KITLTRANSPORT structure when it is time to load KITL.  If OEMKitlStartup
// selects active mode KITL, KitlInit will call this function during boot. If
// OEMKitlStartup selects passive mode KITL, this function will not be called
// until an event triggers KITL load.

// When this function returns, the KITLTRANSPORT structure must contain valid
// variable initializations including valid function pointers for each
// required KITL function.  The KITL transport hardware must also be fully
// initialized.
//
BOOL OEMKitlInit(PKITLTRANSPORT pKitl)
{
  // Fill in init code here.

  return TRUE;
}

// ---------------------------------------------------------------------------
// dpCurSettings: REQUIRED
//
// This variable defines debug zones usable by the kernel and this
// implementation.  This is the operating system's standard
// mechanism for debug zones.
//
DBGPARAM dpCurSettings = {
    TEXT("KITL"), {
    TEXT("Warning"),    TEXT("Init"),       TEXT("Frame Dump"),     TEXT("Timer"),
    TEXT("Send"),       TEXT("Receive"),    TEXT("Retransmit"),     TEXT("Command"),
    TEXT("Interrupt"),  TEXT("Adapter"),    TEXT("LED"),            TEXT("DHCP"),
    TEXT("OAL"),        TEXT("Ethernet"),   TEXT("Unused"),         TEXT("Error"), },
    ZONE_WARNING | ZONE_INIT | ZONE_ERROR,
};
