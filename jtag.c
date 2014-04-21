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

int main(int argc, char **argv) {
	JTAG *jtag;
	unsigned n;
	u64 u;

	if (!(jtag = jtag_open()))
		return -1;
	if ((n = jtag_enumerate(jtag)) <= 0)
		return -1;

	if (jtag_select(jtag, 0x4ba00477))
		return -1;

	if (jtag_ir_wr(jtag, DAP_IDCODE))
		return -1;
	if (jtag_dr_io(jtag, 32, 0, &u))
		return -1;

	fprintf(stderr,"idcode? %08lx\n", u);
	return 0;
}
