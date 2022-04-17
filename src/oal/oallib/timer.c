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
#include <windows.h>
#include <oemglobal.h>
#include <nkexport.h>
#include "mmio.h"

// ---------------------------------------------------------------------------
// OEMGetRealTime: REQUIRED
//
// This function is called by the kernel to retrieve the time from the
// real-time clock.
//

BOOL OEMGetRealTime(LPSYSTEMTIME lpst)
{
	DWORD timestamp = mmio_read(RTC_BASE + RTCDR);
	DWORD days_count[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	DWORD mdays;
	DWORD isleap;
	DWORD yearsecs;

	memset(lpst, 0, sizeof(SYSTEMTIME));
	lpst->wYear = 1970;

	while (1) {
		isleap = ((lpst->wYear % 4) == 0 && ((lpst->wYear % 100) != 0 || (lpst->wYear % 400) == 0));
		yearsecs = 86400 * (365 + isleap);
		if (timestamp >= yearsecs) {
			timestamp -= yearsecs;
			lpst->wYear++;
		} else
			break;
	}

	while (timestamp >= 86400) {
		timestamp -= 86400;

		lpst->wDay++;

		mdays = days_count[lpst->wMonth];
		if (lpst->wMonth == 1 && isleap)
			mdays++;

		if (lpst->wDay == mdays) {
			lpst->wDay = 0;
			lpst->wMonth++;
		}

		if (lpst->wMonth == 12) {
			lpst->wMonth = 0;
			lpst->wYear++;
		}
	}
	
	lpst->wMonth++;
	lpst->wDay++;

	lpst->wHour = timestamp / 3600;
	lpst->wMinute = (timestamp % 3600) / 60;
	lpst->wSecond = (timestamp % 3600) % 60;

	//DEBUGMSG(1, (TEXT("OEMGetRealTime %u-%u-%u %u %u:%u:%u\r\n"), lpst->wYear, lpst->wMonth, lpst->wDay, lpst->wDayOfWeek, lpst->wHour, lpst->wMinute, lpst->wSecond));

	return TRUE;
}

// ---------------------------------------------------------------------------
// OEMSetRealTime: REQUIRED
//
// This function is called by the kernel to set the real-time clock.
//
BOOL OEMSetRealTime(LPSYSTEMTIME lpst)
{
  // Fill in timer code here.
	DEBUGMSG(1, (TEXT("OEMSetRealTime\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMSetAlarmTime: REQUIRED
//
// This function is called by the kernel to set the real-time clock alarm.
//
BOOL OEMSetAlarmTime(LPSYSTEMTIME lpst)
{
  // Fill in timer code here.
	DEBUGMSG(1, (TEXT("OEMSetAlarmTime\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMQueryPerformanceCounter: OPTIONAL
//
// This function retrieves the current value of the high-resolution
// performance counter.
//
// This function is optional.  Generally, it should be implemented for
// platforms that provide timer functions with higher granularity than
// OEMGetTickCount.
//
BOOL OEMQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
  // Fill in high-resolution timer code here.
	DEBUGMSG(1, (TEXT("OEM_QPC\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMQueryPerformanceFrequency: OPTIONAL
//
// This function retrieves the frequency of the high-resolution
// performance counter.
//
// This function is optional.  Generally, it should be implemented for
// platforms that provide timer functions with higher granularity than
// OEMGetTickCount.
//
BOOL OEMQueryPerformanceFrequency(LARGE_INTEGER* lpFrequency)
{
  // Fill in high-resolution timer frequency code here.
	DEBUGMSG(1, (TEXT("OEM_QFC\r\n")));

  return TRUE;
}

// ---------------------------------------------------------------------------
// OEMGetTickCount: REQUIRED
//
// For Release configurations, this function returns the number of
// milliseconds since the device booted, excluding any time that the system
// was suspended. GetTickCount starts at zero on boot and then counts up from
// there.
//
// For debug configurations, 180 seconds is subtracted from the
// number of milliseconds since the device booted. This enables code that
// uses GetTickCount to be easily tested for correct overflow handling.
//
DWORD OEMGetTickCount(void)
{
	//DEBUGMSG(1, (TEXT("OEMGetTickCount %u\r\n"), CurMSec));
	return CurMSec;
}

// ---------------------------------------------------------------------------
// OEMUpdateReschedTime: OPTIONAL
//
// This function is called by the kernel to set the next reschedule time.  It
// is used in variable-tick timer implementations.
//
// It must set the timer hardware to interrupt at dwTick time, or as soon as
// possible if dwTick has already passed.  It must save any information
// necessary to calculate the g_pNKGlobal->dwCurMSec variable correctly.
//
uint32_t update_timer(void);
VOID OEMUpdateReschedTime(DWORD dwTick)
{
	DWORD interval = 1;
	DWORD i, z, us;

	//for (i = 0; i < 50000; i++)
	//	z = mmio_read(SP804_BASE + TimerValue);

	us = update_timer();
	//DEBUGMSG(1, (TEXT("OEMUpdateReschedTime %u, %u\r\n"), CurMSec, dwTick));

	if (dwTick > CurMSec)
		interval = (dwTick - CurMSec) * 1000 - us;

	mmio_write(SP804_BASE2 + TimerLoad, interval);
	mmio_write(SP804_BASE2 + TimerControl, (1 << 7) | (1 << 5) | (1 << 1) | (1 << 0)); // enable, interrupt, 32bit, oneshot
}

// ---------------------------------------------------------------------------
// OEMRefreshWatchDog: OPTIONAL
//
// This function is called by the kernel to refresh the hardware watchdog
// timer.
//
void OEMRefreshWatchDog(void)
{
  // Fill in watchdog code here.
	DEBUGMSG(1, (TEXT("OEMRefreshWatchDog\r\n")));

  return;
}


