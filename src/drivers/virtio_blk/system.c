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

#include <ramdisk.h>
#ifdef DEBUG
//
// These defines must match the ZONE_* defines
//
#define DBG_ERROR      1
#define DBG_WARNING    2
#define DBG_FUNCTION   4
#define DBG_INIT       8
#define DBG_DEPRECATED 16
#define DBG_IO         32

DBGPARAM dpCurSettings = {
    TEXT("virtio_blk"), {
    TEXT("Errors"),TEXT("Warnings"),TEXT("Functions"),TEXT("Initialization"),
    TEXT("Deprecated"),TEXT("Disk I/O"),TEXT("Misc"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
    0x1
};
#endif  // DEBUG


//
// Global Variables
//
CRITICAL_SECTION v_DiskCrit;
PDISK v_DiskList;                // initialized to 0 in bss

BOOL WINAPI
DllMain(HINSTANCE DllInstance
      , DWORD Reason
      , LPVOID Reserved
      )
{
    UNREFERENCED_PARAMETER(Reserved);
    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: DLL_PROCESS_ATTACH\r\n")));
            DEBUGREGISTER(DllInstance);
            DisableThreadLibraryCalls((HMODULE) DllInstance);
            break;

        case DLL_PROCESS_DETACH:
            DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: DLL_PROCESS_DETACH\r\n")));
            DeleteCriticalSection(&v_DiskCrit);
            break;
    }
    return TRUE;
}   // DllMain


//------------------------------------------------------------------------------
//
// CreateDiskObject - create a DISK structure, init some fields and link it.
//
//------------------------------------------------------------------------------
PDISK
CreateDiskObject(VOID)
{
    PDISK pDisk;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+CreateDiskObject\r\n")));

    pDisk = LocalAlloc(LPTR, sizeof(DISK));
    if (pDisk != NULL) {
        pDisk->d_ActivePath = NULL;
        InitializeCriticalSection(&(pDisk->d_DiskCardCrit));
        EnterCriticalSection(&v_DiskCrit);
        pDisk->d_next = v_DiskList;
        v_DiskList = pDisk;
        LeaveCriticalSection(&v_DiskCrit);
    }

    DEBUGMSG(ZONE_FUNCTION, (TEXT("-CreateDiskObject\r\n")));
    return pDisk;
}   // CreateDiskObject



//------------------------------------------------------------------------------
//
// IsValidDisk - verify that pDisk points to something in our list
//
// Return TRUE if pDisk is valid, FALSE if not.
//
//------------------------------------------------------------------------------
BOOL
IsValidDisk(
    PDISK pDisk
    )
{
    PDISK pd;
    BOOL ret = FALSE;

    EnterCriticalSection(&v_DiskCrit);
    pd = v_DiskList;
    while (pd) {
        if (pd == pDisk) {
            ret = TRUE;
            break;
        }
        pd = pd->d_next;
    }
    LeaveCriticalSection(&v_DiskCrit);
    return ret;
}   // IsValidDisk



//------------------------------------------------------------------------------
//
// Returns context data for this Init instance
//
// Arguments:
//      dwContext - registry path for this device's active key
//
//------------------------------------------------------------------------------
DWORD
DSK_Init(
    DWORD dwContext
    )
{
    PDISK pDisk;
    const WCHAR* ActivePath = (const WCHAR*) dwContext;
    DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: DSK_Init entered\r\n")));

    if (v_DiskList == NULL) {
        InitializeCriticalSection(&v_DiskCrit);
    }

    pDisk = CreateDiskObject();

    if (pDisk == NULL) {
        RETAILMSG(1,(TEXT("virtio_blk: LocalAlloc(PDISK) failed %d\r\n"), GetLastError()));
        return 0;
    }

    if (ActivePath) {
        size_t maximumPathElements = wcslen(ActivePath)+1;
        DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk : ActiveKey = %s\r\n"), ActivePath));
        pDisk->d_ActivePath = LocalAlloc(LPTR, maximumPathElements * sizeof(WCHAR));
        if (pDisk->d_ActivePath) {
            if (FAILED(StringCchCopy( pDisk->d_ActivePath, maximumPathElements, ActivePath )))
            {
                LocalFree(pDisk->d_ActivePath);
                pDisk->d_ActivePath = NULL;
            }
        }
        DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk : ActiveKey (copy) = %s (@ 0x%08X)\r\n"), pDisk->d_ActivePath, pDisk->d_ActivePath));
    }

    if (!RegGetValue (pDisk, TEXT("IoBase"), &pDisk->ioBase) ||
		!RegGetValue (pDisk, TEXT("SysIntr"), &pDisk->sysintr) ||
		!InitVirtioBlk(pDisk)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: failed to init device\r\n")));
        if (pDisk->d_ActivePath){
            LocalFree(pDisk->d_ActivePath);
        }
        LocalFree(pDisk);
        return 0;
    }

    pDisk->d_DiskInfo.di_total_sectors = pDisk->sectors;
    pDisk->d_DiskInfo.di_bytes_per_sect = 512;
    pDisk->d_DiskInfo.di_cylinders = 0;
    pDisk->d_DiskInfo.di_heads = 0;
    pDisk->d_DiskInfo.di_sectors = 0;
    pDisk->d_DiskInfo.di_flags = DISK_INFO_FLAG_CHS_UNCERTAIN;

    DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: sectors = %d\r\n"), pDisk->d_DiskInfo.di_total_sectors));

    DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: Init returning 0x%x\r\n"), pDisk));
    return (DWORD) pDisk;

}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
DSK_Close(
    DWORD Handle
    )
{
    PDISK pDisk = (PDISK)Handle;

    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: DSK_Close entered\r\n")));

    if (!IsValidDisk(pDisk)) {
        return FALSE;
    }

    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: DSK_Close done\r\n")));
    return TRUE;
}   // DSK_Close


//------------------------------------------------------------------------------
//
// Device deinit - devices are expected to close down.
// The device manager does not check the return code.
//
//------------------------------------------------------------------------------
BOOL
DSK_Deinit(
    DWORD dwContext     // future: pointer to the per disk structure
    )
{
    DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: DSK_Deinit entered\r\n")));
    DSK_Close(dwContext);
    CloseDisk((PDISK)dwContext);
    DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: DSK_Deinit done\r\n")));
    return TRUE;
}   // DSK_Deinit


//------------------------------------------------------------------------------
//
// Returns handle value for the open instance.
//
//------------------------------------------------------------------------------
DWORD
DSK_Open(
      DWORD dwData
    , DWORD dwAccess
    , DWORD dwShareMode
    )
{
    DISK* pDisk = (DISK*)dwData;
    UNREFERENCED_PARAMETER(dwAccess);
    UNREFERENCED_PARAMETER(dwShareMode);

    if (IsValidDisk(pDisk) == FALSE) {
        DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: DSK_Open - Passed invalid disk handle\r\n")));
        return 0;
    }

    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: DSK_Open returning %d\r\n"),dwData));
    return dwData;
}   // DSK_Open


//------------------------------------------------------------------------------
//
// I/O Control function - responds to info, read and write control codes.
// The read and write take a scatter/gather list in pInBuf
//
//------------------------------------------------------------------------------
BOOL
DSK_IOControl(
    DWORD Handle,
    DWORD dwIoControlCode,
    __in_bcount_opt(nInBufSize) PBYTE pInBuf,
    DWORD nInBufSize,
    __out_bcount_opt(nOutBufSize) PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    PSG_REQ pSG;
    PDISK pDisk = (PDISK) Handle;

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DSK_IOControl (%d) \r\n"), dwIoControlCode));

    if (IsValidDisk(pDisk) == FALSE) {
        SetLastError(ERROR_INVALID_HANDLE);
        DEBUGMSG(ZONE_FUNCTION, (TEXT("-DSK_IOControl (invalid disk) \r\n")));
        return FALSE;
    }

    //
    // Check parameters
    //
    switch (dwIoControlCode) {
    case DISK_IOCTL_READ:
    case IOCTL_DISK_READ:
    case DISK_IOCTL_WRITE:
    case IOCTL_DISK_WRITE:
    case DISK_IOCTL_INITIALIZED:
        if (pInBuf == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case DISK_IOCTL_GETNAME:
    case IOCTL_DISK_GETNAME:
        if (pOutBuf == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case IOCTL_DISK_GETINFO:
        if (pOutBuf == NULL || nOutBufSize != sizeof(DISK_INFO))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case DISK_IOCTL_GETINFO:
    case DISK_IOCTL_SETINFO:
    case IOCTL_DISK_SETINFO:
        if(pInBuf == NULL || nInBufSize != sizeof(DISK_INFO))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;


    case IOCTL_DISK_DEVICE_INFO:
        if(!pInBuf || nInBufSize != sizeof(STORAGEDEVICEINFO)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case IOCTL_DISK_GET_SECTOR_ADDR:
        if(pInBuf == NULL || pOutBuf == NULL || nInBufSize != nOutBufSize) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case IOCTL_DISK_DELETE_SECTORS:
        if(!pInBuf || nInBufSize != sizeof(DELETE_SECTOR_INFO)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        break;

    case DISK_IOCTL_FORMAT_MEDIA:
    case IOCTL_DISK_FORMAT_MEDIA:
        SetLastError(ERROR_SUCCESS);
        return TRUE;

    default:
		DEBUGMSG(ZONE_FUNCTION, (TEXT("-DSK_IOControl (check default) \r\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Execute dwIoControlCode
    //
    switch (dwIoControlCode) {
    case DISK_IOCTL_READ:
    case IOCTL_DISK_READ:
    case DISK_IOCTL_WRITE:
    case IOCTL_DISK_WRITE:
        pSG = (PSG_REQ)pInBuf;
        if (!(pSG && nInBufSize >= (sizeof(SG_REQ) + sizeof(SG_BUF) * (pSG->sr_num_sg - 1)))) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        DoDiskIO(pDisk, dwIoControlCode, pSG);
        if (pSG->sr_status) {
            SetLastError(pSG->sr_status);
            return FALSE;
        }
        return TRUE;

    case DISK_IOCTL_GETINFO:
    case IOCTL_DISK_GETINFO:
        SetLastError(GetDiskInfo(pDisk, (PDISK_INFO)(dwIoControlCode == IOCTL_DISK_GETINFO ? pOutBuf : pInBuf)));
        return TRUE;

    case DISK_IOCTL_SETINFO:
    case IOCTL_DISK_SETINFO:
        SetLastError(SetDiskInfo(pDisk, (PDISK_INFO)pInBuf));
        return TRUE;

    case DISK_IOCTL_INITIALIZED:
        return TRUE;

    case DISK_IOCTL_GETNAME:
    case IOCTL_DISK_GETNAME:
        DEBUGMSG(ZONE_FUNCTION, (TEXT("-DSK_IOControl (name) \r\n")));
        return GetFolderName(pDisk, (LPWSTR)pOutBuf, nOutBufSize, pBytesReturned);

    case IOCTL_DISK_DEVICE_INFO: // new ioctl for disk info
        DEBUGMSG(ZONE_FUNCTION, (TEXT("-DSK_IOControl (device info) \r\n")));
        return GetDeviceInfo(pDisk, (PSTORAGEDEVICEINFO)pInBuf);

	case IOCTL_DISK_DELETE_SECTORS:
	case IOCTL_DISK_GET_SECTOR_ADDR:
    default:
        DEBUGMSG(ZONE_FUNCTION, (TEXT("-DSK_IOControl (default) \r\n")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}   // DSK_IOControl


DWORD DSK_Read(DWORD Handle, LPVOID pBuffer, DWORD dwNumBytes)
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: DSK_Read")));
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(dwNumBytes);
    return 0;
}
DWORD DSK_Write(DWORD Handle, LPCVOID pBuffer, DWORD dwNumBytes)
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: DSK_Write")));
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(dwNumBytes);
    return 0;
}
DWORD DSK_Seek(DWORD Handle, long lDistance, DWORD dwMoveMethod)
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: DSK_Seek")));
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(lDistance);
    UNREFERENCED_PARAMETER(dwMoveMethod);
    return 0;
}
void DSK_PowerUp(void){}
void DSK_PowerDown(void){}


