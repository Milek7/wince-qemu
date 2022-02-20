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

	INCLUDE kxarm.h

	IMPORT  KernelStart

	TEXTAREA

; ---------------------------------------------------------------------------
; StartUp: REQUIRED
;
; This function is the first function that is called in Windows CE after the
; bootloader runs.  It is the entry point of oal.exe (also known as nk.exe),
; which is the kernel process.  Traditionally it is named StartUp, but its
; name can be changed provided it matches the EXEENTRY in the SOURCES file
; where oal.exe is linked.
; 
; This function is repsonsible for performing hardware initialization of the
; CPU, memory controller, clocks, serial port, and caches / TLBs.  For ARM
; and x86 CPUs it is responsible for loading the OEMAddressTable into memory
; for use by the KernelStart function.

    EXPORT StartUp
    LEAF_ENTRY StartUp

;---------------------------------------------------------------
; Jump to WinCE KernelStart
;---------------------------------------------------------------
; Compute the OEMAddressTable's physical address and 
; load it into r0. KernelStart expects r0 to contain
; the physical address of this table. The MMU isn't 
; turned on until well into KernelStart.  

	ldr r0, =0xf00000
	mcr p15, 0, r0, c1, c0, 2
	ldr sp, =0x15000000
	mov     r0, #0
    add     r0, pc, #g_oalAddressTable - (. + 8)
    bl      KernelStart
    b       .

    ; Include memory configuration file with g_oalAddressTable
    INCLUDE addrtab_cfg.inc

    ENTRY_END

	END
