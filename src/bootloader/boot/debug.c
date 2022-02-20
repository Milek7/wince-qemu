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
#include <nkintr.h>

// ---------------------------------------------------------------------------
// OEMWriteDebugByte: REQUIRED
//
// This function writes a byte to the debug peripheral that was initialized
// in OEMDebugInit.
//
void OEMWriteDebugByte(BYTE ch)
{
  // Fill in peripheral write code here.
  return;
}

// ---------------------------------------------------------------------------
// OEMReadDebugByte: OPTIONAL
//
// This function reads a byte from the debug peripheral that was initialized
// in OEMDebugInit.  It is optional with respect to the BLCommon core but if
// any user interaction is desired, it is required.  The most common user
// interaction is through a menu displayed to the user over the debug
// peripheral.
//
int OEMReadDebugByte (void)
{
  // Fill in peripheral read code here.
  return OEM_DEBUG_READ_NODATA;
}
