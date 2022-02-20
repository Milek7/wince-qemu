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
#include <bootCore.h>

// Declaration of core library BootStart function for use by
// BootloaderEntryPoint
extern void BootStart(VOID);

//------------------------------------------------------------------------------
// BootStart: OPTIONAL
//
// This function is a stub for the CE Boot Framework entry point which will be
// called by the bootloader entry point.  It initializes caches and (optionally)
// the MMU and jumps to the BootMain portion of the CE Boot Framework.
//
// You must remove this stub function and stub pTOC declartion.  In its place
// you will use the BootStart function provided by the CE Boot Framework's core
// library specific to your CPU.  See the sources file in this directory for
// information on how to link with the core library.
//
// Remove these includes
#include <pehdr.h>
#include <romldr.h>
// Remove this declaration
ROMHDR* volatile const pTOC = (ROMHDR *)-1; 
void BootStart(VOID)
{
  // Remove this function the and link with the appropriate CE Boot Framework
  // library.
}

//------------------------------------------------------------------------------
// BootloaderEntryPoint: REQUIRED
//
// This function jumps to the CPU-specific CE Boot Framework startup function
// BootStart.
//
// It must do any hardware initialization necessary to perform this jump.
// This function should be implemented in assembly language but is demonstrated
// here in C so as to be compilable for all CPUs.
//
// This function can be renamed but the name must match the entry point
// definition in the sources file.
//
void BootloaderEntryPoint(VOID)
{
  // Replace the below with assembly init and jump code.
  BootStart();
}
