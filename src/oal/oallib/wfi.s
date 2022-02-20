    INCLUDE kxarm.h
    TEXTAREA

    EXPORT _wfi
    LEAF_ENTRY _wfi

    mrs r0, cpsr
    cpsid i
    wfi
    msr cpsr, r0
    bx lr
    
    ENTRY_END

    END
