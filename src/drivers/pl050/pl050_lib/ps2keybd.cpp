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
/*

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>

#include <laymgr.h>
#include <keybdpdd.h>
#include <keybdist.h>

#include "ps2port.hpp"
#include "ps2mouse.hpp"
#include "ps2keybd.hpp"

///<file_doc_scope tref="PS2Keybd" visibility="OAK"/>

// Scan code consts
static const UINT8 scE0Extended = 0xe0;
static const UINT8 scE1Extended = 0xe1;

static const UINT8 scKeyUp = 0xf0;

//  There is really only one physical keyboard supported by the system.
Ps2Keybd    *g_pPS2Keybd;

/// <summary>
///  Handles Power Up/Down of the keyboard
/// </summary>
/// <param name="fOff">TRUE if the event is a power off</param>
void
WINAPI
KeybdPdd_PowerHandler(
    BOOL fOff
    )
{

}

/// <summary>
///   Gets any keyboard events
/// </summary>
/// <param name="uiPddId">the PDD ID (unused)</param>
/// <param name="rguiScanCode">The scan code of the event</param>
/// <param name="rgfKeyUp">TRUE if the event was a keyup</param>
/// <returns>
///   Returns a count of events
/// </returns>
static
UINT
WINAPI
KeybdPdd_GetEventEx2(
    UINT    /*uiPddId*/,
    __out_ecount(16) UINT32  rguiScanCode[16],
    __out_ecount(16) BOOL    rgfKeyUp[16]
    )
{
    static  UINT32  scInProgress;
    static  UINT32  scPrevious;
    static  BOOL    fKeyUp;
    
    UINT8   ui8ScanCode;
    UINT    cEvents = 0;

    PREFAST_DEBUGCHK(rguiScanCode != NULL);
    PREFAST_DEBUGCHK(rgfKeyUp != NULL);

    HardwareLock hwLock;

    g_pPS2Keybd->m_pPS2Port->KeybdDataRead(&ui8ScanCode);

    DEBUGMSG(ZONE_SCANCODES, 
        (_T("KeybdPdd_GetEventEx2: scan code 0x%08x, code in progress 0x%08x, previous 0x%08x\r\n"),
        ui8ScanCode, scInProgress, scPrevious));

    if ( ui8ScanCode == scKeyUp )
    {
        fKeyUp = TRUE;
    }
    else if ( ui8ScanCode == scE0Extended )
    {
        scInProgress = 0xe000;
    }
    else if ( ui8ScanCode == scE1Extended )
    {
        scInProgress = 0xe10000;
    }
    else if ( scInProgress == 0xe10000 )
    {
        scInProgress |= ui8ScanCode << 8;
    }
    else
    {
        scInProgress |= ui8ScanCode;

        if ( ( scInProgress == scPrevious ) && ( fKeyUp == FALSE ) )
        {
            //  mdd handles auto-repeat so ignore auto-repeats from keybd
        }
        else    // Not a repeated key.  This is the real thing.
        {
            //  The Korean keyboard has two keys which generate a single
            //  scan code when pressed.  The keys don't auto-repeat or
            //  generate a scan code on release.  The scan codes are 0xf1
            //  and 0xf2.  It doesn't look like any other driver uses
            //  the 0x71 or 0x72 scan code so it should be safe.

            //  If it is one of the Korean keys, drop the previous scan code.
            //  If we didn't, the earlier check to ignore auto-repeating keys
            //  would prevent this key from working twice in a row.  (Since the
            //  key does not generate a scan code on release.)
            if ( ( scInProgress == 0xf1 ) ||
                 ( scInProgress == 0xf2 ) )
            {
                scPrevious = 0;
            }
            else if ( fKeyUp != FALSE )
            {
                scPrevious = 0x00000000;
            }
            else
            {
                scPrevious = scInProgress;
            }
                
            rguiScanCode[cEvents] = scInProgress;
            rgfKeyUp[cEvents] = fKeyUp;
            ++cEvents;
        }
        scInProgress = 0;
        fKeyUp = FALSE;
    }

    return cEvents;
}

