/* Copyright 2014 Brian Swetland <swetland@frotz.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>

#include "jtag.h"
#include "zynq.h"

void pause(const char *msg) {
	char crap[1024];
	fprintf(stderr,">>> %s <<<\n", msg);
	fgets(crap, sizeof(crap), stdin);
}

int jtag_dp_rd(JTAG *jtag, u32 addr, u32 *val) {
	int retry = 30;
	u64 u;
	jtag_ir_wr(jtag, DAP_IR_DPACC);
	jtag_dr_io(jtag, 35, XPACC_RD(addr), NULL);
	while (retry-- > 0) {
		jtag_dr_io(jtag, 35, XPACC_RD(DPACC_RDBUFF), &u);
		switch (XPACC_STATUS(u)) {
		case XPACC_OK:
			*val = u >> 3;
			return 0;
		case XPACC_WAIT:
			fprintf(stderr,"!");
			continue;
		default:
			return -1;
		}
	}
	return -1;
}

int jtag_dp_wr(JTAG *jtag, u32 addr, u32 val) {
	int retry = 30;
	u64 u;
	u = XPACC_WR(addr);
	u |= (((u64) val) << 3);
	jtag_ir_wr(jtag, DAP_IR_DPACC);
	jtag_dr_io(jtag, 35, u, NULL);
	while (retry-- > 0) {
		jtag_dr_io(jtag, 35, XPACC_RD(DPACC_RDBUFF), &u);
		switch (XPACC_STATUS(u)) {
		case XPACC_OK:
			return 0;
		case XPACC_WAIT:
			fprintf(stderr,"!");
			continue;
		default:
			return -1;
		}
	}
	return -1;
}

/* TODO: cache state, check errors */
int jtag_ap_rd(JTAG *jtag, u32 addr, u32 *val) {
	u64 u;
	jtag_dp_wr(jtag, DPACC_SELECT, DPSEL_APSEL(0) | DPSEL_APBANKSEL(addr));
	jtag_ir_wr(jtag, DAP_IR_APACC);
	jtag_dr_io(jtag, 35, XPACC_RD(addr), NULL);
	jtag_ir_wr(jtag, DAP_IR_DPACC);
	jtag_dr_io(jtag, 35, XPACC_RD(DPACC_RDBUFF), &u);
	*val = (u >> 3);
	return 0;
}

int jtag_ap_wr(JTAG *jtag, u32 addr, u32 val) {
	u64 u;
	jtag_dp_wr(jtag, DPACC_SELECT, DPSEL_APSEL(0) | DPSEL_APBANKSEL(addr));
	jtag_ir_wr(jtag, DAP_IR_APACC);
	u = XPACC_WR(addr);
	u |= (((u64) val) << 3);
	jtag_dr_io(jtag, 35, u, NULL);
	jtag_ir_wr(jtag, DAP_IR_DPACC);
	jtag_dr_io(jtag, 35, XPACC_RD(DPACC_RDBUFF), &u);
	return 0;
}

int main(int argc, char **argv) {
	JTAG *jtag;
	unsigned n;
	u64 u;
	u32 x;

	if (!(jtag = jtag_open()))
		return -1;
	if ((n = jtag_enumerate(jtag)) <= 0)
		return -1;

	if (jtag_select(jtag, 0x4ba00477))
		return -1;

	if (jtag_ir_wr(jtag, DAP_IR_IDCODE))
		return -1;
	if (jtag_dr_io(jtag, 32, 0, &u))
		return -1;
	fprintf(stderr,"idcode? %08lx\n", u);

	jtag_dp_rd(jtag, DPACC_CSW, &x);
	fprintf(stderr,"CSW %08x\n", x);

	jtag_dp_wr(jtag, DPACC_CSW, DPCSW_CSYSPWRUPREQ | DPCSW_CDBGPWRUPREQ);
	do {
		jtag_dp_rd(jtag, DPACC_CSW, &x);
		fprintf(stderr,"CSW %08x\n", x);
	} while ((x & (DPCSW_CSYSPWRUPACK | DPCSW_CDBGPWRUPACK)) != (DPCSW_CSYSPWRUPACK | DPCSW_CDBGPWRUPACK));

	jtag_ap_rd(jtag, APACC_BASE, &x);
	fprintf(stderr,"base %08x\n", x);

	jtag_ap_rd(jtag, APACC_IDR, &x);
	fprintf(stderr,"idr %08x\n", x);

#if STRESSTEST
	for (n = 0; n < 0xFFFFFFFF; n++) { 
		u32 z = (n >> 16) | (n << 16);
		x = 0xeeeeeeee;
		jtag_ap_wr(jtag, APACC_TAR, z);
		jtag_ap_rd(jtag, APACC_TAR, &x);
		if (z != x) {
			fprintf(stderr,"%08x != %08x\n", z, x);
		}
		//if ((n & 0xFFF) == 0)
			fprintf(stderr,"%08x\r", z);
	}

	jtag_ap_wr(jtag, APACC_TAR, 0x12345678);
#endif

	jtag_ap_rd(jtag, APACC_TAR, &x);
	fprintf(stderr,"tar %08x\n", x);

#if 0
	fprintf(stderr,"done\n");
	jtag_dp_wr(jtag, DPACC_CSW, 0);
#endif

	return 0;
}
