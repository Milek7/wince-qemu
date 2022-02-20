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

#include "ps2port.hpp"
#include "ps2mouse.hpp"
#include "ps2keybd.hpp"

#include "keybddbg.h"
#include "keybdpdd.h"

///<file_doc_scope tref="PS2Keybd" visibility="OAK"/>

/// <summary>
///  Initialises the PS/2 Mouse
/// </summary>
/// <param name="pPS2Port">a pointer to the PS/2 port</param>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL
Ps2Mouse::
Initialize(
    Ps2Port *pPS2Port
    )
{
    m_pPS2Port = pPS2Port;
    m_hevInterrupt = NULL;
    m_ui8ButtonState = 0;
    m_uiXButtonState = 0;
    m_dwPriority = CE_THREAD_PRIO_256_HIGHEST;

    m_cReportFormatLength = m_pPS2Port->IntelliMouseFound() ? 4 : 3;

    return TRUE;
}

extern volatile BOOL g_fExiting;

/// <summary>
///  Destructs the PS/2 mouse
/// </summary>
Ps2Mouse::
~Ps2Mouse()
{
    InterruptDisable(m_dwSysIntr_Mouse);

    g_fExiting = TRUE;
    SetEvent(m_hevInterrupt);
    Sleep(0);
    CloseHandle(m_hevInterrupt);
    m_hevInterrupt = 0;

    if (m_dwIrq_Mouse != -1)
    {
        KernelIoControl(IOCTL_HAL_RELEASE_SYSINTR, (PVOID)&m_dwSysIntr_Mouse, sizeof(m_dwSysIntr_Mouse), NULL, 0, 0);
    }
}

/// <summary>
///  Clears the buffers of mouse data
/// </summary>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL 
Ps2Mouse::
ClearBuffers()
{
    m_pPS2Port->ResetAndDetectMouse();

    return TRUE;
}

/// <summary>
///  Initalises the mouse resources from the registry
/// </summary>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL
Ps2Mouse::
InitMouseResources()
{
    DWORD dwStatus = 0;
    BOOL fRetVal = FALSE;

    HKEY hk, hDeviceKey = KeybdShimGetDeviceKey();
    dwStatus = RegOpenKeyEx(hDeviceKey, L"Mouse", 0, 0, &hk); 

    if (dwStatus == ERROR_SUCCESS) 
    {
        // See if we need to enable our interrupt to wake the sustem from suspend.
        DWORD dwValue, dwType;
        DWORD dwSize = sizeof(dwValue);

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
        if (dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
            if (dwValue != 0) 
            {
                m_pPS2Port->SetWake(TRUE);
            }
        }

       
        // get interrupt thread priority
        dwSize = sizeof(dwValue);
        dwStatus = RegQueryValueEx(hk, _T("Priority256"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
        if (dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
           m_dwPriority = dwValue;
        }

        // read our sysintr
        dwSize = sizeof(dwValue);
        dwStatus = RegQueryValueEx(hk, _T("SysIntr"), NULL, &dwType, reinterpret_cast<BYTE*>(&dwValue), &dwSize);
        if(dwStatus == ERROR_SUCCESS && dwType == REG_DWORD) 
        {
            m_dwSysIntr_Mouse = dwValue;
        }
        else
        {
            // no sysintr - need to get the interrupt, and translate to a sysintr
            dwSize = sizeof(dwValue);
            dwStatus = RegQueryValueEx(hk, _T("Irq"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);
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

                m_dwIrq_Mouse = dwValue;

                KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &aIrqs, sizeof(aIrqs), &m_dwSysIntr_Mouse, 
                    sizeof(m_dwSysIntr_Mouse), NULL);
            }
            else
            {
                dwStatus = ERROR_INVALID_PARAMETER;
            }
        }

        RegCloseKey(hDeviceKey);
        RegCloseKey(hk);
    }
    else
    {
        RegCloseKey(hDeviceKey);
    }

    if (dwStatus != ERROR_SUCCESS) 
    {
        goto leave;
    }

    m_hevInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if ( m_hevInterrupt == NULL)
    {
        goto leave;
    }

    if ( !InterruptInitialize(m_dwSysIntr_Mouse, m_hevInterrupt, NULL, 0) )
    {
        goto leave;
    }

    if (m_pPS2Port->WillWake()) 
    {
        DWORD dwTransferred = 0;

        // Ask the OAL to enable our interrupt to wake the system from suspend.
        DEBUGMSG(ZONE_INIT, (TEXT("Mouse: Enabling wake from suspend\r\n")));
        BOOL fErr = KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &m_dwSysIntr_Mouse, 
            sizeof(m_dwSysIntr_Mouse), NULL, 0, &dwTransferred);
        if (fErr == FALSE)
        {
            ERRORMSG(1, (L"Mouse: Failed to set as wake source"));
            DEBUGCHK(fErr); // KernelIoControl should always succeed.
        }

    }

    {
        HardwareLock hwLock;
        m_pPS2Port->MouseInterruptEnable();
    }

    fRetVal = TRUE;

leave:
    return fRetVal;
}