/// <summary>
///  Sets the Keyboard LEDs
/// </summary>
/// <param name="KeyStateFlags">The Keyboard state</param>
void
WINAPI
KeybdPdd_ToggleKeyNotification(
    KEY_STATE_FLAGS KeyStateFlags
    )
{
    unsigned int    fLights;

    fLights = 0;

    if ( KeyStateFlags & KeyShiftCapitalFlag )
    {
        fLights |= 0x04;
    }

    if ( KeyStateFlags & KeyShiftNumLockFlag )
    {
        fLights |= 0x2;
    }

    if ( KeyStateFlags & KeyShiftScrollLockFlag )
    {
        fLights |= 0x1;
    }

    HardwareLock hwLock;
    g_pPS2Keybd->m_pPS2Port->KeyboardLights(fLights);

    return;
}

/// <summary>
///  Initalises the keybd resources from the registry
/// </summary>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL
Ps2Keybd::
InitKeybdResources()
{
    BOOL fRetVal = FALSE;
    DWORD dwTransferred = 0;
    HKEY hKey;
    DWORD dwStatus, dwSize, dwValue, dwType;

    HKEY hDeviceKey = KeybdShimGetDeviceKey();
    dwStatus = RegOpenKeyEx(hDeviceKey, L"Keybd", 0, 0, &hKey); 

    if (dwStatus == ERROR_SUCCESS) 
    {
        // See if we need to enable our interrupt to wake the sustem from suspend.
        dwSize = sizeof(dwValue);
        
        // share EnableWake between keybd and mouse

        // On an 8042 PS/2 controller, if you have both a keyboard and mouse
        // connected and one is set to wake but the other is not, there is a good
        // chance that neither will wake the system. If the non-wake source 
        // has activity, its data will fill the controller's output buffer, but it will not
        // be removed. Then, if the wake source has activity, it will wait for the
        // controller to accept its data, but this will not happen until the data in
        // the controller's output buffer is read. Thus, no activity on either device 
        // will wake the system. Moral: Have both the keyboard and mouse set as
        // wake sources or neither set as wake sources.

        dwStatus = RegQueryValueEx(hDeviceKey, _T("EnableWake"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
        if(dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
            if (dwValue != 0) 
            {
                m_pPS2Port->SetWake(TRUE);
            }
        }

        // get interrupt thread priority
        dwSize = sizeof(dwValue);
        dwStatus = RegQueryValueEx(hKey, _T("Priority256"), NULL, &dwType, reinterpret_cast<BYTE*>(&dwValue), 
            &dwSize);
        if(dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
            m_dwPriority = static_cast<int>(dwValue);
        }

        // read our sysintr
        dwSize = sizeof(dwValue);
        dwStatus = RegQueryValueEx(hKey, _T("SysIntr"), NULL, &dwType, reinterpret_cast<BYTE*>(&dwValue), &dwSize);
        if(dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
            m_dwSysIntr_Keybd = dwValue;
        }
        else
        {
            // no sysintr - need to get the interrupt, and translate to a sysintr
            dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hKey, _T("Irq"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
            if (dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
            {
                UINT32 aIrqs[3];

                aIrqs[0] = UINT32(-1);
                // Using -1 indicates we are not using the legacy calling convention.
#define OAL_INTR_TRANSLATE          (1 << 3)
                aIrqs[1] = OAL_INTR_TRANSLATE;  
                // Flags - in this case we want the existing sysintr if it
                // has already been allocated, and a new sysintr otherwise.

                aIrqs[2] = dwValue;

                BOOL fErr = KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, aIrqs, sizeof(aIrqs), &m_dwSysIntr_Keybd, 
                    sizeof(m_dwSysIntr_Keybd), NULL);
                if (fErr == FALSE)
                {
                    ERRORMSG(1, (L"Keyboard: Failed request sysintr"));
                    DEBUGCHK(fErr); // KernelIoControl should always succeed.
                    goto leave;
                }

                m_dwIrq_Keybd = dwValue;
            }
            else 
            {
                dwStatus = ERROR_INVALID_PARAMETER;
            }
        }

        RegCloseKey(hDeviceKey);
        RegCloseKey(hKey);
    }
    else
    {
        RegCloseKey(hDeviceKey);
    }

    if(dwStatus != ERROR_SUCCESS) 
    {
        goto leave;
    }

    m_hevInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if ( m_hevInterrupt == NULL)
    {
        goto leave;
    }

    if ( !InterruptInitialize(m_dwSysIntr_Keybd, m_hevInterrupt, NULL, 0) )
    {
        goto leave;
    }

    if (m_pPS2Port->WillWake()) 
    {
        // Ask the OAL to enable our interrupt to wake the system from suspend.
        DEBUGMSG(ZONE_INIT, (TEXT("Keyboard: Enabling wake from suspend\r\n")));            
        BOOL fErr = KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &m_dwSysIntr_Keybd,  
            sizeof(m_dwSysIntr_Keybd), NULL, 0, &dwTransferred);
        if (fErr == FALSE)
        {
            ERRORMSG(1, (L"Keyboard: Failed to set as wake source"));
            DEBUGCHK(fErr); // KernelIoControl should always succeed.
        }
    }

    fRetVal = TRUE;

leave:

    return fRetVal;
}

/// <summary>
///   The implementation of the PS2 Keyboard Interrupt Service Thread
/// </summary>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Keybd::
IntThreadProc(
    void
    )
{
    // set the thread priority
    CeSetThreadPriority(GetCurrentThread(), m_dwPriority);

    {
        HardwareLock hwLock;
        m_pPS2Port->KeybdInterruptEnable();
    }

    extern UINT GetKeybdPDDID();
    extern PFN_KEYBD_EVENT GetKeybdEventFn();

    KEYBD_IST keybdIst;
    keybdIst.hevInterrupt = m_hevInterrupt;
    keybdIst.dwSysIntr_Keybd = m_dwSysIntr_Keybd;
    keybdIst.uiPddId = GetKeybdPDDID();
    keybdIst.pfnGetKeybdEvent = KeybdPdd_GetEventEx2;
    keybdIst.pfnKeybdEvent = GetKeybdEventFn();

    KeybdIstLoop(&keybdIst);

    return 0;
}

/// <summary>
///   The PS2 Keyboard Thread
/// </summary>
/// <param name="pp2k">Pointer to a PS2 Keyboard object</param>
/// <returns>
///   A thread exit code
/// </returns>
DWORD
Ps2KeybdIntThread(
    Ps2Keybd    *pp2k
    )
{
    pp2k->IntThreadProc();

    return 0;
}

/// <summary>
///   Starts the Keyboard Interrupt Thread (IST)
/// </summary>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Keybd::
IntThreadStart(
    void
    )
{
    HANDLE hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Ps2KeybdIntThread), 
        this, CREATE_SUSPENDED, NULL);

    CeSetThreadAffinity(hThread, 1);
    ResumeThread(hThread);

    //  Since we don't need the handle, close it now.
    CloseHandle(hThread);
    return TRUE;
}
extern volatile BOOL g_fExiting;

