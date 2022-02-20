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

// ---------------------------------------------------------------------------
// OEMIsFlashAddr: REQUIRED
//
// This function determines whether the passed-in address resides in flash
// memory on the device.  It is used when deciding if the downloaded image
// is to be written to flash or only to RAM.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
BOOL OEMIsFlashAddr(DWORD dwAddr)
{
  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMStartEraseFlash: REQUIRED
//
// This function begins erasing flash at the beginning of the download
// process.  It erases flash so that the Windows CE operating system may be
// placed in flash memory.  If the input address is a bootloader address and
// not an operating system address, this function should do nothing - the
// old bootloader flash region should not be erased until OEMFinishEraseFlash
// when it is certain that a valid new bootloader has been downloaded to RAM
// for placement in flash.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
BOOL OEMStartEraseFlash(DWORD dwStartAddr, DWORD dwLength)
{
  // Fill in flash erase code here.
  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMContinueEraseFlash: REQUIRED
//
// This function erases flash while the download process occurs.  It is called
// periodically by the BLCommon code during the download process, always after
// OEMStartEraseFlash has been called once.  It erases flash so that the
// Windows CE operating system may be placed in flash memory.  If the input
// address to StartErase is a bootloader address and not an operating system
// address, this function should do nothing - the old bootloader flash region
// should not be erased until OEMFinishEraseFlash when it is certain that a
// valid new bootloader has been downloaded to RAM for placement in flash.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
void OEMContinueEraseFlash (void)
{
  return;
}

// ---------------------------------------------------------------------------
// OEMFinishEraseFlash: REQUIRED
//
// This function does any remaining flash erasing necessary after the
// download process occurs and OEMStartEraseFlash and OEMContinueEraseFlash
// have been called.  It erases flash so that the Windows CE operating system
// or a new bootloader may be placed in flash memory.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
BOOL OEMFinishEraseFlash (void)
{
  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMWriteFlash: REQUIRED
//
// This function writes the Windows CE operating system or new bootloader
// image from its temporary RAM location into flash memory.
//
// If hardware does not support booting from flash, this function may be
// implemented as a stub.
//
BOOL OEMWriteFlash(DWORD dwStartAddr, DWORD dwLength)
{
  return TRUE;
}

