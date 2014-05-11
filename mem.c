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
#include <fcntl.h>
#include <errno.h>

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
	if (jtag_select(jtag, 0x13722093)) {
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

void axi_connect(void) {
	jconnect();
	if (jrd(0) != 0x4158494d) {
		fprintf(stderr,"axi master not detected\n");
		exit(-1);
	}
}

int main(int argc, char **argv) {
	if ((argc == 4) && (!strcmp(argv[1],"-download"))) {
		u32 addr = strtoul(argv[2], 0, 0);
		int fd = 0;
		int r;
		u32 n;
		if (strcmp(argv[3], "-")) {
			fd = open(argv[3], O_RDONLY);
			if (fd < 0) {
				fprintf(stderr,"cannot open '%s'\n",argv[3]);
				return -1;
			}
		}
		axi_connect();
		jwr(6, addr-4);
		for (;;) {
			r = read(fd, &n, sizeof(n));
			if (r < 0) {
				if (errno == EINTR)
					continue;
				break;
			}
			if (r < 1)
				break;
			jwr(4, n);
		}
		close(fd);
		return 0;
	}
	if ((argc == 5) && (!strcmp(argv[1],"-upload"))) {
		u32 addr = strtoul(argv[2], 0, 0);
		u32 count = strtoul(argv[3], 0, 0) / 4;
		int fd = 1;
		if (strcmp(argv[4], "-")) {
			fd = open(argv[4], O_WRONLY|O_CREAT|O_TRUNC, 0640);
			if (fd < 0) {
				fprintf(stderr,"cannot open '%s'\n",argv[4]);
				return -1;
			}
		}
		axi_connect();
		while (count > 0) {
			jwr(7, addr);
			u32 n = jrd(1);
			write(fd, &n, sizeof(n));
			addr += 4;
			count--;
		}
		close(fd);
		return 0;
	}
	if (argc == 3) {
		u32 addr = strtoul(argv[1], 0, 0);
		u32 val = strtoul(argv[2], 0, 0);
		if (argv[1][0] == '-') goto usage;
		axi_connect();
		jwr(6, addr);
		jwr(5, val);
		return 0;
	} else if (argc == 2) {
		u32 addr = strtoul(argv[1], 0, 0);
		u32 val;
		if (argv[1][0] == '-') goto usage;
		axi_connect();
		jwr(7, addr);
 		val = jrd(1);
		printf("%08x\n", val);
		return 0;
	}

usage:
	fprintf(stderr,
		"usage:\n"
		"  write:  mem <addr> <val>\n"
		"   read:  mem <addr>\n"
		"  write:  mem -download <addr> <filename>\n"
		"   read:  mem -upload <addr> <length> <filename>\n");
		return -1;
}
