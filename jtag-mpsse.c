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
#include <ctype.h>

#include "jtag.h"

#include <libusb-1.0/libusb.h>

struct device_info {
	u32 idcode;
	u32 idmask;
	u32 irsize;
	const char *name;
};

#define ZYNQID(code)	((0x1B<<21)|(0x9<<17)|((code)<<12)|(0x49<<1)|1)
#define ZYNQMASK	0x0FFFFFFF

struct device_info LIBRARY[] = {
	{ 0x4ba00477, 0xFFFFFFFF, 4, "Cortex A9" },
	{ ZYNQID(0x02), ZYNQMASK, 6, "xc7x010" },
	{ ZYNQID(0x1b), ZYNQMASK, 6, "xc7x015" },
	{ ZYNQID(0x07), ZYNQMASK, 6, "xc7x020" },
	{ ZYNQID(0x0c), ZYNQMASK, 6, "xc7x030" },
	{ ZYNQID(0x11), ZYNQMASK, 6, "xc7x045" },
};

struct device_info *identify_device(u32 idcode) {
	unsigned n;
	for (n = 0; n < sizeof(LIBRARY) / sizeof(LIBRARY[0]); n++) {
		if ((idcode & LIBRARY[n].idmask) == LIBRARY[n].idcode) {
			return LIBRARY + n;
		}
	}
	return NULL;
}

#define TRACE_USB	0
#define TRACE_JTAG	0

#define DEVICE_MAX 16

struct jtag_handle {
	struct libusb_device_handle *udev;
	u8 ep_in;
	u8 ep_out;

	u8 read_buffer[512];
	u8 *read_ptr;
	u32 read_count;
	u32 read_size;

	u32 dr_pre;
	u32 dr_post;
	u32 ir_pre;
	u32 ir_post;

	u32 dev_count;
	u32 dev_idcode[DEVICE_MAX];
	u32 dev_irsize[DEVICE_MAX];
	struct device_info *dev_info[DEVICE_MAX];

	u32 active_irsize;
	u32 active_idcode;
};

static int usb_open(JTAG *jtag, unsigned vid, unsigned pid) {
	struct libusb_device_handle *udev;

	if (libusb_init(NULL) < 0)
		return -1;

	if (!(udev = libusb_open_device_with_vid_pid(NULL, vid, pid))) {
		fprintf(stderr,"cannot find device\n");
		return -1;
	}

	libusb_detach_kernel_driver(udev, 0);

	if (libusb_claim_interface(udev, 0) < 0) {
		fprintf(stderr,"cannot claim interface\n");
		return -1;
	}
	jtag->udev = udev;
	jtag->ep_in = 0x81;
	jtag->ep_out = 0x02;
	return 0;
}


#if TRACE_USB
static void dump(char *prefix, void *data, int len) {
	unsigned char *x = data;
	fprintf(stderr,"%s: (%d)", prefix, len);
	while (len > 0) {
		fprintf(stderr," %02x", *x++);
		len--;
	}
	fprintf(stderr,"\n");
}
#endif

static int usb_bulk(struct libusb_device_handle *udev,
	unsigned char ep, void *data, int len, unsigned timeout) {
	int r, xfer;
#if TRACE_USB
	if (!(ep & 0x80))
		dump("xmit", data, len);
#endif
	r = libusb_bulk_transfer(udev, ep, data, len, &xfer, timeout);
	if (r < 0) {
		fprintf(stderr,"bulk: error: %d\n", r);
		return r;
	}
#if TRACE_USB
	if (ep & 0x80)
		dump("recv", data, xfer);
#endif
	return xfer;
}

#define FTDI_REQTYPE_OUT	(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT)
#define FTDI_CTL_RESET		0x00
#define FTDI_CTL_SET_BITMODE	0x0B
#define FTDI_CTL_SET_EVENT_CH	0x06
#define FTDI_CTL_SET_ERROR_CH	0x07

