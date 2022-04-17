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
    EXPORT  StartUp

	IMPORT  g_numExtraCPUs
	IMPORT  g_pfnNkContinue
	IMPORT  g_controlRegs
	IMPORT  PaFromVa

    EXPORT  Read_ControlRegs
    EXPORT  Read_MPIDR
    EXPORT  Do_DSB

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

    LEAF_ENTRY StartUp

	; enable VFP
	ldr r0, =0xf00000
	mcr p15, 0, r0, c1, c0, 2

	; read MPIDR
	mrc p15, 0, r0, c0, c0, 5
	ands r0, r0, #3
	beq CONT ; continue booting as CPU 0

	; otherwise, setup GIC and wait in WFI loop

	ldr r0, =0x2c001100
	mov r1, #3
	str r1, [r0] ; GICD_ISENABLER0

	ldr r0, =0x2c002000
	ldr r1, =0x201
	str r1, [r0] ; GICC_CTLR

	ldr r0, =0x2c002004
	ldr r1, =0xF0
	str r1, [r0] ; GICC_PMR

WAIT
	wfi
	dsb

	ldr r0, =0x2c00200c
	ldr r1, [r0] ; GICC_IAR

	ldr r3, =0x3FF
	ands r2, r1, r3
	bne WAIT

	ldr r0, =0x2c002010
	str r1, [r0] ; GICC_EOIR
	ldr r0, =0x2c003000
	str r1, [r0] ; GICC_DIR
	
	; startup requested, prepare core for usage
	ldr     r0, =g_controlRegs
	mov     r1, #0
    add     r1, pc, #g_oalAddressTable - (. + 8) + 12
    bl      PaFromVa
	mov     r4, r0
   
    ; setup AUX control register
    ldr     r0, [r4, #0x8]              ; (r0) = Master CPU's AUX Control Register settings
    mcr     p15, 0, r0, c1, c0, 1

    ; Set the TTBR0 (Page table)
    ldr     r0, [r4, #0x0]              ; (r0) = Master CPU's TTBR0
    mcr     p15, 0, r0, c2, c0, 0

    ; Setup access to domain 0 and clear other 
    ldr     r0, [r4, #0xc]              ; (r0) = Master CPU's Domain Control Register settings
    mcr     p15, 0, r0, c3, c0, 0   

    ; Enable MMU
    ldr     r1, [r4, #0x4]              ; (r1) = Master CPU's MMU Control Register settings
    ldr     r2, =VIRTUALSMP             ; Get virtual address of 'VIRTUALSMP' label.
    cmp     r2, #0                      ; Make sure no stall on "mov pc,r2" below.
    mcr     p15, 0, r1, c1, c0, 0       ; MMU ON: All memory accesses are now virtual.

    ; Jump to the virtual address of the 'VIRTUALSMP' label.
    mov     pc, r2

VIRTUALSMP
    ldr     r1, =g_numExtraCPUs
    ldrex   r2, [r1]
    add     r2, r2, #0x1
    strex   r3, r2, [r1]
    cmp     r3, #0x0
    bne     VIRTUALSMP
    
    ldr     r0, =g_pfnNkContinue
    ldr     r0, [r0]
    mov     pc, r0

CONT
	; set temporary stack pointer
	ldr sp, =0x15000000

	mov     r0, #0
    add     r0, pc, #g_oalAddressTable - (. + 8)
    b       KernelStart

    ; Include memory configuration file with g_oalAddressTable
    INCLUDE addrtab_cfg.inc

    ENTRY_END

	LEAF_ENTRY  Read_ControlRegs
	mrc     p15, 0, r1, c2, c0, 0 ; TTBR0
	str     r1, [r0, #0x0]
	mrc     p15, 0, r1, c1, c0, 0 ; MMU
	str     r1, [r0, #0x4]
	mrc     p15, 0, r1, c1, c0, 1 ; AUX
	str     r1, [r0, #0x8]
	mrc     p15, 0, r1, c3, c0, 0 ; Domain
	str     r1, [r0, #0xc]
	bx      lr
	ENTRY_END

    LEAF_ENTRY  Read_MPIDR
	mrc   p15, 0, r0, c0, c0, 5
	and   r0, r0, #3
    bx    lr
    ENTRY_END

    LEAF_ENTRY  Do_DSB
	dsb
    bx    lr
    ENTRY_END

	END
