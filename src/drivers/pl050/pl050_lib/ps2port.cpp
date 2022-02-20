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

#include "ps2port.hpp"
#include "keybddbg.h"
#include "keybdpdd.h"

///<file_doc_scope tref="PS2Keybd" visibility="OAK"/>

// dpCurSettings is in kbdist.c
#define ZONE_ASSERT             DEBUGZONE(16)

DWORD g_dwIoBase = 0;

static DWORD   s_msPollLimit = 0;

static CRITICAL_SECTION s_csHardwareLock;
static volatile BOOL s_fInLock = FALSE;
HardwareLock::HardwareLock()
{
    EnterCriticalSection(&s_csHardwareLock);
    s_fInLock = TRUE;
}
HardwareLock::~HardwareLock()
{
    s_fInLock = FALSE;
    LeaveCriticalSection(&s_csHardwareLock);
}

HardwareLock::HaveLock()
{
    return s_fInLock;
}

//
// Commands sent to the keyboard.
//
static const UINT8 cmdKeybdSetMode  = 0xF0;
static const UINT8 cmdKeybdReset    = 0xFF;
static const UINT8 cmdKeybdLights   = 0xED;

// Keyboard modes
static const UINT8 cmdKeybdModeAT   = 0x02;

//
// Commands sent to the mouse.
//
static const UINT8 cmdMouseReadId           = 0xF2;
static const UINT8 cmdMouseSetReportRate    = 0xF3;
static const UINT8 cmdMouseEnable           = 0xF4;
static const UINT8 cmdMouseReset            = 0xFF;

//
// Mouse and Keyboard Response
//
static const UINT8 response8042Ack              = 0xFA;
static const UINT8 response8042Resend           = 0xFE;
static const UINT8 response8042IntelliMouseId   = 0x03;
static const UINT8 response8042Intelli5buttonMouseId = 0x04;

/// <summary>
///  Reads the PS2 Timeout from the registry
/// </summary>
/// <param name="Timeout">The timeout in ms</param>
/// <returns>
///   TRUE on success
/// </returns>
BOOL
ReadPS2Timeout(DWORD&Timeout)
{
    BOOL fStatus = FALSE;

    HKEY hkReg, hDeviceKey = KeybdShimGetDeviceKey();
    DWORD dwStatus = RegOpenKeyEx(hDeviceKey, L"Keybd", 0, 0, &hkReg); 
    RegCloseKey(hDeviceKey);

    if (dwStatus == ERROR_SUCCESS) 
    {
        DWORD dwType;
        DWORD dwSize = sizeof(Timeout);
        if(RegQueryValueEx(hkReg, L"Timeout", NULL, &dwType, (LPBYTE) &Timeout, &dwSize) == ERROR_SUCCESS) 
        {
            fStatus = TRUE;
        }
        RegCloseKey(hkReg);
    }

    return fStatus;
}

/// <summary>
///  SPINS waiting for the output buffer to go full and then reads the data
///  from the output buffer. Remember that output means output of the
///  8042, not the PC.
/// </summary>
/// <param name="pui8">an output buffer to read into</param>
/// <returns>
///  Returns TRUE if the buffer goes full, FALSE if
///  the buffer never goes full.
/// </returns>
BOOL
Ps2Port::
OutputBufPollReadKBD(
    UINT8   *pui8
    )
{
    BOOL            fRetVal = FALSE;
    unsigned int    msStart;
    unsigned int    msEnd;

    //  Check for data ready first.
    if ( kbd_data_avail() )
    {
        fRetVal = TRUE;
    }
    else
    {
        //  Start polling if output data not ready.
        msStart = GetTickCount();
        for ( ; ; )
        {
            Sleep(0);

            //  Check for data again.
            if ( kbd_data_avail() )
            {
                fRetVal = TRUE;
                break;
            }

            //  Check elapsed time.
            //  Unsigned arithmetic deals with timer rollover.
            msEnd = GetTickCount();
            if ( ( msEnd - msStart ) > s_msPollLimit )
            {
                ERRORMSG(1, (TEXT("Ps2Port::OutputBufPollRead: Warning polling timeout\r\n")));
                // failed to read response
                break;
            }
        }
    }

    //  May as well read it even if we failed.
    *pui8 = kbd_data_get();
    return fRetVal;
}

