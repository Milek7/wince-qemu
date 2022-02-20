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
#include <pehdr.h>
#include <romldr.h>
#include <blcommon.h>

// Main.c
// The comments in this file will vary from OS version to version.
// This file demonstrates the functions necessary to make a bootloader
// using the BLCommon framework.
//
// All functions in the template bootloader fall into one of two categories:
// REQUIRED - you must implement this function for a BLCommon framework
// compatible bootloader
// OPTIONAL - you may implement this function to enable specific functionality
//
// See the BLCommon header public\common\oak\inc\blcommon.h for more details
// on these functions functions. The function names and signatures must match
// the BLCommon headers.  Therefore, you cannot change the names / signatures
// in this file.
//

// Declare optional functions
BOOL OEMCheckSignature(DWORD dwImageStart, DWORD dwROMOffset,
                       DWORD dwLaunchAddr, BOOL bDownloaded);
BOOL OEMVerifyMemory(DWORD dwStartAddr, DWORD dwLength);
void OEMMultiBINNotify(const PMultiBINInfo pInfo);
BOOL OEMReportError(DWORD dwReason, DWORD dwReserved);

// ---------------------------------------------------------------------------
// OEMDebugInit: REQUIRED
//
// This function is the first major function called by the BLCommon code.  It
// initializes any debug peripherals (typically just a single UART) that the
// bootloader will use to output information during its execution.  The
// BLCommon code will call the OEMWriteDebugByte and OEMReadDebugByte
// functions to interact with this debug peripheral.  OEMDebugInit also
// initializes function pointers to optional bootloader functions.
//
BOOL OEMDebugInit (void)
{
  // Initialize optional functions
  g_pOEMCheckSignature = OEMCheckSignature;
  g_pOEMVerifyMemory = OEMVerifyMemory;
  g_pOEMReportError = OEMReportError;
  g_pOEMMultiBINNotify = OEMMultiBINNotify;

  // Fill in debug init code here.
  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMPlatformInit: REQUIRED
//
// This function is the second major function called by the BLCommon code.  It
// initializes the hardware components necessary for the bootloader to run.
// When this function is called, the Startup function will have initialized
// the CPU and memory that the bootloader is executing from.  The OEMDebugInit
// function will have initialized any debug peripherals that will be used by
// the bootloader.
//
// The OEMPlatformInit function is responsible for initializing all other
// hardware necessary for the bootloader to run.  This typically includes:
// - Shared memory area for storing bootloader arguments to the CE operating
// system
// - Flash memory or flash memory controller
// - Transport hardware: UART, USB, network, or other device that will
// download the Windows CE operating system from a development PC
// - Parent bus support for any transport hardware on a bus
// - Real-Time Clock (for use with network timeouts)
// - Any DMA that will be used during bootloader execution
// - Any pin muxing or GPIO configuration necessary for bootloader execution
//
BOOL OEMPlatformInit (void)
{
  // Fill in hardware init code here
  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMPreDownload: REQUIRED
//
// This function is the third major function called by the BLCommon code.  It
// prepares the system to download an image of the Windows CE operating system
// over the transport hardware.  It generates a hardware platform-specific
// idenfitifer for the device that will be used to identify the device to
// Platform Builder on the host PC.  It also does any necessary configuration
// of the transport hardware, such as obtaining an IP address for network
// device transport hardware.  Finally, it initializes the Platform Builder
// download protocol; often TFTP is used.
//
DWORD OEMPreDownload (void)
{
  // Fill in download prep code here
  return 0;
}


// ---------------------------------------------------------------------------
// OEMLaunch: REQUIRED
//
// This function is the fourth and final major function called by the BLCommon
// code.  This function receives operating system settings from Platform
// Builder, makes any necessary changes to the bootloader arguments to the
// operating system, then jumps to the address of the operating system in
// memory.
//
void OEMLaunch(DWORD dwImageStart, DWORD dwImageLength, DWORD dwLaunchAddr,
               const ROMHDR* pRomHdr)
{
  // Fill in jump code here

  // This function should never return!
  return;
}

// ---------------------------------------------------------------------------
// OEMMemMapAddr: REQUIRED
//
// This function maps a flash address to a RAM address where the Windows CE
// operating system can be temporarily stored during the download process.
// Since the download transport is typically faster than flash writes, storing
// the operating system in RAM is necessary so that the download process does
// not stall.  Also, storing the image in RAM first allows for validation that
// the download completed successfully before flash writes begin.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
LPBYTE OEMMapMemAddr(DWORD dwImageStart, DWORD dwAddr)
{
  // Fill in address mapping code here
  return 0;
}

// ---------------------------------------------------------------------------
// OEMReadData: REQUIRED
//
// This function reads data from the transport hardware during the download\
// process.  
//
BOOL OEMReadData(DWORD cbData, LPBYTE pbData)
{
  // Fill in read code here

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMShowProgress: REQUIRED
//
// This function is called during the download process to indicate the
// download is occurring.  It can light an LED, send information to a
// debug peripheral, or just be implemented as a stub.
//
void OEMShowProgress(DWORD dwPacketNum)
{
  // Fill in indicator code here

  return;
}

// ---------------------------------------------------------------------------
// OEMCheckSignature: OPTIONAL
//
// This function verifies the signature of a downloaded Windows CE operating
// system or bootloader.  It the signature is correct, then it is safe to
// write the downloaded OS or bootloader to flash.
//
BOOL OEMCheckSignature(DWORD dwImageStart, DWORD dwROMOffset,
                       DWORD dwLaunchAddr, BOOL bDownloaded)
{
  // Fill in signature check code here.

  return FALSE;
}

// ---------------------------------------------------------------------------
// OEMVerifyMemory: OPTIONAL
//
// This function verifies the memory where the downloaded Windows CE
// operating system or bootloader will be placed is valid.  It checks that
// the RAM or flash memory specified is physically available on the system.
//
BOOL OEMVerifyMemory(DWORD dwStartAddr, DWORD dwLength)
{
  // Fill in memory check code here.

  return FALSE;
}

// ---------------------------------------------------------------------------
// OEMMultiBINNotify: OPTIONAL
//
// This function prepares downloading of multiple files.  The number of files
// and their addresses are necessary to successfully cache all the data in
// RAM for a flash write.  If multiple NB0 (binary) files are being
// downloaded, this function also determines their placement in memory.
//
void OEMMultiBINNotify(const PMultiBINInfo pInfo)
{
  // Fill in multi-file download preparation code here

  return;
}

// ---------------------------------------------------------------------------
// OEMReportError: OPTIONAL
//
// This function is called when a critical error occurs in the bootloader,
// causing it to halt.  It is the last code that executes before the system
// is halted.  It can be used to send a final message to the debug peripheral.
//
BOOL OEMReportError(DWORD dwReason, DWORD dwReserved)
{
  return FALSE;
}
