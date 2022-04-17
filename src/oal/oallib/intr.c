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
#include <nkintr.h>
#include <nkexport.h>
#include "mmio.h"

unsigned int irq_mapping[] = {
	44, // SYSINTR_FIRMWARE + 0, keyboard
	45, // SYSINTR_FIRMWARE + 1, mouse
	72, // SYSINTR_FIRMWARE + 2, virtio0
	73, // SYSINTR_FIRMWARE + 3, virtio1
	74, // SYSINTR_FIRMWARE + 4, virtio2
	75, // SYSINTR_FIRMWARE + 5, virtio3
	37, // SYSINTR_FIRMWARE + 6, uart0
	38, // SYSINTR_FIRMWARE + 7, uart1
	39, // SYSINTR_FIRMWARE + 8, uart2
	40, // SYSINTR_FIRMWARE + 9, uart3
	43, // SYSINTR_FIRMWARE + 10, audio
};

#define SYS_IRQS 11

unsigned int irq_iar[SYS_IRQS];
unsigned int irq_armed[SYS_IRQS];

// ---------------------------------------------------------------------------
// OEMInterruptEnable: REQUIRED
//
// This function performs all hardware operations necessary to enable the
// specified hardware interrupt.
//
BOOL OEMInterruptEnable(DWORD dwSysIntr, LPVOID pvData, DWORD cbData)
{
	DEBUGMSG(1, (TEXT("OEMInterruptEnable %u\r\n"), dwSysIntr));

	if (dwSysIntr >= SYSINTR_FIRMWARE && dwSysIntr < SYSINTR_FIRMWARE + SYS_IRQS) {
		int i = dwSysIntr - SYSINTR_FIRMWARE;
		irq_armed[i] = 1;
		gic_irq_enable(irq_mapping[i]);
		return TRUE;
	}

	return FALSE;
}

// ---------------------------------------------------------------------------
// OEMInterruptDisable: REQUIRED
//
// This function performs all hardware operations necessary to disable the
// specified hardware interrupt.
//
void OEMInterruptDisable(DWORD dwSysIntr)
{
	DEBUGMSG(1, (TEXT("OEMInterruptDisable %u\r\n"), dwSysIntr));

	if (dwSysIntr >= SYSINTR_FIRMWARE && dwSysIntr < SYSINTR_FIRMWARE + SYS_IRQS) {
		int i = dwSysIntr - SYSINTR_FIRMWARE;
		irq_armed[i] = 0;
		gic_irq_disable(irq_mapping[i]);
	}
}

// ---------------------------------------------------------------------------
// OEMInterruptDone: REQUIRED
//
// This function signals completion of interrupt processing.  This function
// should re-enable the interrupt if the interrupt was previously masked.
//
void OEMInterruptDone(DWORD dwSysIntr)
{
	if (dwSysIntr >= SYSINTR_FIRMWARE && dwSysIntr < SYSINTR_FIRMWARE + SYS_IRQS) {
		int i = dwSysIntr - SYSINTR_FIRMWARE;
		if (irq_armed[i]) {
			DEBUGMSG(1, (TEXT("unexpected IRQ arming %u\r\n"), irq_mapping[i]));
		} else {
			irq_armed[i] = 1;
			mmio_write(GIC_BASE + GICC_DIR, irq_iar[i]);
		}
	}
}

// ---------------------------------------------------------------------------
// OEMInterruptMask: REQUIRED
//
// This function masks or unmasks the interrupt according to its SysIntr
// value.
//
void OEMInterruptMask(DWORD dwSysIntr, BOOL fDisable)
{
	if (fDisable)
		OEMInterruptDisable(dwSysIntr);
	else
		OEMInterruptEnable(dwSysIntr, NULL, 0);
}

uint32_t timeUS;
uint32_t lastTimer;

uint32_t update_timer(void)
{
	uint32_t msec;
	uint32_t timer = mmio_read(SP804_BASE + TimerValue);

	if (timer < lastTimer) { // normal decrement
		timeUS += lastTimer - timer;
	}
	if (timer > lastTimer) { // overflow
		timeUS += lastTimer; // remainder from previous value
		timeUS += 1000000 - timer;
	}

	lastTimer = timer;

	msec = timeUS / 1000;
	timeUS -= msec * 1000;
	CurMSec += msec;

	return timeUS;
}

DWORD Read_MPIDR(void);

// ---------------------------------------------------------------------------
// OEMInterruptHandler: REQUIRED for ARM, UNUSED for other cpus
//
// This function handles the standard ARM interrupt, providing all ISR
// functionality for ARM-based platforms.
//
DWORD OEMInterruptHandler(DWORD dwEPC)
{
	unsigned int i;
	unsigned int iar = mmio_read(GIC_BASE + GICC_IAR);
	unsigned int irq = iar & 0x3FF;

	DWORD ret = SYSINTR_NOP;
	DWORD known = 0;

	if (irq == 1023) // spurious
		return ret;

	if (irq == TIMER_IRQ) {
		known = 1;
		update_timer();
		if (mmio_read(SP804_BASE + TimerMIS)) {
			mmio_write(SP804_BASE + TimerIntClr, 1);
		}
		if (mmio_read(SP804_BASE2 + TimerMIS)) {
			mmio_write(SP804_BASE2 + TimerIntClr, 1);
			ret = SYSINTR_RESCHED;
		}
	}

	if (irq == RTC_IRQ) {
		known = 1;
		mmio_write(RTC_BASE + RTCICR, 1);
		ret = SYSINTR_RTC_ALARM;
	}

	if (irq == IPI_IRQ) {
		known = 1;
		ret = SYSINTR_IPI;
	}

	if (!known)
	{
		for (i = 0; i < SYS_IRQS; i++) {
			if (irq == irq_mapping[i]) {
				if (!irq_armed[i])
					DEBUGMSG(1, (TEXT("unexpected IRQ trigger %u\r\n"), irq));

				irq_iar[i] = iar;
				irq_armed[i] = 0;

				known = 1;
				ret = SYSINTR_FIRMWARE + i;
				break;
			}
		}
	}

	if (!known)
		DEBUGMSG(1, (TEXT("unexpected IRQ %u\r\n"), irq));

	if (known && ret != SYSINTR_IPI && Read_MPIDR() != 0)
		DEBUGMSG(1, (TEXT("IRQ on other CPU: %u\r\n"), irq));

	mmio_write(GIC_BASE + GICC_EOIR, iar);

	if (ret < SYSINTR_FIRMWARE)
		mmio_write(GIC_BASE + GICC_DIR, iar);

	return ret;
}

// ---------------------------------------------------------------------------
// OEMInterruptHandlerFIQ: REQUIRED for ARM, UNUSED for other cpus
//
// This function handles the fast-interrupt request (FIQ) ARM interrupt,
// providing all FIQ ISR functionality for ARM-based platforms.
//
void OEMInterruptHandlerFIQ(void)
{
	DEBUGMSG(1, (TEXT("unexpected FIQ\r\n")));
}
