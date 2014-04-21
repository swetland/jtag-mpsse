
/* ARM DAP Controller (4bit IR) */
#define DAP_IR_SIZE		4

#define DAP_IR_ABORT		0x08
#define DAP_IR_DPACC		0x0A
#define DAP_IR_APACC		0x0B
#define DAP_IR_IDCODE		0x0E
#define DAP_IR_BYPASS		0x0F

#define XPACC_STATUS(n)		((n) & 0x3)
#define XPACC_WAIT		0x1
#define XPACC_OK		0x2
#define XPACC_RD(n)		(0x1 | (((n) >> 1) & 6))
#define XPACC_WR(n)		(0x0 | (((n) >> 1) & 6))

#define DPACC_RESERVED		0x0
#define DPACC_CSW		0x4
#define DPACC_SELECT		0x8
#define DPACC_RDBUFF		0xC

#define DPCSW_CSYSPWRUPACK	(1 << 31)
#define DPCSW_CSYSPWRUPREQ	(1 << 30)
#define DPCSW_CDBGPWRUPACK	(1 << 29)
#define DPCSW_CDBGPWRUPREQ	(1 << 28)
#define DPCSW_CDBGRSTACK	(1 << 27)
#define DPCSW_CDBGRSTREQ	(1 << 26)
#define DPCSW_TRNCNT(n)		(((n) & 0x3FF) << 12)
#define DPCSW_MASKLANE(n)	(((n) & 0xF) << 8) // pushed verify or compare
#define DPCSW_WDATAERR		(1 << 7) // reserved on jtag
#define DPCSW_READOK		(1 << 6) // reserved on jtag
#define DPCSW_STICKYERR		(1 << 5)
#define DPCSW_STICKYCMP		(1 << 4)
#define DPCSW_TRNMODE_NORMAL	(0 << 2)
#define DPCSW_TRNMODE_PUSH_VRFY	(1 << 2)
#define DPCSW_TRNMODE_PUSH_CMP	(2 << 2)
#define DPCSW_STICKYORUN	(1 << 1)
#define DPCSW_ORUNDETECT	(1 << 0)

#define DPSEL_APSEL(n)		(((n) & 0xFF) << 24)
#define DPSEL_APBANKSEL(a)	((a) & 0xF0)
#define DPSEL_CTRLSEL		(1 << 0) // reserved on jtag

/* Reading RDBUFF returns 0, has no side effects */
/* Can be used to obtain final read result and ack values at end of seq */

#define APACC_CSW		0x00
#define APACC_TAR		0x04
#define APACC_DRW		0x0C
#define APACC_BD0		0x10
#define APACC_BD1		0x14
#define APACC_BD2		0x18
#define APACC_BD3		0x1C
#define APACC_CFG		0xF4
#define APACC_BASE		0xF8
#define APACC_IDR		0xFC


/* Xilinx TAP Controller (6bit IR) */
#define XIL_IR_SIZE		6

#define XIL_EXTEST		0x00
#define XIL_SAMPLE		0x01
#define XIL_USER1		0x02
#define XIL_USER2		0x03
#define XIL_USER3		0x22
#define XIL_USER4		0x23
#define XIL_CFG_OUT		0x04
#define XIL_CFG_IN		0x05
#define XIL_USERCODE		0x08
#define XIL_IDCODE		0x09
#define XIL_ISC_ENABLE		0x10
#define XIL_ISC_PROGRAM		0x11
#define XIL_ISC_PROGRAM_SECURITY 0x12
#define XIL_ISC_NOOP		0x14
#define XIL_ISC_READ		0x1B
#define XIL_ISC_DISABLE		0x17
#define XIL_BYPASS		0x3F

