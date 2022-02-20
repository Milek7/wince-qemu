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
#include <BootCore.h>

// Bldr.c
// The comments in this file will vary from OS version to version.
// This file demonstrates the functions necessary to make a bootloader
// using the CE Boot Framework.
//
// All functions in the template bootloader fall into one of two categories:
// REQUIRED - you must implement this function for a CE Boot Framework
// compatible bootloader
// OPTIONAL - you may implement this function to enable specific functionality
//
// See the CE Boot Framework core header BootCore.h for more details on these
// functions. The function names and signatures must match the CE Boot
// Framework headers.  Therefore, you cannot change the names / signatures in
// this file.
//

//------------------------------------------------------------------------------
// OEMBootInit: REQUIRED
//
// This function initializes hardware used by the bootloader (typically
// timer, serial port, any muxing) as well as any global context the
// bootloader will use.  The CE Boot framework calls this function after it 
// initializes global variables, system heap and (optionally) virtual memory.
//
void* OEMBootInit(VOID)
{
  // Fill in init code here.

  return NULL;
}

//------------------------------------------------------------------------------
// OEMBootLoad: REQUIRED
//
// This function places the CE operating system into memory and then returns
// BOOT_STATE_RUN.  The CE Boot Framework will call this function repeatedly
// until BOOT_STATE_RUN is returned, allowing this function to act as a state
// machine.
//
enum_t OEMBootLoad(void *pContext, enum_t state)
{
  // Fill in loader code here.

  return BOOT_STATE_RUN;
}

//------------------------------------------------------------------------------
// OEMBootRun: REQUIRED
//
// This function prepares the hardware to run the CE operating system image and
// returns the physical starting address of the image's entry point.
//
uint32_t OEMBootRun(void *pContext)
{
  uint32_t physAddress = 0;

  // Fill in init code here.

  return physAddress;
}

//------------------------------------------------------------------------------
// OEMBootPowerOff: REQUIRED
//
// This function powers down or resets the device in response to a boot failure.
// The CE Boot Framework calls this function when there is an unrecoverable
// error.
//
void OEMBootPowerOff(void *pContext)
{
  // Fill in power code here.

  return;
}

//------------------------------------------------------------------------------
// OEMBootGetTickCount: OPTIONAL
//
// This function returns the system time with 1ms resolution, in most cases the
// since device power on.  The CE Boot Framework does not call this function,
// but it is often useful for implementing specific bootloader or boot driver
// functionality.
//
uint32_t OEMBootGetTickCount(VOID)
{
  // Fill in timer code here.

  return 0;
}

//------------------------------------------------------------------------------
// OEMBootStall: OPTIONAL
//
// This function delays execution for the input number of microseconds.  It is
// typically implemented with a busy loop.  The CE Boot Framework does not call
// this function, but it is often useful for implementing specific bootloader
// or boot driver functionality.
void OEMBootStall(uint32_t delay)
{
  // Fill in stall code here.

  return;
}


