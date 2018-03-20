/*
 * FWDT MSR driver
 *
 * Copyright(C) 2016-2018 Canonical Ltd.
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
#include <asm/msr.h>
#include <linux/semaphore.h>
#include "fwdt_lib.h"

#ifdef CONFIG_X86

static int msr_register;
ssize_t msr_read_data(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u32 h, l;
	rdmsr(msr_register, l, h);

	return sprintf(buf, "0x%08x%08x\n", (h << 16), l);
}

ssize_t msr_set_register(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (kstrtoint(buf, 16, &msr_register))
		return -EINVAL;
	return count;
}

#endif
