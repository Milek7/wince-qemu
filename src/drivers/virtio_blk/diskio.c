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

#define REG_PROFILE_KEY     TEXT("Profile")
#define DEFAULT_PROFILE     TEXT("Default")


//------------------------------------------------------------------------------
//
// Function to open the driver key specified by the active key
//
// The caller is responsible for closing the returned HKEY
//
//------------------------------------------------------------------------------
HKEY
OpenDriverKey(
    const TCHAR* ActiveKey
    )
{
    TCHAR DevKey[256];
    HKEY hDevKey;
    HKEY hActive;
    DWORD ValType;
    DWORD ValLen;
    DWORD status;

    //
    // Get the device key from active device registry key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                ActiveKey,
                0,
                0,
                &hActive);
    if (status) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("RAM:OpenDriverKey RegOpenKeyEx(HLM\\%s) returned %d!!!\r\n"),
                  ActiveKey, status));
        return NULL;
    }

    hDevKey = NULL;

    ValLen = sizeof(DevKey);
    status = RegQueryValueEx(
                hActive,
                DEVLOAD_DEVKEY_VALNAME,
                NULL,
                &ValType,
                (PUCHAR)DevKey,
                &ValLen);
    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("RAM:OpenDriverKey - RegQueryValueEx(%s) returned %d\r\n"),
                  DEVLOAD_DEVKEY_VALNAME, status));
        goto odk_fail;
    }

    //
    // Get the geometry values from the device key
    //
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                DevKey,
                0,
                0,
                &hDevKey);
    if (status) {
        hDevKey = NULL;
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("RAM:OpenDriverKey RegOpenKeyEx - DevKey(HLM\\%s) returned %d!!!\r\n"),
                  DevKey, status));
    }

odk_fail:
    RegCloseKey(hActive);
    return hDevKey;
}   // OpenDriverKey



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL
RegGetValue(
    PDISK pDisk,
    const WCHAR* pwsName,
    PDWORD pdwValue
    )
{
    HKEY DriverKey;
    DWORD ValType;
    DWORD status;
    DWORD dwBytes;

    DriverKey = OpenDriverKey(pDisk->d_ActivePath);
    if (!DriverKey) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR, (TEXT("RAM:GetRegValue - Could not open registry key %s\r\n"), pDisk->d_ActivePath));
        return FALSE;
    }

    dwBytes = sizeof(DWORD);
    status = RegQueryValueEx(DriverKey, pwsName, NULL, &ValType, (PUCHAR)pdwValue, &dwBytes);

    if (status != ERROR_SUCCESS) {
        DEBUGMSG(ZONE_INIT|ZONE_ERROR,
            (TEXT("RAM: No %s entry specified in the registry\r\n"), pwsName));
        RegCloseKey(DriverKey);
        return FALSE;
    }

    DEBUGMSG(ZONE_INIT,(TEXT("RAM:%s registry value = %d\r\n"), pwsName, *pdwValue));
    RegCloseKey(DriverKey);
    return TRUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL GetDeviceInfo(PDISK pDisk, PSTORAGEDEVICEINFO pInfo)
{
    BOOL fRet = FALSE;
    HKEY hKeyDriver;
    DWORD dwBytes;
    DWORD dwStatus;
    DWORD dwValType;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: GetDeviceInfo")));

    if(pDisk && pInfo)
    {
        if( SUCCEEDED(StringCchCopy( pInfo->szProfile, _countof(pInfo->szProfile), DEFAULT_PROFILE)) )
        {
            __try
            {
                hKeyDriver = OpenDriverKey(pDisk->d_ActivePath);
                if(hKeyDriver)
                {
                    // read the profile string from the active reg key if it exists
                    dwBytes = sizeof(pInfo->szProfile);
                    dwStatus = RegQueryValueEx(hKeyDriver, REG_PROFILE_KEY, NULL, &dwValType,
                        (PBYTE)&pInfo->szProfile, &dwBytes);
                    RegCloseKey (hKeyDriver);

                    // if this fails, szProfile will contain the default string
                }

                // set our class, type, and flags
                pInfo->dwDeviceClass = STORAGE_DEVICE_CLASS_BLOCK;
                pInfo->dwDeviceType = STORAGE_DEVICE_TYPE_UNKNOWN;
				pInfo->dwDeviceFlags = pDisk->readonly ? STORAGE_DEVICE_FLAG_READONLY : STORAGE_DEVICE_FLAG_READWRITE;
                fRet = TRUE;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                fRet = FALSE;
            }
        }
    }
    return fRet;
}

