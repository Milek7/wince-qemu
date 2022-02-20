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

extern void BootloaderMain();

// ---------------------------------------------------------------------------
// StartUp: REQUIRED
//
// This function is the first function that the bootloader executes.  It is
// the entry point of boot.exe.  Traditionally it is named StartUp, but its
// name can be changed provided it matches the EXEENTRY in the SOURCES file
// where boot.exe is linked.
// 
// This function is repsonsible for initializing the CPU and jumping to the
// BLCommon function BootloaderMain.
//
// Because StartUp implements the lowest level of initialization before the
// MMU is initialized, it is typically implemented in assembly code.  It is
// common for StartUp to be implemented in an assembly file such as startup.s
// or startup.asm.  You should replace this file with such a file and modify
// the SOURCES file in this directory to match.
//
void StartUp(void)
{
  // replace this file with assembly code that initializes the CPU and
  // jumps to BootloaderMain
  BootloaderMain();
}
