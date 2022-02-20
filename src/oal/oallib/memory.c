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
// OEMGetOEMRamTable: OPTIONAL
//
// This function defines the OEMRamTable, which specifies the RAM available
// for the platform (in addition to the RAM in OEMAddressTable for x86/ARM).
// Implementing this function allows your platform to support more than
// 512MB of physical memory.
//
PCRamTable OEMGetOEMRamTable(void)
{
  // Fill in memory code here.
	DEBUGMSG(1, (TEXT("OEMGetOEMRamTable\r\n")));

  return NULL;
}

// ---------------------------------------------------------------------------
// OEMGetExtensionDRAM: REQUIRED
//
// This function returns information about extension dynamic RAM (DRAM), if
// present on the device.
//
BOOL OEMGetExtensionDRAM(LPDWORD lpMemStart, LPDWORD lpMemLen)
{
  // Fill in extension DRAM code here.
	DEBUGMSG(1, (TEXT("OEMGetExtensionDRAM\r\n")));

  // False indicates no extension DRAM is present, which is a valid state.
  return FALSE; 
}

// ---------------------------------------------------------------------------
// OEMEnumExtensionDRAM: OPTIONAL
//
// This function returns information about extension dynamic RAM (DRAM),
// if present on the device.  It can describe multiple memory sections and
// if it is implemented (and mapped) it will be called in place of
// OEMGetExtensionDRAM.
//
DWORD OEMEnumExtensionDRAM(PMEMORY_SECTION pMemSections, DWORD dwMemSections)
{
  // Fill in extension DRAM code here.
	DEBUGMSG(1, (TEXT("EnumExtensionDRAM\r\n")));

  return 0; 
}

// ---------------------------------------------------------------------------
// OEMCalcFSPages: OPTIONAL
//
// This functions returns the amount of pages the kernel should use for
// the object store (RAM based filesystem).  Similar results can be
// accomplished by adjusting FSRAMPERCENT in config.bib.
//
DWORD OEMCalcFSPages(DWORD dwMemPages, DWORD dwDefaultFSPages)
{
	DEBUGMSG(1, (TEXT("OEMCalcFSPages\r\n")));
  // Fill in page calculation code here.

  return dwDefaultFSPages;
}

// ---------------------------------------------------------------------------
// OEMReadRegistry: OPTIONAL
//
// This functions reads a registry file into RAM from persistent storage.
//
DWORD OEMReadRegistry(DWORD dwFlags, LPBYTE pBuf, DWORD len)
{
  // Fill in registry code here.
	DEBUGMSG(1, (TEXT("OEMReadRegistry\r\n")));

  return 0;
}

// ---------------------------------------------------------------------------
// OEMWriteRegistry: OPTIONAL
//
// This functions writes a registry file to persistent storage.
//
BOOL OEMWriteRegistry (DWORD dwFlags, LPBYTE pBuf, DWORD len)
{
  // Fill in registry code here.
	DEBUGMSG(1, (TEXT("OEMWriteRegistry\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMIsRom: OPTIONAL
//
// This function determines whether a given address range falls within a
// valid range of ROM addresses.
//
BOOL OEMIsRom(DWORD dwShiftedPhysAddr)
{
  // Fill in memory code here.
DEBUGMSG(1, (TEXT("OEMIsRom\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMSetMemoryAttributes: OPTIONAL
//
// This function handles changes to memory attributes.
//
BOOL OEMSetMemoryAttributes(LPVOID pVirtualAddr, LPVOID pPhysAddr, DWORD cbSize, DWORD dwAttributes)
{
	DEBUGMSG(1, (TEXT("OEMSetMemoryAttributes\r\n")));
  // Fill in memory code here

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMNotifyForceCleanBoot: OPTIONAL
//
// This function is called by the kernel when the filesystem is identified
// as corrupted.  The call occurs before a clean system boot is forced.
//
void OEMNotifyForceCleanBoot(DWORD dwArg1, DWORD dwArg2, DWORD dwArg3, DWORD dwArg4)
{
  // Fill in cleanup code here.
	DEBUGMSG(1, (TEXT("OEMNotifyForceCleanBoot\r\n")));

  return;
}


