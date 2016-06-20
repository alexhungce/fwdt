/*
 * FWDT I/O driver
 *
 * Copyright(C) 2016 Canonical Ltd.
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

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include "fwdt_lib.h"

static u16 iow_addr;
ssize_t iow_read_address(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%04x\n", iow_addr);
}

ssize_t iow_write_address(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	iow_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;

	return count;
}

ssize_t iow_read_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%04x\n", inw(iow_addr));
}

ssize_t iow_write_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	u16 data;

	data = simple_strtoul(buf, NULL, 16);
	outw(data, iow_addr);

	return count;
}

static u16 iob_addr;
ssize_t iob_read_address(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%04x\n", iob_addr);
}

ssize_t iob_write_address(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	iob_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;

	return count;
}

ssize_t iob_read_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%02x\n", inb(iob_addr));
}

ssize_t iob_write_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	u8 data;

	data = simple_strtoul(buf, NULL, 16);
	outb(data, iob_addr);

	return count;
}

int handle_hardware_io_cmd(fwdt_generic __user *fg)
{
	int ret = 0;
	struct fwdt_io_data fid;

	if (copy_from_user(&fid, fg, sizeof(struct fwdt_io_data)))
		return -EFAULT;

	switch (fg->parameters.func) {
	case GET_DATA_BYTE:
		fid.io_byte = inb(fid.io_address);
		break;
	case GET_DATA_WORD:
		fid.io_word = inw(fid.io_address);
		break;
	case SET_DATA_BYTE:
		outb(fid.io_byte, fid.io_address);
		break;
	case SET_DATA_WORD:
		outw(fid.io_word, fid.io_address);
		break;
	default:
		ret = FWDT_FUNC_NOT_SUPPORTED;
		goto err;
		break;
	}

	fid.parameters.func_status = FWDT_SUCCESS;

	if (copy_to_user(fg, &fid, sizeof(struct fwdt_io_data)))
		return -EFAULT;

 err:
	return ret;
}