#define FTDI_IFC_A 1
#define FTDI_IFC_B 2

int ftdi_reset(struct libusb_device_handle *udev) {
	if (libusb_control_transfer(udev,
		FTDI_REQTYPE_OUT, FTDI_CTL_RESET, 0, FTDI_IFC_A, NULL, 0, 10000) < 0) {
		fprintf(stderr,"ftdi: reset failed\n");
		return -1;
	}
	return 0;
}

int ftdi_mpsse_enable(struct libusb_device_handle *udev) {
	if (libusb_control_transfer(udev,
		FTDI_REQTYPE_OUT, FTDI_CTL_SET_BITMODE, 0x0000, FTDI_IFC_A, NULL, 0, 10000) < 0) {
		fprintf(stderr,"ftdi: set bitmode failed\n");
		return -1;
	}
	if (libusb_control_transfer(udev,
		FTDI_REQTYPE_OUT, FTDI_CTL_SET_BITMODE, 0x020b, FTDI_IFC_A, NULL, 0, 10000) < 0) {
		fprintf(stderr,"ftdi: set bitmode failed\n");
		return -1;
	}
	if (libusb_control_transfer(udev,
		FTDI_REQTYPE_OUT, FTDI_CTL_SET_EVENT_CH, 0, FTDI_IFC_A, NULL, 0, 10000) < 0) {
		fprintf(stderr,"ftdi: disable event character failed\n");
		return -1;
	}
	return 0;	
	if (libusb_control_transfer(udev,
		FTDI_REQTYPE_OUT, FTDI_CTL_SET_ERROR_CH, 0, FTDI_IFC_A, NULL, 0, 10000) < 0) {
		fprintf(stderr,"ftdi: disable error character failed\n");
		return -1;
	}
	return 0;	
}

/* TODO: handle smaller packet size for lowspeed version of the part */
/* TODO: multi-packet reads */
/* TODO: asynch/background reads */
static int ftdi_read(JTAG *jtag, unsigned char *buffer, int count, int timeout) {
	int xfer;
	while (count > 0) {
		if (jtag->read_count >= count) {
			memcpy(buffer, jtag->read_ptr, count);
			jtag->read_count -= count;
			jtag->read_ptr += count;
			return 0;
		}
		if (jtag->read_count > 0) {
			memcpy(buffer, jtag->read_ptr, jtag->read_count);
			count -= jtag->read_count;
			jtag->read_count = 0;
		}
		xfer = usb_bulk(jtag->udev, jtag->ep_in,
			jtag->read_buffer, jtag->read_size, 1000);
		if (xfer < 0)
			return -1;
		if (xfer < 2)
			return -1;
		/* discard header */
		jtag->read_ptr = jtag->read_buffer + 2;
		jtag->read_count = xfer - 2;
	}
	return 0;
}

static unsigned char bitxfermask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };

static int jtag_move(JTAG *jtag, int count, unsigned bits){
	unsigned char buf[1024];
	unsigned char *p = buf;
	unsigned xfer;

	if (count > 32)
		return -1;

#if TRACE_JTAG
	{
		u64 tmp = bits;
		int n;
		fprintf(stderr,"TDI <- ");
		for (n = 0; n < count; n++) {
			fprintf(stderr, "X");
		}
		fprintf(stderr,"\nTMS <- ");
		for (n = 0; n < count; n++) {
			fprintf(stderr,"%c", (tmp & 1) ? '1' : '0');
			tmp >>= 1;
		}
		fprintf(stderr,"\n");
	}
#endif
	while (count > 0) {
		xfer = (count > 6) ? 6 : count;
		*p++ = 0x4b;
		*p++ = xfer - 1;
		*p++ = bits & bitxfermask[xfer];
		bits >>= xfer;
		count -= xfer;
	}
	if (usb_bulk(jtag->udev, jtag->ep_out, buf, p - buf, 1000) != (p - buf))
		return -1;
	return 0;
}

