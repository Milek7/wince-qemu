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

const static DWORD IN_PACKET_TIMEOUT = 500;


class Ps2Port;

class Ps2Mouse
{
    Ps2Port *m_pPS2Port;
    HANDLE  m_hevInterrupt;
    UINT8   m_ui8ButtonState;
    UINT8   m_uiXButtonState; //State of 4th and 5th button
    BYTE    m_cReportFormatLength; // depends on the presence of an IntelliMouse(R)
    DWORD   m_dwSysIntr_Mouse;
    DWORD   m_dwIrq_Mouse;
    DWORD   m_dwPriority;

public:
    BOOL
    Initialize(
        Ps2Port *m_pPS2Port
        );

    BOOL
    IntThreadStart(
        void
        );

    BOOL
    InitMouseResources(
        void
        );

    BOOL
    IntThreadProc(
        void
        );

    BOOL
    ClearBuffers(
        void
        );

    ~Ps2Mouse();
    Ps2Mouse() : m_dwSysIntr_Mouse(DWORD(SYSINTR_UNDEFINED)), m_dwIrq_Mouse(DWORD(-1)) {};

};
