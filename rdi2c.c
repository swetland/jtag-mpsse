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

#include "i2c.h"

int main(int argc, char **argv) {
	unsigned saddr, addr, val;

	if (argc != 3) {
		fprintf(stderr, "usage: rdi2c <saddr> <addr>\n");
		return -1;
	}

	i2c_init();

	saddr = strtoul(argv[1], 0, 16);
	addr = strtoul(argv[2], 0, 16);

	if (i2c_rd16(saddr, addr, &val)) {
		fprintf(stderr, "error\n");
		return -1;
	}
	printf("%04x\n", val);
	return 0;
}
