    INCLUDE kxarm.h
    TEXTAREA

    EXPORT _flush
    LEAF_ENTRY _flush

        ; Invalidate TLB and I cache
        mov     r0, #0                          ; setup up for MCR
        mcr     p15, 0, r0, c8, c7, 0           ; invalidate TLB's
        mcr     p15, 0, r0, c7, c5, 0           ; invalidate icache
        mcr     p15, 0, r0, c7, c14, 0           ; invalidate dcache
        dsb

    bx lr
    
    ENTRY_END

    END
