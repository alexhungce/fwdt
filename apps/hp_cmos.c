/*
 * FWDT sample for reading CMOS error on HP laptops
 *
 * Copyright(C) 2014-2018 Canonical Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

#include "fwdtapp.h"
#include "fwdt.h"

int get_cmos_register(int fd, u8 addr, u8* val) {
	int err;
	long ioret;
	struct fwdt_cmos_data fc;

	fc.parameters.func = GET_DATA_BYTE;
	fc.cmos_address = addr;

	ioret = ioctl(fd, FWDT_HW_ACCESS_CMOS_CMD, &fc);
	if (ioret)
		return FWDT_FAIL;

	*val = fc.cmos_data;

	return 0;
}

int main(void) {
	int err;
	int fd;
	u8 cmos_data;
	int i;

	err = 0;

	fd = open("/dev/fwdt", O_RDONLY);
	if (fd == -1) {
		printf("Cannot open fwdt driver. Aborted.\n");
		err = FWDT_FAIL;
	}

	printf("hp laptop debugging info:\n");
	for (i = 0x70; i < 0x74; i++) {
		err = get_cmos_register(fd, i, &cmos_data);
		printf("\tCMOS register 0x%02x = 0x%02x\n", i, cmos_data);
	}

	close(fd);

	return err;
}
