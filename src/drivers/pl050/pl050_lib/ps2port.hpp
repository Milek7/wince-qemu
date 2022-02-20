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

///<file_doc_scope tref="PS2Keybd" visibility="OAK"/>

/// <summary>
/// </summary>
class HardwareLock 
{
public:
    // aquires the hardware lock
    HardwareLock();
    // releases the hardware lock
    ~HardwareLock();

    // TRUE if *someone* has the lock, not just the current thread
    // used to find code that uses hardware without locking
    static BOOL HaveLock();
};

#define uint32_t unsigned int
#define mmio_write(reg, data)  *(volatile uint32_t*)(reg) = (data)
#define mmio_read(reg)  *(volatile uint32_t*)(reg)

enum pl050_defs
{
	KBD_OFFSET   = 0x00000,
	MOUSE_OFFSET = 0x10000,

	KMICR   = 0x00,
	KMISTAT = 0x04,
	KMIDATA = 0x08
};

/// <summary>
///  This is a very simplistic i8042 interface.
///  It does not provide a complete interface to the i8042 chip.  It is 
///  intended to provide only basic keyboard and mouse support.
/// </summary>
class Ps2Port
{
    unsigned int        m_iopBase;
    CRITICAL_SECTION    m_csWrite;

    int                 m_cEnterWrites;
    BOOL                m_fMouseFound;
    BOOL                m_fIntelliMouseFound;
    BOOL                m_fIntelli5ButtonMouseFound;

    BOOL                m_fEnableWake;

	BOOL m_kbd_int;
	BOOL m_mouse_int;

	BOOL kbd_data_avail()
	{
		return (mmio_read(m_iopBase + KBD_OFFSET + KMISTAT) & (1 << 4)) != 0;
	}

	UINT8 kbd_data_get()
	{
		return mmio_read(m_iopBase + KBD_OFFSET + KMIDATA);
	}

	void kbd_data_put(UINT8 d)
	{
		mmio_write(m_iopBase + KBD_OFFSET + KMIDATA, d);
	}

	BOOL mouse_data_avail()
	{
		return (mmio_read(m_iopBase + MOUSE_OFFSET + KMISTAT) & (1 << 4)) != 0;
	}

	UINT8 mouse_data_get()
	{
		return mmio_read(m_iopBase + MOUSE_OFFSET + KMIDATA);
	}

	void mouse_data_put(UINT8 d)
	{
		mmio_write(m_iopBase + MOUSE_OFFSET + KMIDATA, d);
	}

	void kbd_control_set(BOOL interrupt)
	{
		mmio_write(m_iopBase + KBD_OFFSET + KMICR, (1 << 2) | (interrupt << 4));
	}

	void mouse_control_set(BOOL interrupt)
	{
		mmio_write(m_iopBase + MOUSE_OFFSET + KMICR, (1 << 2) | (interrupt << 4));
	}

    BOOL
    OutputBufPollReadKBD(
        UINT8   *pui8
        );

	BOOL
    OutputBufPollReadMOUSE(
        UINT8   *pui8
        );

    BOOL
    InputBufPutKBD(
        UINT8   ui8Data
        );

	BOOL
    InputBufPutMOUSE(
        UINT8   ui8Data
        );

    void
    MouseCommandPut(
        UINT8   cmdMouse
        );

    BOOL
    MouseCommandPutAndReadResponse(
        UINT8   cmd
        );

    BOOL
    MouseTest(
        void
        );

    UINT8
    MouseId(
        void
        );

    BOOL
    EnterWrite(
        void
        );

    BOOL
    LeaveWrite(
        void
        );

    BOOL
    MouseReset(
        void
        );

    void
    SetModeIntelliMouse
        (
        void
        );

    void
    SetModeIntelli5ButtonMouse
        (
        void
        );

public:

    BOOL
    Initialize(
        unsigned int    iopBase
        );

    ~Ps2Port();
    
    BOOL
    MouseFound(
        void
        ) const
    {
        return m_fMouseFound;
    }

    BOOL
    ResetAndDetectMouse();

    BOOL
    IntelliMouseFound(
        void
        ) const
    {
        return m_fIntelliMouseFound;
    }

    BOOL
    Intelli5ButtonMouseFound(
        void
        ) const
    {
        return m_fIntelli5ButtonMouseFound;
    }

    BOOL
    MouseDataRead(
        UINT8   *pui8Data
        );


    BOOL
    MouseInterruptEnable(
        void
        );


    BOOL
    KeyboardInterfaceTest(
        void
        );

    BOOL
    KeyboardCommandPut(
        UINT8   ui8Cmd
        );

    BOOL
    KeyboardReset(
        void
        );

    void
    KeyboardLights(
        unsigned int    fLights
        );

    BOOL
    KeybdInterruptEnable(
        void
        );

    BOOL
    KeybdDataRead(
        UINT8   *pui8Data
        );

    // Note: On a PS/2 controller, if you have both a keyboard and mouse
    // connected and one is set to wake but the other is not, there is a good
    // chance that neither will wake the system. If the non-wake source 
    // has activity, its data will fill the controller's output buffer, but it will not
    // be removed. Then, if the wake source has activity, it will wait for the
    // controller to accept its data, but this will not happen until the data in
    // the controller's output buffer is read. Thus, no activity on either device 
    // will wake the system. Moral: Have both the keyboard and mouse set as
    // wake sources or neither set as wake sources. We enforce that here.
    void
    SetWake(
        BOOL fEnable
        )
    {
        m_fEnableWake = fEnable;
    }

    BOOL
    WillWake(
        void
        ) const
    {
        return m_fEnableWake;
    }
    
};
