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

