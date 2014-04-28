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

#include "i2c.h"

#define TRACE_TXN 0

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

static int i2c_txn(unsigned cmd, unsigned *_status) {
	unsigned timeout = 0;
	unsigned status;
	jwr(cmd);
	while ((status = jrd()) & TIP) {
		timeout++;
		if (timeout == 10000) {
#if TRACE_TXN
			fprintf(stderr,"txn: %04x XXXX\n",cmd); 
#endif
			fprintf(stderr, "i2c: txn timeout\n");
			return -1;
		}
	}
#if TRACE_TXN
	fprintf(stderr,"txn: %04x %04x\n",cmd, status); 
#endif
	*_status = status;
	return 0;	
}

static int i2c_start(unsigned saddr) {
	unsigned status;
	if (i2c_txn((saddr & 0xFF) | STA | WR, &status))
		return -1;
	if (status & ACK) {
		fprintf(stderr, "i2c: slave NAK'd\n");
		return -1;
	}
	return 0;
}

static int i2c_stop(void) {
	unsigned status;
	return i2c_txn(STP, &status);
}

static int i2c_write(unsigned data) {
	unsigned status;
	if (i2c_txn((data & 0xFF) | WR, &status))
		return -1;
	if (status & ACK) {
		fprintf(stderr, "i2c: slave NAK'd\n");
		return -1;
	}
	return 0;
}

static int i2c_read(unsigned *data, unsigned send_nak) {
	unsigned status;
	if (i2c_txn(RD | (send_nak ? RACK : 0), &status))
		return -1;
	*data = status & 0xFF;
	return 0;
}

int i2c_wr16(unsigned saddr, unsigned addr, unsigned val) {
	if (i2c_start(saddr)) return -1;
	if (i2c_write(addr >> 8)) return -1;
	if (i2c_write(addr)) return -1;
	if (i2c_write(val >> 8)) return -1;
	if (i2c_write(val)) return -1;
	return i2c_stop();
}

int i2c_rd16(unsigned saddr, unsigned addr, unsigned *val) {
	unsigned a, b;
	if (i2c_start(saddr)) return -1;
	if (i2c_write(addr >> 8)) return -1;
	if (i2c_write(addr)) return -1;
	if (i2c_start(saddr | 1)) return -1;
	if (i2c_read(&a, 0)) return -1;
	if (i2c_read(&b, 1)) return -1;
	*val = (a << 8) | b;
	return i2c_stop();
}

void i2c_init(void) {
	jconnect();
}
