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
#include "mmio.h"

volatile int g_numExtraCPUs;
volatile void* g_pfnNkContinue;
volatile DWORD g_controlRegs[4];

void Read_ControlRegs(DWORD*);
DWORD Read_MPIDR(void);
void Do_DSB(void);

// ---------------------------------------------------------------------------
// OEMMpStartAllCPUs: OPTIONAL
//
// This function is called by the kernel before multiprocessing is enabled.
// It runs on the master CPU and starts up all non-master CPUs.  This function
// is required for multiprocessor support.
//
BOOL OEMMpStartAllCPUs(PLONG pnCpus, FARPROC pfnContinue)
{
	DEBUGMSG(1, (TEXT("OEMMpStartAllCPUs\r\n")));

	g_numExtraCPUs = 0;
	g_pfnNkContinue = pfnContinue;
	Read_ControlRegs(g_controlRegs);
	Do_DSB();

	mmio_write(GIC_BASE + GICD_SGIR, (1 << 24));

	while (mmio_read(SP804_BASE + TimerValue) > 500000);
	Do_DSB();

	*pnCpus = g_numExtraCPUs + 1;

	return TRUE;
}

// ---------------------------------------------------------------------------
// OEMSendIPI: OPTIONAL
//
// This function sends an interprocessor interrupt.  It is required for
// multiprocessor support.
//
BOOL OEMSendIPI(DWORD dwType, DWORD dwTarget)
{
	if (dwType == IPI_TYPE_ALL_BUT_SELF) {
		mmio_write(GIC_BASE + GICD_SGIR, (1 << 24) | 1);
	} else if (dwType == IPI_TYPE_ALL_INCLUDE_SELF) {
		mmio_write(GIC_BASE + GICD_SGIR, (1 << 24) | 1);
		mmio_write(GIC_BASE + GICD_SGIR, (1 << 25) | 1);
	} else if (dwType == IPI_TYPE_SPECIFIC_CPU) {
		mmio_write(GIC_BASE + GICD_SGIR, (1 << (16 + dwTarget)) | 1);
	}

	return TRUE;
}
