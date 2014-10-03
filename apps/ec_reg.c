/*
 * FWDT driver
 *
 * Copyright(C) 2014 Canonical Ltd.
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
#include <fcntl.h>

#include "fwdtapp.h"
#include "fwdt.h"

int get_ec_register(int fd, u8 addr, u8* val)
{
	int err;
	long ioret;
	struct fwdt_ec_data fec;

	fec.parameters.func = GET_EC_REGISTER;
	fec.address = addr;

	ioret = ioctl(fd, FWDT_ACPI_EC_CMD, &fec);
	if (ioret)
		return FWDT_FAIL;

	*val = fec.data;

	return 0;
}

int main(void)
{
	int err, i;
	int fd;
	u8 ec_reg;

	err = 0;

	fd = open("/dev/fwdt", O_RDONLY);
	if (fd == -1) {
		printf("Cannot open fwdt driver. Aborted.\n");
		return FWDT_FAIL;
	}

	printf("Loading embedded controlled's registers:\n");
	for (i = 0; i < 256; i++) {
		err = get_ec_register(fd, i, &ec_reg);
		printf("\tEC register 0x%02x = 0x%02x\n", i, ec_reg);
	}

	close(fd);

	return err;
}

