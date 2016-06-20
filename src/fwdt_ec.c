/*
 * FWDT EC driver
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <acpi/acpi_drivers.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include "fwdt_lib.h"

MODULE_AUTHOR("Alex Hung");
MODULE_DESCRIPTION("FWDT EC Driver");
MODULE_LICENSE("GPL");

static acpi_handle ec_device = NULL;
static int ec_offset;
ssize_t ec_read_data(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	u8 data;

	ret = ec_read(ec_offset, &data);
	if (ret)
		return -EINVAL;

	return sprintf(buf, "%x\n", data);;
}

ssize_t ec_write_data(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	u8 data;

	data = simple_strtoul(buf, NULL, 16);
	ret = ec_write(ec_offset, data);
	if (ret)
		return -EINVAL;

	return count;
}

ssize_t ec_read_addr(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%02x\n", ec_offset);;
}

ssize_t ec_write_addr(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ec_offset = simple_strtoul(buf, NULL, 16);
	return count;
}

ssize_t ec_exec_qmethod(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_status status;
	u8 data;
	char q_num[5];

	data = simple_strtoul(buf, NULL, 16);
	sprintf(q_num, "_Q%02X", data);

	status = acpi_evaluate_object(ec_device, q_num, NULL, NULL);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", q_num);
	else
		printk("Failed to execute %s\n", q_num);

	return count;
}

int handle_acpi_ec_cmd(fwdt_generic __user *fg)
{
	int err;
	struct fwdt_ec_data *fec = (struct fwdt_ec_data*) fg;
	struct fwdt_ec_data ecd;
	char q_num[5];
	acpi_status status;

	if (copy_from_user(&ecd, fec, sizeof(struct fwdt_ec_data)))
		return -EFAULT;

	switch (fg->parameters.func) {
	case GET_EC_REGISTER:
		err = ec_read(ecd.address, &ecd.data);
		if (copy_to_user(fec, &ecd, sizeof(struct fwdt_ec_data)))
			return -EFAULT;
		break;
	case SET_EC_REGISTER:
		err = ec_write(ecd.address, ecd.data);
		break;
	case CALL_EC_QMETHOD:
		sprintf(q_num, "_Q%02X", ecd.q_method);
		status = acpi_evaluate_object(ec_device, q_num, NULL, NULL);
		if (ACPI_SUCCESS(status))
			err = FWDT_SUCCESS;
		else
			err = FWDT_DEVICE_NOT_FOUND;
		break;
	case CHECK_EC_DEVICE:
		if (ec_device == NULL) {
			ecd.parameters.func_status = FWDT_DEVICE_NOT_FOUND;
			err = FWDT_SUCCESS;
		} else {
			ecd.parameters.func_status = FWDT_SUCCESS;
			err = FWDT_SUCCESS;
		}

		if (copy_to_user(fec, &ecd, sizeof(struct fwdt_ec_data)))
			return -EFAULT;
		break;
	default:
		err = FWDT_FUNC_NOT_SUPPORTED;
		break;
	}

	return err;
}
