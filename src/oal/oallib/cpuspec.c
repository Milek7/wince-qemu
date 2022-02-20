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

// This file contains required functions and variables that are specific
// to a CPU and required before the OEMGlobal table is available.  Therefore
// their names are hard-coded and they must be implemented for their
// respective CPU architectures.

// ---------------------------------------------------------------------------
// OEMARMCacheMode: REQUIRED (ARM CPUs only)
//
// This function sets the cache mode used to build the ARM CPU page tables.
// It sets the value of g_pOemGlobal->dwARMCacheMode before the OemGlobal
// table is initialized.
//
DWORD OEMARMCacheMode(void)
{
  return 0x0C;
}

