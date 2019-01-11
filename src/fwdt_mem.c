/*
 * FWDT Memory driver
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

static u32 mem_addr;
ssize_t mem_read_address(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "0x%08x\n", mem_addr);
}

ssize_t mem_write_address(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	mem_addr = simple_strtoul(buf, NULL, 16);

	return count;
}

ssize_t mem_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	volatile u32 *mem;
	u32 data;

	mem = ioremap(mem_addr, 8);
	data = *mem;
	iounmap(mem);

	return sprintf(buf, "0x%08x\n", data);
}

ssize_t mem_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	volatile u32 *mem;
	u32 data;

	data = simple_strtoul(buf, NULL, 16) & 0xFFFFFFFF;

	mem = ioremap(mem_addr, 8);
	*mem = data;
	iounmap(mem);

	return count;
}

int handle_hardware_memory_cmd(fwdt_generic __user *fg)
{
	int ret = 0;
	volatile u64 *mem;
	struct fwdt_mem_data fmd;

	if (unlikely(copy_from_user(&fmd, fg, sizeof(struct fwdt_mem_data))))
		return -EFAULT;

	mem = ioremap(fmd.mem_address, 8);

	switch (fg->parameters.func) {
	case GET_DATA_DWORD:
		fmd.mem_data = *mem;
		break;
	case SET_DATA_DWORD:
		*mem = fmd.mem_data;
		break;
	default:
		ret = -EINVAL;
		goto err;
		break;
	}

	if (unlikely(copy_to_user(fg, &fmd, sizeof(struct fwdt_mem_data))))
		ret = -EFAULT;

err:
	iounmap(mem);
	return ret;
}
