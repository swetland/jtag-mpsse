
/* ARM DAP Controller (4bit IR) */
#define DAP_IR_SIZE		4

#define DAP_ABORT		0x08
#define DAP_DPACC		0x0A
#define DAP_APACC		0x0B
#define DAP_IDCODE		0x0E
#define DAP_BYPASS		0x0F


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