int _jtag_shift(JTAG *jtag, int count, u64 bits, u64 *out,
	int movecount, unsigned movebits) {
	unsigned char buf[1024];
	unsigned char *p = buf;
	unsigned xfer, bytes, readbytes, iobytes, readbits;

	if (count > 64)
		return -1;

	if (movecount) {
		/* if we're doing a joint shift-move, the last bit is
		 * shifted during the first bit of the move
		 */
		count--;
	}

	if (count <= 0)
		return -1;

#if TRACE_JTAG
	{
		u64 tmp = bits;
		int n;
		fprintf(stderr,"TDI <- ");
		for (n = 0; n < count; n++) {
			fprintf(stderr,"%c", (tmp & 1) ? '1' : '0');
			tmp >>= 1;
		}
		for (n = 0; n < movebits; n++)
			fprintf(stderr,"%c", (tmp & 1) ? '1' : '0');
		fprintf(stderr,"\nTMS <- ");
		for (n = 0; n < count; n++)
			fprintf(stderr,"0");
		tmp = movebits;
		for (n = 0; n < movebits; n++) {
			fprintf(stderr,"%c", (tmp & 1) ? '1' : '0');
			tmp >>= 1;
		}
		fprintf(stderr,"\n");
	}
#endif

	bytes = count / 8;
	readbytes = bytes;
	iobytes = bytes;

	if (bytes) {
		count -= (bytes * 8);
		*p++ = out ? 0x39 : 0x19;
		*p++ = bytes - 1;
		*p++ = 0;
		while (bytes > 0) {
			*p++ = bits & 0xff;
			bits >>= 8;
			bytes--;
		}
	}
	readbits = count;
	while (count > 0) {
		xfer = (count > 6) ? 6 : count;
		*p++ = out ? 0x3b : 0x1b;
		*p++ = xfer - 1;
		*p++ = bits & bitxfermask[xfer];
		bits >>= xfer;
		count -= xfer;
		iobytes++;
	}

	if (movecount) {
		if (movebits > 6)
			return -1; /* TODO */
		*p++ = out ? 0x6b : 0x4b;
		*p++ = movecount - 1;
		*p++ = ((bits & 1) << 7) | (movebits & 0x3F);
		iobytes++;
	}

	/* if we're doing readback demand an ioflush */
	if (out)
		*p++ = 0x87;

	if (usb_bulk(jtag->udev, jtag->ep_out, buf, p - buf, 1000) != (p - buf))
		return -1;

	if (out) {
		u64 n = 0, bit;
		unsigned shift = 0;
		if (ftdi_read(jtag, buf, iobytes, 1000))
			return -1;
		p = buf;
		while (readbytes-- > 0) {
			n |= (((u64) *p++) << shift);
			shift += 8;
		}
		while (readbits > 0) {
			xfer = (readbits > 6) ? 6 : readbits;
			bit = ((*p++) >> (8 - xfer)) & bitxfermask[xfer];
			n |= (bit << shift);
			shift += xfer;
			readbits -= xfer;
		}
		if (movecount) {
			/* we only ever care about the first bit */
			bit = ((*p++) >> (8 - movecount)) & 1;
			n |= (bit << shift);
		}
		*out = n;
	}
	return 0;
}

#define ONES(n) (0x7FFFFFFFFFFFFFFFULL >> (63 - (n)))

static int jtag_shift_dr(JTAG *jtag, int count, u64 bits,
	u64 *out, int movecount, unsigned movebits) {
	int r;
	bits <<= jtag->dr_pre;
	r = _jtag_shift(jtag, count + jtag->dr_pre + jtag->dr_post,
		bits, out, movecount, movebits);
	if (out) {
		*out = (*out >> jtag->dr_pre) & ONES(count);
	}
	return r;
}

