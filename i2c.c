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

u32 jrd(void) {
	u64 u;
	jtag_dr_io(jtag, 32, 0, &u);
	return (u32) u;
}

u32 jwr(u32 n) {
	u64 u;
	u = n;
	u |= 0x100000000ULL;
	jtag_dr_io(jtag, 33, u, &u);
	return (u32) u;
}

//              0x00XX // wo - data to write
//              0x00XX // ro - last data read
#define STA	0x0100 // rw - issue start on next rd or wr
#define STP	0x0200 // rw - issue stop
#define WR	0x0400 // rw - issue write
#define RD	0x0800 // rw - issue read
#define RACK    0x1000 // wo - master ACK/NAK state for reads
#define TIP	0x1000 // ro - transaction in progress
#define ACK     0x2000 // ro - slave ACK/NAK received on write
#define LOST    0x4000 // ro - arbitration lost
#define BUSY    0x8000 // ro - I2C bus busy

/* example:  ./i2c sw:a0 w:fa sw:a1 r r r r r r p  */

int main(int argc, char **argv) {
	unsigned n,x;
	unsigned c;

	argc--;
	argv++;

	jconnect();

	while (argc > 0) {
		char *cmd = argv[0];
		n = 0;
		while (*cmd) {
			switch(*cmd) {
			case 's': case 'S': n |= STA; break;
			case 'p': case 'P': n |= STP; break;
			case 'w': case 'W': n |= WR; break;
			case 'r': case 'R': n |= RD; break;
			case 'z': case 'Z': n |= RACK; break;
			case ':':
				cmd++;
				n |= (strtoul(cmd, 0, 16) & 0xFF);
				goto done;
			default:
				fprintf(stderr,"syntax error\n");
				return -1;
			}
			cmd++;
		}
done:
		jwr(n);
		c = 1;
		while ((x = jrd()) & TIP) {
			c++;
			if (c == 100000) {
				fprintf(stderr,"timeout\n");
				return -1;
			}
		}

		fprintf(stderr, "%c%c%c%c %02x -> %02x %c%c%c (%d)\n",
			(n&RD)?'R':'-', (n&WR)?'W':'-',
			(n&STP)?'P':'-', (n&STA)?'S':'-',
			n & 0xFF, x & 0xFF,
			(x&ACK)?'N':'A', (x&LOST)?'L':'-',
			(x&BUSY)?'B':'-', c);
		argc--;
		argv++;
	}

	return 0;
}
