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

#pragma once

#include <windows.h>
#include <nkintr.h>

class Ps2Port;

class Ps2Keybd
{
    Ps2Port *m_pPS2Port;
    HANDLE   m_hevInterrupt;
    DWORD    m_dwSysIntr_Keybd;
    DWORD    m_dwIrq_Keybd;
    DWORD    m_dwPriority;

    BOOL
    InitKeybdResources();

public:
    BOOL
    Initialize(
        Ps2Port *pPS2Port
        );

    BOOL
    IntThreadStart(
        void
        );


    BOOL
    IntThreadProc(
        void
        );

    ~Ps2Keybd();
    Ps2Keybd();

friend
    static
    UINT
    WINAPI
    KeybdPdd_GetEventEx2(
        UINT    uiPddId,
        __out_ecount(16) UINT32  rguiScanCode[16],
        __out_ecount(16) BOOL    rgfKeyDown[16]
        );

friend
    void
    WINAPI
    KeybdPdd_ToggleKeyNotification(
        KEY_STATE_FLAGS KeyStateFlags
        );
};