static int jtag_shift_ir(JTAG *jtag, int count, u64 bits,
	int movecount, unsigned movebits) {
	bits |= (ONES(jtag->ir_post) << count);
	bits <<= jtag->ir_pre;
	bits |= ONES(jtag->ir_pre);
	return _jtag_shift(jtag, count + jtag->ir_pre + jtag->ir_post,
		bits, NULL , movecount, movebits);
}

/* JTAG notes
 *
 * TMS is sampled on +TCK
 * Capture-XR state loads shift register on +TCK as state is exited
 * Shift-XR state TDO goes active (containing shiftr[0]) on the first -TCK
 *          after entry, shifts occur on each +TCK, *including* the +TCK
 *          that will exist Shift-XR when TMS=1 again
 * Update-XR update occurs on the -TCK after entry to state
 * 
 * Any -> Reset: 11111
 * Any -> Reset -> RTI: 111110
 * RTI -> ShiftDR: 100
 * ShiftDR shifting: 0 x N
 * ShiftDR -> UpdateDR -> RTI: 110
 * ShiftDR -> UpdateDR -> ShiftDR: 11100
 * RTI -> ShiftIR: 1100
 * ShiftIR shifting: 0 x N
 * ShiftIR -> UpdateIR -> RTI: 110
 */

#define MOVE_NONE		0,0
#define MOVE_ANY_TO_RESET_IDLE	8,0b01111111
#define MOVE_IDLE_TO_SHIFTDR	3,0b001
#define MOVE_IDLE_TO_SHIFTIR	4,0b0011
#define MOVE_SHIFTxR_TO_IDLE	3,0b011


void jtag_close(JTAG *jtag) {
}

static unsigned char mpsse_init[] = {
	0x85, // loopback off
	0x8a, // disable clock/5
	0x86, 0x01, 0x00, // set divisor
	0x80, 0xe8, 0xeb, // set low state and dir
	0x82, 0x20, 0x30, // set high state and dir
};

JTAG *jtag_open(void) {
	int r;
	JTAG *jtag = malloc(sizeof(JTAG));
	if (!jtag)
		return NULL;
	memset(jtag, 0, sizeof(JTAG));
	jtag->read_size = sizeof(jtag->read_buffer);

	r = usb_open(jtag, 0x0403, 0x6010);
	if (r < 0)
		goto fail;
	if (ftdi_reset(jtag->udev))
		goto fail;
	if (ftdi_mpsse_enable(jtag->udev))
		goto fail;
	if (usb_bulk(jtag->udev, jtag->ep_out,
		mpsse_init, sizeof(mpsse_init), 1000) != sizeof(mpsse_init))
		goto fail;
	return jtag;

fail:
	jtag_close(jtag);
	free(jtag);
	return NULL;
}

#define ENUM_MAGIC (0x00005038aaaaaaFFULL)

/* TODO: attempt to probe for IR size if devices are not in library.
 *       On RESET the IR must have 1 in bit0 and 0 in bit1.
 *       The other bits are undefined.
 */
int jtag_enumerate(JTAG *jtag) {
	u64 u;
	unsigned n;

	jtag_move(jtag, MOVE_ANY_TO_RESET_IDLE);
	jtag_move(jtag, MOVE_IDLE_TO_SHIFTIR);
	/* BYPASS is always all 1s -- shift a pile of BYPASS opcodes into the chain */
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_NONE);
	_jtag_shift(jtag, 64, 0xFFFFFFFFFFFFFFFFULL, NULL, MOVE_SHIFTxR_TO_IDLE);
	jtag_move(jtag, MOVE_IDLE_TO_SHIFTDR);
	/* BYPASS registers should be one 0 bit each. 
	 * Shift a pattern in and try to find it after all the 0s. 
	 * If we can't find it, there must be >16 devices on the chain
	 * and/or they have enormous instruction registers.
	 */
	_jtag_shift(jtag, 64, ENUM_MAGIC, &u, MOVE_SHIFTxR_TO_IDLE);
	for (n = 0; n < DEVICE_MAX; n++) {
		if (!(u & 1)) {
			u >>= 1;
			continue;
		}
		if (u == ENUM_MAGIC) {
			jtag->dev_count = n;
#if TRACE_JTAG
			fprintf(stderr, "jtag: found %d devices\n", jtag->dev_count);
#endif
			goto okay;
		}
	}
	fprintf(stderr,"jtag: more than %d devices?!\n", DEVICE_MAX);
	return -1;

