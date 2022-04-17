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
#include <tchddi.h>

// ---------------------------------------------------------------------------
// dpCurSettings: OPTIONAL
//
// The variable enables the drivers internal debug zones so that debug
// messaging macros like DEBUGMSG can be used.  dpCurSettings is a set of
// names for debug zones which are enabled or disabled by the bitfield
// parameter at the end.  There are 16 zones, each with a customizable name.
//
// If dpCurSettings is not implemented, the DEBUGREGISTER macro must be
// removed from the Init function.
//
#ifdef DEBUG
#define ZONE_INIT       DEBUGZONE(0)
#define ZONE_ERROR      DEBUGZONE(1)
#define ZONE_WARN       DEBUGZONE(2)
#define ZONE_IO         DEBUGZONE(3)
#define ZONE_INFO       DEBUGZONE(4)

DBGPARAM dpCurSettings = 
{
  // Name of the debug module
  TEXT("virtio_tablet"), 
  {
    // Names of the individual zones
    TEXT("Init"),       TEXT("Error"),      TEXT("Warning"),    TEXT("IO"), TEXT("Info")
  },
  // Bitfield controlling the zones.  1 means the zone is enabled, 0 disabled
  0x17
};
#endif

// ---------------------------------------------------------------------------
// DllMain: REQUIRED
//
// This function is the first function that is called when the stream driver
// loads (DLL_PROCESS_ATTACH case).  It is also called when the driver unloads
// (DLL_PROCESS_DETACH case).  It should do minimal handling and setup for the
// driver; the majority of initialization occurs later, in the Init function.
// Any initialization that can be delayed until the Init function, should be.
//
extern "C" BOOL WINAPI DllMain(
  HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved
)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:

      // This function registers the dpCurSettings global variable
      // with the kernel debug system.  This allows the DEBUGMSG
      // macro to be used to output debug information from
      // the driver.
      //
      // If dpCurSettings is not implemented, this line should be removed.
      DEBUGREGISTER((HINSTANCE) hinstDLL);

      // This turns off the DLL_THREAD_ATTACH and DLL_THREAD_ATTACH
      // notifications for the driver.  Unless a driver is interested
      // in tracking resource use at the thread level, it is good
      // practice to call this function.
      DisableThreadLibraryCalls ((HMODULE) hinstDLL);

      // Fill in driver load code here
      break;

    case DLL_PROCESS_DETACH:

      // Fill in driver unload code here
      break;
  }
     
  return TRUE;
}

PFN_TOUCH_PANEL_CALLBACK_EX gwes_callback;
HANDLE ist_thread;

extern "C" BOOL init(void);
extern "C" void poll(void);
extern "C" void close(void);

