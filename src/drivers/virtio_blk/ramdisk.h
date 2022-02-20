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

/*++
Module Name:

    ramdisk.h

Abstract:

    This module contains the function prototypes and constant, type,
    global data and structure definitions for the Windows CE RAM disk driver.

Functions:

Notes:


--*/

#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#include <windows.h>
#include <types.h>
#include <excpt.h>
#include <tchar.h>
#include <devload.h>
#include <diskio.h>
#include <storemgr.h>
#include "virtio_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DISK {
    struct _DISK * d_next;
    CRITICAL_SECTION d_DiskCardCrit;// guard access to global state and card
    DISK_INFO d_DiskInfo;    // for DISK_IOCTL_GET/SETINFO
    LPWSTR d_ActivePath;    // registry path to active key for this device

	DWORD sysintr;
	DWORD ioBase;

	HANDLE interrupt_ev;

	DWORD sectors;
	BOOL readonly;

	void *virt_addr;
	uint32_t meta_phys_addr;

	struct virtq vq;
	uint32_t vq_feat;
	uint16_t last_used_idx;
} DISK, * PDISK;

//
// Global Variables
//
extern CRITICAL_SECTION v_DiskCrit;    // guard access to disk structure list
extern PDISK v_DiskList;


//
// Global functions
//
VOID DoDiskIO(PDISK pDisk, DWORD Opcode, PSG_REQ pSG);
DWORD GetDiskInfo(PDISK pDisk, PDISK_INFO pInfo);
DWORD SetDiskInfo(PDISK pDisk, PDISK_INFO pInfo);
VOID CloseDisk(PDISK pDisk);
BOOL GetFolderName(PDISK pDisk, __out_bcount(cBytes) LPWSTR FolderName, DWORD cBytes, DWORD * pBytesReturned);
BOOL GetDeviceInfo(PDISK pDisk, PSTORAGEDEVICEINFO pInfo);
BOOL RegGetValue(PDISK pDisk, const WCHAR* pwsName, PDWORD pdwValue);

BOOL InitVirtioBlk(PDISK pDisk);
VOID CloseVirtioBlk(PDISK pDisk);
void _dsb(void);

#ifdef DEBUG
//
// Debug zones
//
#define ZONE_ERROR      DEBUGZONE(0)
#define ZONE_WARNING    DEBUGZONE(1)
#define ZONE_FUNCTION   DEBUGZONE(2)
#define ZONE_INIT       DEBUGZONE(3)
#define ZONE_DEPRECATED DEBUGZONE(4)
#define ZONE_IO         DEBUGZONE(5)
#define ZONE_MISC       DEBUGZONE(6)

#endif  // DEBUG

#ifdef __cplusplus
}
#endif

#endif // _RAMDISK_H_

