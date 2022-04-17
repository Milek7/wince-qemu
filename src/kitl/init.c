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
#include <kitl.h>
#include <nkintr.h>

#define OAL_KITL_SERIAL_PACKET      0xAA

static const UINT8 g_kitlSign[] = { 'k', 'I', 'T', 'L' };

#pragma pack (push, 1)
typedef struct {
    UINT32 signature;
    UCHAR  packetType;
    UCHAR  reserved;
    USHORT payloadSize; // packet size, not including this header
    UCHAR  crcData;
    UCHAR  crcHeader;
} OAL_KITL_SERIAL_HEADER;
#pragma pack (pop)

// header checksum size == size of the header minus the checksum itself
#define HEADER_CHKSUM_SIZE      (sizeof (OAL_KITL_SERIAL_HEADER) - sizeof (UCHAR))

static struct {
    UINT8 buf[KITL_MTU];
    UINT32 count;
} kitlState;

static UINT8 ChkSum(UINT8 *pBuffer, UINT16 length)
{
    UINT8 sum = 0;
    int   nLen = length;

    while (nLen -- > 0) sum += *pBuffer++;
    return sum;
}

#define uint32_t unsigned int
#define mmio_write(reg, data)  *(volatile uint32_t*)(reg) = (data)
#define mmio_read(reg)  *(volatile uint32_t*)(reg)

enum
{
	// we need to use mapped address
	UART0_BASE = (0x910b0000),
 
    // The offsets for each register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C)
};

static BOOL SerialSend(UINT8 *pData, UINT16 length)
{
	UINT16 i;

	for (i = 0; i < length; i++)
		mmio_write(UART0_DR, pData[i]);

	return TRUE;
}

static BOOL SerialRecv(UINT8 *pData, UINT16 *pLength)
{
	OAL_KITL_SERIAL_HEADER *pHeader = (OAL_KITL_SERIAL_HEADER*)kitlState.buf;

	while (kitlState.count < sizeof(OAL_KITL_SERIAL_HEADER)) {
		if (mmio_read(UART0_FR) & (1 << 4)) {
			*pLength = 0;
			return FALSE;
		}
		kitlState.buf[kitlState.count++] = (UINT8)mmio_read(UART0_DR);
	}

	if (pHeader->packetType != OAL_KITL_SERIAL_PACKET ||
		pHeader->crcHeader != ChkSum(kitlState.buf, HEADER_CHKSUM_SIZE) ||
		pHeader->payloadSize > KITL_MTU - sizeof(OAL_KITL_SERIAL_HEADER)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("KITL: invalid packet\r\n")));

		kitlState.count = 0;
		*pLength = 0;
		return FALSE;
	}

	while (kitlState.count < sizeof(OAL_KITL_SERIAL_HEADER) + pHeader->payloadSize) {
		if (mmio_read(UART0_FR) & (1 << 4)) {
			*pLength = 0;
			return FALSE;
		}
		kitlState.buf[kitlState.count++] = (UINT8)mmio_read(UART0_DR);
	}

	if (pHeader->crcData != ChkSum(kitlState.buf + sizeof(OAL_KITL_SERIAL_HEADER), pHeader->payloadSize)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("KITL: invalid checksum, dropped\r\n")));

		kitlState.count = 0;
		*pLength = 0;
		return FALSE;
	}

	if (kitlState.count > *pLength) {
		DEBUGMSG(ZONE_ERROR, (TEXT("KITL: receive buffer too small, packet dropped\r\n")));

		kitlState.count = 0;
		*pLength = 0;
		return FALSE;
	}

	memcpy(pData, kitlState.buf, kitlState.count);
	*pLength = kitlState.count;

	kitlState.count = 0;

	return TRUE;
}

static BOOL SerialEncode(UINT8 *pFrame, UINT16 size)
{
    OAL_KITL_SERIAL_HEADER *pHeader = (OAL_KITL_SERIAL_HEADER*)pFrame;

    memcpy(&pHeader->signature, g_kitlSign, sizeof(pHeader->signature));
    pHeader->packetType = OAL_KITL_SERIAL_PACKET;
    pHeader->payloadSize = size;
    pHeader->crcData = ChkSum(pFrame + sizeof(OAL_KITL_SERIAL_HEADER), size);
    pHeader->crcHeader = ChkSum(pFrame, HEADER_CHKSUM_SIZE);
    return TRUE;
}

