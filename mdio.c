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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "jtag.h"
#include "zynq.h"

static JTAG *jtag;

void jconnect(void) {
	if (!(jtag = jtag_open())) {
		fprintf(stderr, "jconnect: cannot open usb jtag ifc\n");
		exit(-1);
	}
	if (jtag_enumerate(jtag) <= 0) {
		fprintf(stderr, "jconnect: cannot enumerate jtag devices\n");
		exit(-1);
	}
	if (jtag_select(jtag, 0x13722093) && jtag_select(jtag, 0x13631093)) {
		fprintf(stderr, "jconnect: cannot connect to ZYNQ\n");
		exit(-1);
	}

	jtag_ir_wr(jtag, XIL_USER4);
}

u32 jrd(u32 addr) {
	u64 u;
	jtag_dr_io(jtag, 4, addr & 7, &u);
	jtag_dr_io(jtag, 36, 0, &u);
	return (u32) u;
}

void jwr(u32 addr, u32 val) {
	u64 u = ((u64)val) | (((u64) (addr & 7)) << 32) | 0x800000000ULL;
	jtag_dr_io(jtag, 36, u, &u);
}

int main(int argc, char **argv) {

	if (argc == 3) {
		u32 addr = strtoul(argv[1], 0, 0);
		u32 val = strtoul(argv[2], 0, 0);
		jconnect();
		jwr(3, 0xFFFFFFFF);
		usleep(10000);
		jwr(3, 0x50820000 | ((addr & 31) << 18) | (val & 0xFFFF));
		return 0;
	} else if (argc == 2) {
		u32 addr = strtoul(argv[1], 0, 0);
		u32 val;
		jconnect();
		jwr(3, 0xFFFFFFFF);
		usleep(10000);
		jwr(3, 0x60800000 | ((addr & 31) << 18));
		usleep(10000);
 		val = jrd(3);
		printf("%08x\n", val);
		return 0;
	} else {
		fprintf(stderr,
			"usage:\n"
			"  write:  debug <addr> <val>\n"
			"   read:  debug <addr>\n");
		return -1;
	}
}
