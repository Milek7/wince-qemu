#define uint32_t unsigned int
#define mmio_write(reg, data)  *(volatile uint32_t*)(reg) = (data)
#define mmio_read(reg)  *(volatile uint32_t*)(reg)

enum
{
	GIC_BASE = 0x92000000,
 
    GICD_CTLR = 0x1000,
	GICD_ISENABLER0 = 0x1100,
	GICD_ICENABLER0 = 0x1180,
	GICD_ITARGETSR0 = 0x1800,
	GICD_ICFGR0 = 0x1C00,
	GICD_SGIR   = 0x1F00,

	GICC_CTLR = 0x2000,
	GICC_PMR = 0x2004,
	GICC_IAR = 0x200C,
	GICC_EOIR = 0x2010,
	GICC_DIR = 0x3000
};

enum
{
	SP804_BASE  = 0x91110000,
	SP804_BASE2 = 0x91110020,

    TimerLoad    = 0x00,
	TimerValue   = 0x04,
	TimerControl = 0x08,
	TimerIntClr  = 0x0C,
	TimerRIS     = 0x10,
	TimerMIS     = 0x14,
	TimerBGLoad  = 0x18
};

enum
{
	LCD_BASE  = 0x911f0000,
	VRAM_ADDR = 0x18000000,

	LCDTiming0 = 0x000,
	LCDTiming1 = 0x004,
	LCDUpbase  = 0x010,
	LCDControl = 0x018
};

#define TIMER_IRQ 34

enum
{
	RTC_BASE = 0x91170000,

	RTCDR   = 0x000,
	RTCMR   = 0x004,
	RTCLR   = 0x008,
	RTCCR   = 0x00C,
	RTCIMSC = 0x010,
	RTCRIS  = 0x014,
	RTCMIS  = 0x018,
	RTCICR  = 0x01C
};

#define RTC_IRQ 36
#define IPI_IRQ 1

enum
{
	// we need to use mapped address
	UART0_BASE = (0x91090000),
 
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

void gic_irq_enable(int irq);
void gic_irq_disable(int irq);