okay:
	jtag_move(jtag, MOVE_ANY_TO_RESET_IDLE);
	/* should put IDCODE in IR of everyone */
	jtag_move(jtag, MOVE_IDLE_TO_SHIFTDR);
	for (n = 0; n < jtag->dev_count; n++) {
		_jtag_shift(jtag, 32, 0xFFFFFFFF, &u, MOVE_NONE);
		if ((u & 1) == 0) {
			fprintf(stderr, "jtag: device %2d has no idcode\n", n);
			return -1;
		}
		jtag->dev_idcode[n] = (u32) u;
		jtag->dev_info[n] = identify_device((u32) u);
		if (jtag->dev_info[n] == NULL) {
			fprintf(stderr, "jtag: device %2d idcode %08x unknown IR size\n",
				n, (u32) u);
			return -1;
		}
		jtag->dev_irsize[n] = jtag->dev_info[n]->irsize;
#if TRACE_JTAG
		fprintf(stderr, "jtag: device %2d idcode %08x irsize %2d '%s'\n",
			n, jtag->dev_idcode[n], jtag->dev_irsize[n],
			jtag->dev_info[n]->name);
#endif
	}
	return jtag->dev_count;
}

int jtag_select(JTAG *jtag, u32 idcode) {
	u32 irsize = 0;
	u32 ir_pre = 0;
	u32 ir_post = 0;
	u32 dr_pre = 0;
	u32 dr_post = 0;
	int found = 0;
	unsigned n;

	for (n = 0; n < jtag->dev_count; n++) {
		u32 sz;
		if (idcode == jtag->dev_idcode[n]) {
			irsize = jtag->dev_irsize[n];
			found = 1;
			continue;
		}
		sz = jtag->dev_irsize[n];
		if (sz > 32)
			return -1;
		if (found) {
			ir_post += sz;
			dr_post += 1;
		} else {
			ir_pre += sz;
			dr_pre += 1;
		}
	}
	if (!found) {
		fprintf(stderr,"device id %08x not found in chain\n", idcode);
		return -1;
	}

	jtag->active_idcode = idcode;
	jtag->active_irsize = irsize;
#if TRACE_JTAG
	fprintf(stderr,"jtag: select idcode %08x\n", idcode);
	fprintf(stderr,"jtag: TDI -> irpost(%d) -> iract(%d) -> irpre(%d) -> TDO\n",
		ir_post, irsize, ir_pre);
#endif
	jtag->dr_pre = dr_pre;
	jtag->dr_post = dr_post;
	jtag->ir_pre = ir_pre;
	jtag->ir_post = ir_post;

	return jtag_move(jtag, MOVE_ANY_TO_RESET_IDLE);
}

int jtag_ir_wr(JTAG *jtag, u32 ir) {
	if (jtag_move(jtag, MOVE_IDLE_TO_SHIFTIR))
		return -1;
	if (jtag_shift_ir(jtag, jtag->active_irsize, ir, MOVE_SHIFTxR_TO_IDLE))
		return -1;
	return 0;
}

int jtag_dr_io(JTAG *jtag, u32 bitcount, u64 wdata, u64 *rdata) {
	if (jtag_move(jtag, MOVE_IDLE_TO_SHIFTDR))
		return -1;
	if (jtag_shift_dr(jtag, bitcount, wdata, rdata, MOVE_SHIFTxR_TO_IDLE))
		return -1;
	return 0;
}

