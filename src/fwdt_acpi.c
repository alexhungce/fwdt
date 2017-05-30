/*
 * FWDT ACPI driver
 *
 * Copyright(C) 2016-2017 Canonical Ltd.
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
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <linux/semaphore.h>
#include "fwdt_lib.h"

#ifdef CONFIG_ACPI

acpi_status acpi_handle_locate_callback(acpi_handle handle,
			u32 level, void *context, void **return_value)
{
	*(acpi_handle *)return_value = handle;

	return AE_CTRL_TERMINATE;
}

void acpi_device_path(const char *buf, char *path)
{
	path[0] = '\\';
	if (strlen(buf) < 80)  {
		strncpy(path + 1, buf, strlen(buf));
		path[strlen(buf)] = 0;
	} else
		path[1] = 0;
}

ssize_t acpi_method_0_0_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;
	char path[80];

	acpi_device_path(buf, path);

	status = acpi_get_handle(NULL, path, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", path);
		goto err;
	}

	status = acpi_evaluate_object(NULL, path, NULL, NULL);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", path);
	else
		printk("Failed to execute %s\n", path);

      err:
	return count;
}

static char device_path_0_1[80];
ssize_t acpi_method_0_1_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	acpi_status status;
	unsigned long long output;

	status = acpi_evaluate_integer(NULL, device_path_0_1, NULL, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", device_path_0_1);
	else
		printk("Failed to execute %s\n", device_path_0_1);

	return sprintf(buf, "0x%08llx\n", output);
}

ssize_t acpi_method_0_1_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;

	acpi_device_path(buf, device_path_0_1);

	status = acpi_get_handle(NULL, device_path_0_1, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", device_path_0_1);
	}

	return count;
}

static u32 acpi_arg0;
ssize_t acpi_arg0_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", acpi_arg0);
}

ssize_t acpi_arg0_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_arg0 = simple_strtoul(buf, NULL, 16);

	return count;
}

static u32 acpi_arg1;
ssize_t acpi_arg1_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", acpi_arg1);
}

ssize_t acpi_arg1_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_arg1 = simple_strtoul(buf, NULL, 16);

	return count;
}

static char acpi_method_1_x[80];
ssize_t acpi_method_1_0_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	acpi_status status;

	status = acpi_execute_simple_method(NULL, acpi_method_1_x, acpi_arg0);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_1_x);
	else
		printk("Failed to execute %s\n", acpi_method_1_x);

	return sprintf(buf, "0x%08x\n", ACPI_SUCCESS(status));
}

ssize_t acpi_method_1_0_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;

	acpi_device_path(buf, acpi_method_1_x);

	status = acpi_get_handle(NULL, acpi_method_1_x, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", acpi_method_1_x);
	}

	return count;
}

ssize_t acpi_method_1_1_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg0 = { ACPI_TYPE_INTEGER };
	struct acpi_object_list args = { 1, &arg0 };

	arg0.integer.value = acpi_arg0;

	status = acpi_evaluate_integer(NULL, acpi_method_1_x, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_1_x);
	else
		printk("Failed to execute %s\n", acpi_method_1_x);

	return sprintf(buf, "0x%08llx\n", output);
}

ssize_t acpi_method_1_1_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;

	acpi_device_path(buf, acpi_method_1_x);

	status = acpi_get_handle(NULL, acpi_method_1_x, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", acpi_method_1_x);
	}

	return count;
}

ssize_t acpi_method_2_0_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg_objs[] = {
		{ACPI_TYPE_INTEGER},
		{ACPI_TYPE_INTEGER}
	};
	struct acpi_object_list args = { 2, arg_objs };

	arg_objs[0].integer.value = acpi_arg0;
	arg_objs[1].integer.value = acpi_arg1;

	status = acpi_evaluate_integer(NULL, acpi_method_1_x, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_1_x);
	else
		printk("Failed to execute %s\n", acpi_method_1_x);

	return sprintf(buf, "0x%08x\n", ACPI_SUCCESS(status));
}

int handle_acpi_aml_cmd(fwdt_generic __user *fg)
{
	int ret = 0;
	struct fwdt_acpi_data fd;

	if (unlikely(copy_from_user(&fd, fg, sizeof(struct fwdt_acpi_data))))
		return -EFAULT;

	switch (fg->parameters.func) {
	case GET_DATA_DWORD:
		// TODO evaluate acpi object
		break;
	default:
		ret = -ENOTTY;
		goto err;
		break;
	}

	fd.parameters.func_status = FWDT_SUCCESS;

	if (unlikely(copy_to_user(fg, &fd, sizeof(struct fwdt_acpi_data))))
		return -EFAULT;

 err:
	return ret;
}

#endif