/// <summary>
///  PS2 Keyboard destructor
/// </summary>
Ps2Keybd::
~Ps2Keybd()
{
    g_fExiting = TRUE;

    InterruptDisable(m_dwSysIntr_Keybd);

    SetEvent(m_hevInterrupt);
    Sleep(0);

    if (m_dwIrq_Keybd != -1)
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, (PVOID)&m_dwSysIntr_Keybd, sizeof(m_dwSysIntr_Keybd), NULL, 0, 0);

    CloseHandle(m_hevInterrupt);
    m_hevInterrupt = 0;
}

/// <summary>
///  PS2 Keyboard constructor
/// </summary>
Ps2Keybd::
Ps2Keybd() : 
    m_dwSysIntr_Keybd(DWORD(SYSINTR_UNDEFINED)), 
    m_dwIrq_Keybd(DWORD(-1)), 
    m_pPS2Port(NULL)  
{
}

/// <summary>
///  Initalises the PS Keyboard
/// </summary>
/// <param name="pPS2Port">a pointer to the PS2 Port</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Keybd::
Initialize(
    Ps2Port *pPS2Port
    )
{
    BOOL fRet = FALSE;

    m_dwPriority = 145;
    m_pPS2Port = pPS2Port;

    if (InitKeybdResources() == FALSE)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    HardwareLock hwLock;

    if ( !m_pPS2Port->KeyboardInterfaceTest() )
    {
        ASSERT(0);
        goto leave;
    }

    g_pPS2Keybd->m_pPS2Port->KeyboardLights(0);

    fRet = TRUE;

leave:
    return fRet;
}