BOOL
Ps2Port::
OutputBufPollReadMOUSE(
    UINT8   *pui8
    )
{
    BOOL            fRetVal = FALSE;
    unsigned int    msStart;
    unsigned int    msEnd;

    //  Check for data ready first.
    if ( mouse_data_avail() )
    {
        fRetVal = TRUE;
    }
    else
    {
        //  Start polling if output data not ready.
        msStart = GetTickCount();
        for ( ; ; )
        {
            Sleep(0);

            //  Check for data again.
            if ( mouse_data_avail() )
            {
                fRetVal = TRUE;
                break;
            }

            //  Check elapsed time.
            //  Unsigned arithmetic deals with timer rollover.
            msEnd = GetTickCount();
            if ( ( msEnd - msStart ) > s_msPollLimit )
            {
                ERRORMSG(1, (TEXT("Ps2Port::OutputBufPollRead: Warning polling timeout\r\n")));
                // failed to read response
                break;
            }
        }
    }

    //  May as well read it even if we failed.
    *pui8 = mouse_data_get();
    return fRetVal;
}

/// <summary>
///  Must be called before writing data which is to be sent to the keyboard or
///  mouse.  Takes the write critical section if successful.
/// </summary>
/// <remarks>
///  Disables the keyboard and auxiliary interrupts.
///  
///  Disables the keyboard and auxiliary interfaces.
///  
///  May be called multiple times but only the first caller actually does the
///  disabling.
///  
///  A count is kept to verify that the number of calls to LeaveWrite match the
///  number of calls to EnterWrite.
/// </remarks>
/// <returns>
///  Returns TRUE if successful, FALSE if there is an error.  If there is an
///  error, the write critical section is not held.
/// </returns>
BOOL
Ps2Port::
EnterWrite(
    void
    )
{
    //  Only one writer at a time.
    EnterCriticalSection(&m_csWrite);

    //  If we are already in, we're done.
    if ( m_cEnterWrites++ )
		return TRUE;
    
	kbd_control_set(FALSE);
	mouse_control_set(FALSE);
	while (kbd_data_avail())
		kbd_data_get();
	while (mouse_data_avail())
		mouse_data_get();
	// possible race with incoming data

    //  Keep the critical section if we succeed.
    return TRUE;
}

/// <summary>
///  Counterpart to EnterWrite.  The last caller to LeaveWrite re-enables the
///  interrupts and interfaces.
/// </summary>
/// <returns>
///  Returns TRUE if successful.
/// </returns>
BOOL
Ps2Port::
LeaveWrite(
    void
    )
{
    BOOL    fRetVal = FALSE;

    //  Haven't really seen this but it's worth checking.
    if ( m_cEnterWrites == 0 )
    {
        ERRORMSG(1,(TEXT("LeaveWrite: too many calls to LeaveWrite.\r\n")));
        return fRetVal;
    }

    //  Last one out turns everything back on.
    if ( m_cEnterWrites != 1 )
    {
        fRetVal = TRUE;
        goto leave;
    }

    kbd_control_set(m_kbd_int);
	mouse_control_set(m_mouse_int);

    fRetVal = TRUE;

leave:
    --m_cEnterWrites;
    LeaveCriticalSection(&m_csWrite);
    return fRetVal;
}

/// <summary>
///  Puts data in the 8042 input buffer.  Caller must have called EnterWrite
///  before calling.
/// </summary>
/// <param name="ui8Data">The data to put</param>
/// <returns>
///  Returns TRUE if successful.
/// </returns>
BOOL
Ps2Port::
InputBufPutKBD(
    UINT8   ui8Data
    )
{
    BOOL    fRetVal = FALSE;

    if ( !m_cEnterWrites )
    {
        ERRORMSG(1,(TEXT("Did not use EnterWrite before calling InputBufferPut.\r\n")));
        ASSERT(0);
    }

    kbd_data_put(ui8Data);
    fRetVal = TRUE;

    return fRetVal;
}

