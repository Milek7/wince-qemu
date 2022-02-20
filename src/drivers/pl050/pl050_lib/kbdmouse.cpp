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
#include <ddkreg.h>
#include <keybdpdd.h>
#include <laymgr.h>

#include "ps2port.hpp"
#include "ps2mouse.hpp"
#include "ps2keybd.hpp"

///<file_doc_scope tref="PS2Keybd" visibility="OAK"/>

extern Ps2Keybd *g_pPS2Keybd;

static Ps2Port  *s_pPS2Port;
static Ps2Mouse *s_pPS2Mouse;

static UINT s_uiPddId;

/// <summary>
///  returns the PDD ID
/// </summary>
/// <returns>
/// </returns>
UINT GetKeybdPDDID()
{
    return s_uiPddId;
}

static PFN_KEYBD_EVENT s_pfnKeybdEvent;

/// <summary>
///  Gets a Keyboard Event (for the IST)
/// </summary>
/// <returns>
///  An event
/// </returns>
PFN_KEYBD_EVENT GetKeybdEventFn()
{
    return s_pfnKeybdEvent;
}

void
WINAPI
KeybdPdd_PowerHandler(
    BOOL    bOff
    );

/// <summary>
///  Handles power down events
/// </summary>
/// <param name="uiPddId">The PDD ID (unused)</param>
/// <param name="fTurnOff">TRUE if it is a power downe event, FALSE for power up</param>
static
void
WINAPI
PS2_8042_PowerHandler(
    UINT /*uiPddId*/,
    BOOL fTurnOff
    )
{
    KeybdPdd_PowerHandler(fTurnOff);

    if (fTurnOff == FALSE && s_pPS2Mouse)
    {
        s_pPS2Mouse->ClearBuffers();
    }
}

/// <summary>
///   Handles IO Controls for the stream interface
/// </summary>
/// <param name="dwCode">The Device Io Control code</param>
/// <param name="pBufIn">The input buffer</param>
/// <param name="dwLenIn">The input buffer size</param>
/// <param name="pdwBufOut">The output buffer</param>
/// <param name="dwLenOut">The output buffer size</param>
/// <param name="pdwActualOut">The size of the results of the IO control placed in the output buffer</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL PS2_8042_IOControl(
    DWORD                         dwCode, 
    __in_bcount(dwLenOut) PVOID   /*pBufIn*/,
    DWORD                         /*dwLenIn*/, 
    __out_bcount(dwLenOut) PDWORD pdwBufOut, 
    DWORD                         dwLenOut,
    __out PDWORD                  pdwActualOut
    )
{
    BOOL fRetVal = FALSE;
    switch (dwCode)
    {
    case IOCTL_POWER_SET:
        // buffers are checked in KBD_SetPowerState
        KeybdShimSetPowerState(pdwBufOut, dwLenOut);
        fRetVal = TRUE;
        break;

    case IOCTL_POWER_GET:
        KeybdShimGetPowerState(pdwBufOut, dwLenOut, pdwActualOut);
        fRetVal = TRUE;
        break;

    // Return device specific power capabilities.
    case IOCTL_POWER_CAPABILITIES:
        // buffers are checked in KBD_GetPowerCaps
        KeybdShimGetPowerCaps(pdwBufOut, dwLenOut, pdwActualOut);
        fRetVal = TRUE;
        break;

    default:
        // no other control codes are supported
        break;
    }

    return fRetVal;
}

/// <summary>
///   Returns the Power States this driver supports
/// </summary>
UCHAR PS2_8042_GetPowerCaps()
{
    return DX_MASK(D0);
}

/// <summary>
///   Sets the keyboard LEDs
/// </summary>
/// <param name="uiPddId">the PDDID (unused)</param>
/// <param name="KeyStateFlags">The keyboard State</param>
static
void 
WINAPI
PS2_8042_ToggleLights(
    UINT /*uiPddId*/,
    KEY_STATE_FLAGS KeyStateFlags
    )
{
    static const KEY_STATE_FLAGS ksfLightMask = KeyShiftCapitalFlag | 
        KeyShiftNumLockFlag | KeyShiftScrollLockFlag; 
    static KEY_STATE_FLAGS ksfCurr;

    KEY_STATE_FLAGS ksfNewState = (ksfLightMask & KeyStateFlags);

    if (ksfNewState != ksfCurr) 
    {
        DEBUGMSG(ZONE_PDD, (_T("PS2_8042_ToggleLights: Changing light state\r\n")));
        KeybdPdd_ToggleKeyNotification(ksfNewState);
        ksfCurr = ksfNewState;
    }

    return;
}


static KEYBD_PDD PS28042Pdd = {
    PS2_AT_PDD,
    _T("PS/2 PL050"),
    PS2_8042_PowerHandler,
    PS2_8042_ToggleLights
};