/// <summary>
///  The implementation of the PS/2 Mouse Interrupt Service Thread (IST)
/// </summary>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL
Ps2Mouse::
IntThreadProc(
    void
    )
{
    int  cBytes = 0;
    INT8 buf[4];

    CeSetThreadPriority(GetCurrentThread(), m_dwPriority);

    while (g_fExiting == FALSE)
    {
        DWORD WaitResult = WaitForSingleObject(m_hevInterrupt, (cBytes == 0 ? INFINITE : IN_PACKET_TIMEOUT));

        ASSERT(WaitResult != WAIT_FAILED);
        if (g_fExiting != FALSE || WaitResult == WAIT_FAILED)
        {
            break;
        }

        if (WaitResult == WAIT_TIMEOUT)
        {
            HardwareLock hwLock;
            UINT8       ui8Data;

            DEBUGMSG(ZONE_MOUSEDATA, (_T("%s: packet timeout, cBytes = %d\r\n"), _T(__FILE__), cBytes));

            cBytes = 0;

            // make sure mouse input buffer is empty
            m_pPS2Port->MouseDataRead(&ui8Data);
            m_pPS2Port->MouseDataRead(&ui8Data);
            m_pPS2Port->MouseDataRead(&ui8Data);

            continue;
        }

        HardwareLock hwLock;
        UINT8       ui8Data;
        if ( m_pPS2Port->MouseDataRead(&ui8Data) )
        {   
            if (cBytes < m_cReportFormatLength && cBytes <= sizeof(buf))
            {
                DEBUGCHK(cBytes < _countof(buf));
                buf[cBytes++] = ui8Data;
            }
            if ( cBytes == m_cReportFormatLength )
            {
                unsigned int    evfMouse = 0;
                UINT8 ui8XorButtons = buf[0] ^ m_ui8ButtonState;
                if ( ui8XorButtons )
                {
                    UINT8 ui8Buttons = buf[0];

                    if ( ui8XorButtons & 0x01 )
                    {
                        if ( ui8Buttons & 0x01 )
                        {
                            evfMouse |= MOUSEEVENTF_LEFTDOWN;
                            DEBUGMSG(ZONE_MOUSEDATA, (_T("%s: MOUSEEVENTF_LEFTDOWN byte %d = 0x%02x\r\n"),
                                _T(__FILE__), cBytes, ui8Data));
                        }
                        else
                        {
                            evfMouse |= MOUSEEVENTF_LEFTUP;
                            DEBUGMSG(ZONE_MOUSEDATA, (_T("%s: MOUSEEVENTF_LEFTUP byte %d = 0x%02x\r\n"),
                                _T(__FILE__), cBytes, ui8Data));
                        }
                    }

                    if ( ui8XorButtons & 0x02 )
                    {
                        if ( ui8Buttons & 0x02 )
                        {
                            evfMouse |= MOUSEEVENTF_RIGHTDOWN;
                            DEBUGMSG(ZONE_MOUSEDATA, (_T("%s: MOUSEEVENTF_RIGHTDOWN byte %d = 0x%02x\r\n"),
                                _T(__FILE__), cBytes, ui8Data));
                        }
                        else
                        {
                            evfMouse |= MOUSEEVENTF_RIGHTUP;
                            DEBUGMSG(ZONE_MOUSEDATA, (_T("%s: MOUSEEVENTF_RIGHTUP byte %d = 0x%02x\r\n"),
                                _T(__FILE__), cBytes, ui8Data));
                        }
                    }

                    if ( ui8XorButtons & 0x04 )
                    {
                        if ( ui8Buttons & 0x04 )
                        {
                            evfMouse |= MOUSEEVENTF_MIDDLEDOWN;
                        }
                        else
                        {
                            evfMouse |= MOUSEEVENTF_MIDDLEUP;
                        }
                    }
                    m_ui8ButtonState = buf[0];
                }

                if ( buf[1] || buf[2] )
                {
                    evfMouse |= MOUSEEVENTF_MOVE;
                }

                long x = buf[1];
                long y = -buf[2];
                long w =  0;

                // If IntelliMouse is present, buf[3] contains the relative displacement of the mouse wheel
                // If 5 button Intellimouse is present, buf[3] contains mouse wheel and xbutton information
                if ( m_pPS2Port->IntelliMouseFound())
                {
                    // For 5 button mouse, the lower nibble of buf[3] contains the wheel info
                    // and the upper one contains state of the 4th and 5th buttons
                    if (m_pPS2Port->Intelli5ButtonMouseFound())
                    {
                        UINT8 uiXButtons = buf[3] & 0x30;
                        
                        UINT8 uiXXorButtons = uiXButtons ^ m_uiXButtonState;
                        
                        if ( uiXXorButtons )
                        {
                            if ( uiXXorButtons & 0x10 )
                            {
                                // Change in state of 4th button
                                if ( uiXButtons & 0x10 )
                                {
                                    evfMouse |= MOUSEEVENTF_XDOWN;
                                }
                                else
                                {
                                    evfMouse |= MOUSEEVENTF_XUP;
                                }
                                mouse_event(evfMouse, x, y, XBUTTON1, NULL);
                                evfMouse = 0;
                            }

                            if ( uiXXorButtons & 0x20 )
                            {
                                // Change in state of 5th button
                                if ( uiXButtons & 0x20 )
                                {
                                    evfMouse |= MOUSEEVENTF_XDOWN;
                                }
                                else
                                {
                                    evfMouse |= MOUSEEVENTF_XUP;
                                }
                                mouse_event(evfMouse, x, y, XBUTTON2, NULL);
                                evfMouse = 0;
                            }
                            m_uiXButtonState = uiXButtons;

                        }

                        UINT8 uiWheelData = buf[3] & 0x0F;
                        
                        if (uiWheelData)
                        {   
                            // Sign extend the wheel data
                            if (uiWheelData & 0x08)
                            {
                                uiWheelData |= 0xF0;
                            }
                            evfMouse |= MOUSEEVENTF_WHEEL;
                            w = -(INT8)uiWheelData * 120;
                        }
                    }
                    else
                    {
                        if (buf[3])
                        {
                            evfMouse |= MOUSEEVENTF_WHEEL;
                            // we need to scale by WHEEL_DELTA = 120
                            w = -buf[3] * 120;
                        }    
                    }
                }
                if (evfMouse)
                {
                    mouse_event(evfMouse, x, y, w, NULL);
                }

                cBytes = 0;
            }
        }
        else
        {
            ERRORMSG(1,(TEXT("Error reading mouse data\r\n")));
        }

        InterruptDone(m_dwSysIntr_Mouse);
    }

    return TRUE;
}

/// <summary>
///  The Interrupt Service Thread C stub
/// </summary>
/// <param name="pPS2Mouse"></param>
/// <returns>
///  A Thread Exit Code
/// </returns>
DWORD
Ps2MouseIntThread(
    Ps2Mouse    *pPS2Mouse
    )
{
    pPS2Mouse->IntThreadProc();

    return 0;
}

/// <summary>
///  Starts the mouse Interrupt Service Thread
/// </summary>
/// <returns>
///  TRUE if successful
/// </returns>
BOOL
Ps2Mouse::
IntThreadStart(
    void
    )
{
    HANDLE  hThrd;

    if (InitMouseResources() == FALSE)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    hThrd = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Ps2MouseIntThread), 
        this, CREATE_SUSPENDED, NULL);

    CeSetThreadAffinity(hThrd, 1);
    ResumeThread(hThrd);

    // Since we don't need the handle, close it now.
    CloseHandle(hThrd);
    return TRUE;
}