static UINT8* SerialDecode(UINT8 *pFrame, UINT16 *pSize)
{
    UINT8 *pData;
    
    *pSize -= sizeof(OAL_KITL_SERIAL_HEADER);
    pData = pFrame + sizeof(OAL_KITL_SERIAL_HEADER);
    return pData;
}

static void SerialEnableInt(BOOL enable)
{
	mmio_write(UART0_IMSC, enable ? (1 << 4) : 0);
}

static BOOL SerialGetDevCfg(UINT8 *pBuffer, UINT16 *pSize)
{
    *pSize = 0;
    return TRUE;
}

static BOOL SerialSetHostCfg(UINT8 *pData, UINT16 size)
{
    return TRUE;
}

//------------------------------------------------------------------------------
//
// OEMKitlStartup: REQUIRED
//
// This function is the first OEM code that executes in kitl.dll.  It is called
// by the kernel after the BSP calls KITLIoctl( IOCTL_KITL_STARTUP, ... ) in
// OEMInit().  This function should set up internal state, read any boot
// arguments, and call the KitlInit function to initialize KITL in active
// (immediate load) or passive (delay load) mode.
//
BOOL OEMKitlStartup()
{
	DEBUGMSG(ZONE_INIT, (TEXT("OEMKitlStartup\r\n")));

	return KitlInit(TRUE);
}

// ---------------------------------------------------------------------------
// OEMKitlInit: REQUIRED
//
// This function is called from the kernel to initialize the KITL device and
// KITLTRANSPORT structure when it is time to load KITL.  If OEMKitlStartup
// selects active mode KITL, KitlInit will call this function during boot. If
// OEMKitlStartup selects passive mode KITL, this function will not be called
// until an event triggers KITL load.

// When this function returns, the KITLTRANSPORT structure must contain valid
// variable initializations including valid function pointers for each
// required KITL function.  The KITL transport hardware must also be fully
// initialized.
//
BOOL OEMKitlInit(PKITLTRANSPORT pKitl)
{
	// Fill in init code here.

	DEBUGMSG(ZONE_INIT, (TEXT("OEMKitlInit\r\n")));

	// 8 bit data transmission (1 stop bit, no parity).
	mmio_write(UART0_LCRH, (1 << 5) | (1 << 6));
 
	// Enable UART0, receive & transfer part of UART.
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));

	//pKitl->dwBootFlags = KITL_FL_DBGMSG | KITL_FL_PPSH | KITL_FL_KDBG;
	pKitl->dwBootFlags = 0;
	strcpy(pKitl->szName, "WinCE QEMU");
	pKitl->Interrupt = (UCHAR)(SYSINTR_FIRMWARE + 7);
	pKitl->FrmHdrSize = sizeof(OAL_KITL_SERIAL_HEADER);
	pKitl->FrmTlrSize = 0;
    pKitl->pfnEncode     = SerialEncode;
    pKitl->pfnDecode     = SerialDecode;
    pKitl->pfnSend       = SerialSend;
    pKitl->pfnRecv       = SerialRecv;
    pKitl->pfnEnableInt  = SerialEnableInt;
    pKitl->pfnGetDevCfg  = SerialGetDevCfg;
    pKitl->pfnSetHostCfg = SerialSetHostCfg;

	return TRUE;
}

// ---------------------------------------------------------------------------
// dpCurSettings: REQUIRED
//
// This variable defines debug zones usable by the kernel and this
// implementation.  This is the operating system's standard
// mechanism for debug zones.
//
DBGPARAM dpCurSettings = {
    TEXT("KITL"), {
    TEXT("Warning"),    TEXT("Init"),       TEXT("Frame Dump"),     TEXT("Timer"),
    TEXT("Send"),       TEXT("Receive"),    TEXT("Retransmit"),     TEXT("Command"),
    TEXT("Interrupt"),  TEXT("Adapter"),    TEXT("LED"),            TEXT("DHCP"),
    TEXT("OAL"),        TEXT("Ethernet"),   TEXT("Unused"),         TEXT("Error"), },
    ZONE_WARNING | ZONE_INIT | ZONE_ERROR,
};