BOOL
Ps2Port::
InputBufPutMOUSE(
    UINT8   ui8Data
    )
{
    BOOL    fRetVal = FALSE;

    if ( !m_cEnterWrites )
    {
        ERRORMSG(1,(TEXT("Did not use EnterWrite before calling InputBufferPut.\r\n")));
        ASSERT(0);
    }

    mouse_data_put(ui8Data);
    fRetVal = TRUE;

    return fRetVal;
}

/// <summary>
///  Writes the keyboard interface test command to the 8042 command register
///  and reads the response.
/// </summary>
/// <returns>
///  Returns TRUE if the success response is read.
/// </returns>
BOOL
Ps2Port::
KeyboardInterfaceTest(
    void
    )
{
	return TRUE;
}

/// <summary>
///  Writes a command to the keyboard and reads the response.
/// </summary>
/// <param name="ui8Cmd">The command to put</param>
/// <returns>
///  Returns TRUE if the success response code is read.
/// </returns>
BOOL
Ps2Port::
KeyboardCommandPut(
    UINT8   ui8Cmd
    )
{
    BOOL    fRetVal = FALSE;
    UINT8   ui8Data;
    int     iTries = 0;

    EnterWrite();

    while( iTries++ < 3 )
    {
        if( !InputBufPutKBD(ui8Cmd) )
        {
            break;
        }

        if( !OutputBufPollReadKBD(&ui8Data) )
        {
            break;
        }

        if( ui8Data == response8042Ack )
        {
            fRetVal = TRUE;
            break;
        }

        // resend command if keyboard microcontroller asked for a resend
        // otherwise break and return FALSE
        if( ui8Data != response8042Resend )
        {
            ASSERT(0);
            break;
        }
    }

    LeaveWrite();
    return fRetVal;
}

/// <summary>
///  Sends the mouse reset command to the mouse and reads the response.
/// </summary>
/// <returns>
///  Returns TRUE if the success response is read.
/// </returns>
BOOL
Ps2Port::
MouseReset(
    void
    )
{
    BOOL    fRetVal = FALSE;
    UINT8   ui8Data;

    EnterWrite();

    if ( !MouseCommandPutAndReadResponse(cmdMouseReset) )
    {
        goto leave;
    }

    if ( !OutputBufPollReadMOUSE(&ui8Data) )
    {
        goto leave;
    }

    PREFAST_SUPPRESS( 30033, "comparison is between bytes, not chars");
    if ( ui8Data != 0xAA )
    {
        ASSERT(0);
        goto leave;
    }

    if ( !OutputBufPollReadMOUSE(&ui8Data) )
    {
        goto leave;
    }

    if ( ui8Data != 0x00 )
    {
        ASSERT(0);
        goto leave;
    }

    fRetVal = TRUE;

leave:

    LeaveWrite();
    return fRetVal;
}

/// <summary>
///  Sends the keyboard reset command to the keyboard and reads the response.
/// </summary>
/// <returns>
///  Returns TRUE if the success response is read.
/// </returns>
BOOL
Ps2Port::
KeyboardReset(
    void
    )
{
    BOOL    fRetVal = FALSE;
    UINT8   ui8Data;

    EnterWrite();

    if ( !KeyboardCommandPut(cmdKeybdReset) )
    {
        goto leave;
    }

    if ( !OutputBufPollReadKBD(&ui8Data) )
    {
        goto leave;
    }

    PREFAST_SUPPRESS( 30033, "comparison is between bytes, not chars");
    if ( ui8Data != 0xAA )
    {
        ASSERT(0);
        goto leave;
    }

    if ( KeyboardCommandPut(cmdKeybdSetMode) )
    {
        KeyboardCommandPut(cmdKeybdModeAT);
    }

    fRetVal = TRUE;

leave:

    LeaveWrite();
    return fRetVal;
}

/// <summary>
///  Sets the keyboard indicator lights.
/// </summary>
/// <param name="fLights">The keybaord state</param>
void
Ps2Port::
KeyboardLights(
    unsigned int    fLights
    )
{
    EnterWrite();

    if ( !KeyboardCommandPut(cmdKeybdLights) )
    {
        goto leave;
    }

    if ( !KeyboardCommandPut(static_cast<UINT8>(fLights)) )
    {
        goto leave;
    }

leave:
    LeaveWrite();
    return;
}

