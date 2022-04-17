;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this sample source code is subject to the terms of the Microsoft
; license agreement under which you licensed this sample source code. If
; you did not accept the terms of the license agreement, you are not
; authorized to use this sample source code. For the terms of the license,
; please see the license agreement between you and Microsoft or, if applicable,
; see the LICENSE.RTF on your install media or the root of your tools installation.
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
;
; Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

;--------------------------------------------------------------
;
;  File:  memory_cfg.inc
;
;  This file is used to define g_oalAddressTable. This table is passed to
;  KernelStart to estabilish physical to virtual memory mapping. This table
;  is used also in memory OAL module to map between physical and virtual
;  memory addresses via OALPAtoVA/OALVAtoPA functions.
;
;  The config.bib file defines image memory layout ant it contains virtual
;  cached memory addresses which must be synchronized with g_oalAddressTable.
;  With each table change make sure that those constant are still valid.
;
;------------------------------------------------------------------------------
;  Export Definition

        EXPORT  g_oalAddressTable[DATA]

;------------------------------------------------------------------------------
;  Table format: cached address, physical address, size

g_oalAddressTable
	DCD     CE_NEW_MAPPING_TABLE, g_oalDeviceTable, 0
    DCD     0x80000000, 0x80000000, 256     ; main RAM
    DCD     0, 0, 0 ; end of table
 
g_oalDeviceTable
	DCD     0x91000000, 0x001C0000, 0x400000, 0 ; periph
	DCD     0x92000000, 0x002C0000, 0x10000, 0 ; GIC
	DCD     0, 0, 0, 0 ; end of table
 
;------------------------------------------------------------------------------

        END