extern "C" BOOL TouchPanelEnable(
	PFN_TOUCH_PANEL_CALLBACK pfnCallback)
{
	DEBUGMSG(ZONE_ERROR, (TEXT("virtio_tablet: legacy callback not supported\r\n")));
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

DWORD UpdateThread(void *arg)
{
	while (1) {
		poll();
	}

/*
	// [HKEY_LOCAL_MACHINE\SYSTEM\GWE\UserInput]
	// "TouchInputTimeout"=dword:xxxx ; where xxxx is in milliseconds

	CETOUCHINPUT input;
	memset(&input, 0, sizeof(CETOUCHINPUT));
	input.x = (long long)input_ready.x * screen_w / (32768 / TOUCH_SCALING_FACTOR);
	input.y = (long long)input_ready.y * screen_h / (32768 / TOUCH_SCALING_FACTOR);
	input.dwID = 0;
	input.hSource = (HANDLE)1;
	input.dwFlags = TOUCHEVENTF_PRIMARY | TOUCHEVENTF_CALIBRATED;

	if (prev_button && input_ready.button)
		input.dwFlags |= TOUCHEVENTF_MOVE | TOUCHEVENTF_INRANGE;
	if (!prev_button && input_ready.button)
		input.dwFlags |= TOUCHEVENTF_DOWN | TOUCHEVENTF_INRANGE;
	if (prev_button && !input_ready.button)
		input.dwFlags |= TOUCHEVENTF_UP;

	if (prev_button || input_ready.button)
		gwes_callback(1, &input, sizeof(CETOUCHINPUT));

	prev_button = input_ready.button;
*/
}

extern "C" BOOL TouchPanelEnableEx(
	PFN_TOUCH_PANEL_CALLBACK_EX pfnCallbackEx)
{
	DEBUGMSG(ZONE_INIT, (TEXT("TouchPanelEnableEx\r\n")));
	gwes_callback = pfnCallbackEx;

	int screen_w = GetSystemMetrics(SM_CXSCREEN);
	int screen_h = GetSystemMetrics(SM_CYSCREEN);

	BOOL ret = init();
	if (!ret) {
		SetLastError(ERROR_GEN_FAILURE);
		return FALSE;
	}

	ist_thread = CreateThread(NULL, 0, UpdateThread, NULL, CREATE_SUSPENDED, NULL);
	CeSetThreadAffinity(ist_thread, 1);
	ResumeThread(ist_thread);

	return TRUE;
}

extern "C" VOID TouchPanelDisable(VOID)
{
	DEBUGMSG(ZONE_INIT, (TEXT("TouchPanelDisable\r\n")));
	close();
}

/* Register a new callback function */
extern "C" PFN_TOUCH_PANEL_CALLBACK_EX TouchPanelRegisterCallback(
	PFN_TOUCH_PANEL_CALLBACK_EX pfnCallbackEx)
{
	DEBUGMSG(ZONE_INIT, (TEXT("TouchPanelRegisterCallback\r\n")));
	PFN_TOUCH_PANEL_CALLBACK_EX old = gwes_callback;
	gwes_callback = pfnCallbackEx;
	return old;
}

extern "C" BOOL TouchPanelGetDeviceCaps(
	INT iIndex,
	LPVOID lpOutput)
{
	DEBUGMSG(ZONE_INFO, (TEXT("TouchPanelGetDeviceCaps\r\n")));
	if (iIndex == TPDC_SAMPLE_RATE_ID) {
		TPDC_SAMPLE_RATE *out = (TPDC_SAMPLE_RATE*)lpOutput;
		out->SamplesPerSecondLow = 1;
		out->SamplesPerSecondHigh = 1;
		out->CurrentSampleRateSetting = 1;
	} else if (iIndex == TPDC_CALIBRATION_POINT_COUNT_ID) {
		TPDC_CALIBRATION_POINT_COUNT *out = (TPDC_CALIBRATION_POINT_COUNT*)lpOutput;
		out->flags = 0;
		out->cCalibrationPoints = 0;
	} else if (iIndex == TPDC_DEVICE_TYPE_ID) {
		TPDC_DEVICE_TYPE *out = (TPDC_DEVICE_TYPE*)lpOutput;
		out->touchType = TOUCHTYPE_SINGLETOUCH;
		out->dwTouchInputMask = 0;
	} else {
		DEBUGMSG(ZONE_WARN, (TEXT("virtio_tablet: requested unknown caps\r\n")));
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	return TRUE;
}

extern "C" BOOL TouchPanelSetMode(
	INT     iIndex,
	LPVOID  lpInput)
{
	DEBUGMSG(ZONE_INFO, (TEXT("TouchPanelSetMode\r\n")));
	return TRUE;
}

extern "C" VOID TouchPanelPowerHandler(
	BOOL bOff)
{
	DEBUGMSG(ZONE_INFO, (TEXT("TouchPanelPowerHandler\r\n")));
}

extern "C" BOOL TouchPanelSetCalibration(
	INT32   cCalibrationPoints,     //The number of calibration points
	INT32   *pScreenXBuffer,        //List of screen X coords displayed
	INT32   *pScreenYBuffer,        //List of screen Y coords displayed
	INT32   *pUncalXBuffer,         //List of X coords collected
	INT32   *pUncalYBuffer)         //List of Y coords collected
{
	DEBUGMSG(ZONE_INFO, (TEXT("TouchPanelSetCalibration\r\n")));
	return TRUE;
}

extern "C" BOOL TouchPanelReadCalibrationPoint(
	INT *pRawX,
	INT *pRawY)
{
	DEBUGMSG(ZONE_WARN, (TEXT("TouchPanelReadCalibrationPoint\r\n")));
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

extern "C" VOID TouchPanelReadCalibrationAbort(VOID)
{
}

extern "C" VOID TouchPanelCalibrateAPoint(
	INT32 UncalX,
	INT32 UncalY,
	INT32* pCalX,
	INT32* pCalY)
{
	DEBUGMSG(ZONE_INFO, (TEXT("TouchPanelCalibrateAPoint\r\n")));
	*pCalX = UncalX;
	*pCalY = UncalY;
}