/// <summary>
///  Writes a command to the auxiliary device.
/// </summary>
/// <param name="cmd">the command to write</param>
void
Ps2Port::
MouseCommandPut(
    UINT8   cmd
    )
{
    EnterWrite();
    InputBufPutMOUSE(cmd);
    LeaveWrite();
    return;
}

/// <summary>
///  Writes a command to the auxiliary device and reads the response.
/// </summary>
/// <param name="cmd">the command to write</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
MouseCommandPutAndReadResponse(
    UINT8   cmd
    )
{
    BOOL    fRetVal = FALSE;
    UINT8   ui8Data;
    int     iTries = 0;

    EnterWrite();

	while( iTries++ < 3 )
	{
        if( !InputBufPutMOUSE(cmd) )
        {
            break;
        }

        if( !OutputBufPollReadMOUSE(&ui8Data) )
        {
            break;
        }

        if( ui8Data == response8042Ack )
        {
            fRetVal = TRUE;
            break;
        }

        // resend command if keyboard microcontroller asked for a resend
        // otherwise break and return FALSE
        if( ui8Data != response8042Resend )
        {
            ASSERT(0);
            break;
        }
    }

    LeaveWrite();

    return fRetVal;
}

/// <summary>
///  Tests for a mouse present on the 8042 auxiliary port.
/// </summary>
/// <returns>
///  Returns TRUE if a mouse is found.
/// </returns>
BOOL
Ps2Port::
MouseTest(
    void
    )
{
    BOOL    fRetVal = FALSE;
    UINT8   ui8Data;

    EnterWrite();

    MouseCommandPut(cmdMouseReadId);

    //ui8Data should contain response8042Ack
    if ( !OutputBufPollReadMOUSE(&ui8Data) )
    {
        goto leave;
    }

    //ui8Data should contain mouse ID
    if ( !OutputBufPollReadMOUSE(&ui8Data) )
    {
        // this will fail here if there is no mouse attached
        goto leave;
    }

    fRetVal = TRUE;

leave:
    if ( !fRetVal )
    {
        DEBUGMSG(ZONE_INIT,(TEXT("MouseTest fails.  Mouse probably not present.\r\n")));
    }

    LeaveWrite();
    return fRetVal;
}

/// <summary>
///  Set mouse to IntelliMouse mode.  If an IntelliMouse is present:
///      1. mouse ID becomes response8042IntelliMouseId
///      2. the motion report format changes
///  If IntelliMouse not present the mouse ID and motion report format remain at the default values
/// </summary>
void
Ps2Port::
SetModeIntelliMouse(
    void
    )
{
    EnterWrite();

    // IntelliMouse(R) is uniquely identified by issuing the specific series of Set Report Rate commands:
    // 200Hz (0xC8), then 100Hz (0x64), then 80Hz (0x50).  The Set Report Rate commands are valid and we
    // therefore have to set the report rate back to the default 100Hz (this is done be MouseId() ).
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0xC8);
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0x64);
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0x50);

    LeaveWrite();
}

/// <summary>
///  Set the mouse to IntelliMouse 5 button mode
/// </summary>
void
Ps2Port::
SetModeIntelli5ButtonMouse(
    void
    )
{
    EnterWrite();

    // Intelli 5 button Mouse is uniquely identified by issuing the specific series of Set Report Rate commands:
    // 200Hz (0xC8), then 200Hz (0xC8), then 80Hz (0x50).  The Set Report Rate commands are valid and we
    // therefore have to set the report rate back to the default 100Hz (this is done be MouseId() ).
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0xC8);
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0xC8);
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0x50);

    LeaveWrite();
}

/// <summary>
///  Reads the mouse ID
/// </summary>
/// <returns>
///  The mouse ID
/// </returns>
UINT8
Ps2Port::
MouseId(
    void
    )
{
    UINT8   ui8MouseId;

    EnterWrite();

    MouseCommandPutAndReadResponse(cmdMouseReadId);
    OutputBufPollReadMOUSE(&ui8MouseId);

    // Set the report rate back to the default 100Hz
    MouseCommandPutAndReadResponse(cmdMouseSetReportRate);
    MouseCommandPutAndReadResponse(0x64);

    LeaveWrite();
    return ui8MouseId;
}

