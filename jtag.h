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

#ifndef _JTAG_H_
#define _JTAG_H_

#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef struct jtag_handle JTAG;

JTAG *jtag_open(void);
void jtag_close(JTAG *jtag);

int jtag_enumerate(JTAG *jtag);
u32 jtag_get_nth_idcode(JTAG *jtag, u32 n);
int jtag_select(JTAG *jtag, u32 idcode);

int jtag_ir_wr(JTAG *jtag, u32 ir);
int jtag_dr_io(JTAG *jtag, u32 bitcount, u64 wdata, u64 *rdata);

#endif
