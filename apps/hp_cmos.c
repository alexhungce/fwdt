#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
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