/// <summary>
///  Destructs the PS/2 driver
/// </summary>
/// <param name="uiPddId">the PDD ID (unused)</param>
/// <param name="ppKeybdPdd">a pointer to a pointer to a PDD (unused)</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
WINAPI
PS2_8042_Exit(
    UINT /*uiPddId*/,
    PKEYBD_PDD * /*ppKeybdPdd*/
    )
{
    if (g_pPS2Keybd != NULL)
    {
        delete g_pPS2Keybd;
        g_pPS2Keybd = NULL;
    }
    if (s_pPS2Mouse != NULL)
    {
        delete s_pPS2Mouse;
        s_pPS2Mouse = NULL;
    }
    if (s_pPS2Port != NULL)
    {
        delete s_pPS2Port;
        s_pPS2Port = NULL;
    }

    return TRUE;
}

/// <summary>
///  Constructs the PS2 driver
/// </summary>
/// <param name="uiPddId">The PDD ID</param>
/// <param name="pfnKeybdEvent">a Pointer to a function to send keyboard events</param>
/// <param name="ppKeybdPdd">The entry will place a PDD in this varible</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
WINAPI
PS2_8042_Entry(
    UINT uiPddId,
    PFN_KEYBD_EVENT pfnKeybdEvent,
    __out PKEYBD_PDD *ppKeybdPdd
    )
{
    BOOL    fRet = FALSE;

    unsigned int        iopBase = 0;

    s_uiPddId = uiPddId;
    s_pfnKeybdEvent = pfnKeybdEvent;

    KEYBD_CALLBACKS Callbacks;
    ZeroMemory(&Callbacks, sizeof(Callbacks));
    Callbacks.dwSize = sizeof Callbacks;
    Callbacks.pfnPddExits = PS2_8042_Exit;
    Callbacks.pfnPddIoControl = PS2_8042_IOControl;
    Callbacks.pfnPddGetPowerCaps = PS2_8042_GetPowerCaps;
    KeybdMDDRegisterCallbacks(uiPddId, &Callbacks);

    // {969877D1-6658-4459-A2C4-056E8A74DAD5}
    static const GUID keybdGuid = 
        { 0x969877d1, 0x6658, 0x4459, { 0xa2, 0xc4, 0x5, 0x6e, 0x8a, 0x74, 0xda, 0xd5 } };

    KeybdMDDRegisterGUID(uiPddId, &keybdGuid);

    DEBUGMSG(ZONE_INIT, (_T("PS2_8042_Entry: Initialize 8042 ID %u\r\n"), uiPddId));
    PREFAST_DEBUGCHK(ppKeybdPdd != NULL);

    *ppKeybdPdd = &PS28042Pdd;

    if ( s_pPS2Port )
    {
        // This driver is not designed for re-entry
        ASSERT(s_pPS2Port == NULL);
        goto leave;
    }

    HKEY hDeviceKey = KeybdShimGetDeviceKey();
	DWORD dwStatus, dwType, dwSize = 4;

	dwStatus = RegQueryValueEx(hDeviceKey, _T("IoBase"), NULL, &dwType, (BYTE*)&iopBase, &dwSize);
    if (dwStatus != ERROR_SUCCESS || dwType != REG_DWORD) 
    {
		ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Could not fetch IoBase.\r\n")));
        goto leave;
    }

    s_pPS2Port = new Ps2Port;
    if ( s_pPS2Port == NULL || !s_pPS2Port->Initialize(iopBase) ) //TODO
    {
        ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Could not initialize ps2 port.\r\n")));
        goto leave;
    }

    //  We always assume that there is a keyboard.
    g_pPS2Keybd = new Ps2Keybd;

    if (g_pPS2Keybd == NULL)
    {
        ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Memory allocation failed\r\n")));
        goto leave;
    }

    if ( g_pPS2Keybd->Initialize(s_pPS2Port) )
    {
        g_pPS2Keybd->IntThreadStart();
    }
    else
    {
        ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Could not initialize ps2 keyboard.\r\n")));
        goto leave;
    }

    if ( s_pPS2Port->MouseFound() )
    {
        s_pPS2Mouse = new Ps2Mouse;

        if (s_pPS2Mouse == NULL)
        {
            ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Memory allocation failed\r\n")));
            goto leave;
        }

        if ( s_pPS2Mouse->Initialize(s_pPS2Port) )
        {
            s_pPS2Mouse->IntThreadStart();
        }
        else
        {
            ERRORMSG(ZONE_ERROR, (TEXT("PS2_8042_Entry: Could not initialize ps2 mouse\r\n")));
            goto leave;
        }
    }

    fRet = TRUE;
    
leave:
    if (fRet == FALSE)
    {
        // failed to initalise, clean up
        if (g_pPS2Keybd != NULL)
        {
            delete g_pPS2Keybd;
            g_pPS2Keybd = NULL;
        }
        if (s_pPS2Mouse != NULL)
        {
            delete s_pPS2Mouse;
            s_pPS2Mouse = NULL;
        }
        if (s_pPS2Port != NULL)
        {
            delete s_pPS2Port;
            s_pPS2Port = NULL;
        }
    }

    DEBUGMSG(ZONE_INIT, (_T("PS2_8042_Entry: Initialization complete\r\n")));
    return fRet;
}

#ifdef DEBUG
// Verify function declaration against the typedef.
static PFN_KEYBD_PDD_ENTRY v_pfnKeybdEntry = PS2_8042_Entry;
#endif


