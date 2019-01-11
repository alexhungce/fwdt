/*
 * FWDT CMOS driver
 *
 * Copyright(C) 2016-2019 Canonical Ltd.
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
 */

#define pr_fmt(fmt) "fwdt: " fmt

#include "fwdt_lib.h"
#include <asm/time.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#ifdef CONFIG_X86

static int cmos_offset;
ssize_t cmos_read_data(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	if (cmos_offset > 0xFF)
		return -EINVAL;

	return sprintf(buf, "0x%02x\n", CMOS_READ(cmos_offset));
}

ssize_t cmos_write_addr(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (kstrtoint(buf, 16, &cmos_offset))
		return -EINVAL;

	if (cmos_offset > 0xFF)
		return -EINVAL;

	return count;
}

int handle_hardware_cmos_cmd(fwdt_generic __user *fg)
{
	int ret = 0;
	struct fwdt_cmos_data fcd;

	if (unlikely(copy_from_user(&fcd, fg, sizeof(struct fwdt_cmos_data))))
		return -EFAULT;

	switch (fg->parameters.func) {
	case GET_DATA_BYTE:
		fcd.cmos_data = CMOS_READ(fcd.cmos_address);
		break;
	default:
		ret = -EINVAL;
		goto err;
		break;
	}

	if (unlikely(copy_to_user(fg, &fcd, sizeof(struct fwdt_cmos_data))))
		return -EFAULT;

err:
	return ret;
}

#endif