/// <summary>
///  Reads data from the 8042 output port.
/// </summary>
/// <param name="pui8Data">The output data</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
MouseDataRead(
    UINT8   *pui8Data
    )
{
    BOOL fRetVal = TRUE;
    if ( !mouse_data_avail() )
    {
        DEBUGMSG(1, (TEXT("WARNING MouseDataRead: Mouse data not ready\r\n")));
        fRetVal = FALSE;
    }

    *pui8Data = mouse_data_get();
    return fRetVal;
}

/// <summary>
///  Enables 8042 auxiliary interrupts.
/// </summary>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
MouseInterruptEnable(
    void
    )
{
	m_mouse_int = TRUE;
	mouse_control_set(m_mouse_int);
    return TRUE;
}

/// <summary>
///  Enables 8042 keyboard interrupts.
/// </summary>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
KeybdInterruptEnable(
    void
    )
{
    m_kbd_int = TRUE;
	kbd_control_set(m_kbd_int);
    return TRUE;
}

/// <summary>
///  Reads data from 8042 output port.
/// </summary>
/// <param name="pui8Data">The data that was read</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
KeybdDataRead(
    UINT8   *pui8Data
    )
{
    if ( !kbd_data_avail() )
    {
        ERRORMSG(1, (TEXT("KeybdDataRead: Keybd data not ready\r\n")));
    }

    *pui8Data = kbd_data_get();
    return TRUE;
}

/// <summary>
///  Destructs the PS2 Port
/// </summary>
Ps2Port::
~Ps2Port()
{
	kbd_control_set(FALSE);
	mouse_control_set(FALSE);
    DeleteCriticalSection(&m_csWrite);
    DeleteCriticalSection(&s_csHardwareLock);
}

/// <summary>
///  Resets the mouse then detects what sort of mouse it is
/// </summary>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
ResetAndDetectMouse()
{
    m_fMouseFound = FALSE;

    if ( MouseReset() && MouseTest() )
    {
        m_fMouseFound = TRUE;

        // Read mouse ID to determine if IntelliMouse is present.
        SetModeIntelliMouse();
        if( response8042IntelliMouseId == MouseId() )
        {
            m_fIntelliMouseFound = TRUE;
            //Read mouse ID to determine if 5 button IntelliMouse is present
            SetModeIntelli5ButtonMouse();
            if( response8042Intelli5buttonMouseId == MouseId() )
            {
                m_fIntelli5ButtonMouseFound = TRUE;
            }
        }

        MouseCommandPutAndReadResponse(cmdMouseEnable);
    }
    else
    {
        DEBUGMSG(ZONE_INIT, (TEXT("PS2: PS2 mouse reset and detect failed.\r\n")));
    }

    return m_fMouseFound;
}

/// <summary>
///  Initializes the Ps2Port object.
/// </summary>
/// <param name="iopBase">the address of the PS2 registers</param>
/// <returns>
///  TRUE on success
/// </returns>
BOOL
Ps2Port::
Initialize(
    unsigned int    iopBase
    )
{
    BOOL    fRetVal = FALSE;

    m_iopBase = iopBase;
    g_dwIoBase = m_iopBase;      // for power handling
    InitializeCriticalSection(&m_csWrite);
    InitializeCriticalSection(&s_csHardwareLock);
    m_cEnterWrites = 0;

    m_fMouseFound = FALSE;
    m_fIntelliMouseFound = FALSE;
    m_fIntelli5ButtonMouseFound = FALSE;
    m_fEnableWake = FALSE;
	m_kbd_int = FALSE;
	m_mouse_int = FALSE;

    HardwareLock hwLock;
    CeSetThreadAffinity(GetCurrentThread(), 1);

    if (ReadPS2Timeout(s_msPollLimit) == FALSE)
    {
        s_msPollLimit = 450; //  mouse should respond in 400ms +=10%
    }

    ASSERT(s_msPollLimit);

	kbd_control_set(FALSE);
	mouse_control_set(FALSE);

    KeyboardReset();

    ResetAndDetectMouse();

    fRetVal = TRUE;

    return fRetVal;
}