//------------------------------------------------------------------------------
//
// Function to retrieve the folder name value from the driver key. The folder name is
// used by FATFS to name this disk volume.
//
//------------------------------------------------------------------------------
BOOL
GetFolderName(
    PDISK pDisk,
    __out_bcount(cBytes) LPWSTR FolderName,
    DWORD cBytes,
    DWORD * pcBytes
    )
{
    HKEY DriverKey;
    DWORD ValType;
    DWORD status;

	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: GetFolderName")));

    DriverKey = OpenDriverKey(pDisk->d_ActivePath);
    if (DriverKey) {
        *pcBytes = cBytes;
        status = RegQueryValueEx(
                    DriverKey,
                    TEXT("Folder"),
                    NULL,
                    &ValType,
                    (PUCHAR)FolderName,
                    pcBytes);
        if (status != ERROR_SUCCESS) {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR,
                (TEXT("RAM:GetFolderName - RegQueryValueEx(Folder) returned %d\r\n"),
                      status));
            *pcBytes = 0;
        } else {
            DEBUGMSG(ZONE_INIT|ZONE_ERROR,
                (TEXT("RAM:GetFolderName - FolderName = %s, length = %d\r\n"),
                 FolderName, *pcBytes));
           *pcBytes += sizeof(WCHAR); // account for terminating 0.
        }
        RegCloseKey(DriverKey);
        if (status || (*pcBytes == 0)) {
            // Use default
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}



//------------------------------------------------------------------------------
//
// CloseDisk - free all resources associated with the specified disk
//
//------------------------------------------------------------------------------
VOID
CloseDisk(
    PDISK pDisk
    )
{
    PDISK pd;

    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: CloseDisk closing 0x%x\r\n"), pDisk));

    //
    // Remove it from the global list of disks
    //
    EnterCriticalSection(&v_DiskCrit);
    if (pDisk == v_DiskList) {
        v_DiskList = pDisk->d_next;
    } else {
        pd = v_DiskList;
        while (pd->d_next != NULL) {
            if (pd->d_next == pDisk) {
                pd->d_next = pDisk->d_next;
                break;
            }
            pd = pd->d_next;
        }
    }
    LeaveCriticalSection(&v_DiskCrit);

    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: CloseDisk - freeing resources\r\n")));

    //
    // Try to ensure this is the only thread holding the disk crit sec
    //
    Sleep(50);
    EnterCriticalSection(&(pDisk->d_DiskCardCrit));
    LeaveCriticalSection(&(pDisk->d_DiskCardCrit));
    DeleteCriticalSection(&(pDisk->d_DiskCardCrit));
	CloseVirtioBlk(pDisk);
    if (pDisk->d_ActivePath){
        LocalFree(pDisk->d_ActivePath);
    }
    LocalFree(pDisk);
    DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: CloseDisk done with 0x%x\r\n"), pDisk));
}

//------------------------------------------------------------------------------
//
// GetDiskInfo - return disk info in response to DISK_IOCTL_GETINFO
//
//------------------------------------------------------------------------------
DWORD
GetDiskInfo(
    PDISK pDisk,
    PDISK_INFO pInfo
    )
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: GetDiskInfo")));
    *pInfo = pDisk->d_DiskInfo;
	pInfo->di_flags |= DISK_INFO_FLAG_CHS_UNCERTAIN;
    //pInfo->di_flags |= DISK_INFO_FLAG_PAGEABLE;
    //pInfo->di_flags &= ~DISK_INFO_FLAG_UNFORMATTED;
    return ERROR_SUCCESS;
}   // GetDiskInfo



//------------------------------------------------------------------------------
//
// SetDiskInfo - store disk info in response to DISK_IOCTL_SETINFO
//
//------------------------------------------------------------------------------
DWORD
SetDiskInfo(
    PDISK pDisk,
    PDISK_INFO pInfo
    )
{
	DEBUGMSG(ZONE_FUNCTION, (TEXT("virtio_blk: SetDiskInfo")));
    pDisk->d_DiskInfo = *pInfo;
    return ERROR_SUCCESS;
}   // SetDiskInfo
