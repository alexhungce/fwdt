/*
 * FWDT ACPI driver
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

#include "fwdt_lib.h"
#include <acpi/acpi_bus.h>
#include <linux/acpi.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#ifdef CONFIG_ACPI

acpi_status acpi_handle_locate_callback(acpi_handle handle, u32 level,
					void *context, void **return_value)
{
	*(acpi_handle *)return_value = handle;

	return AE_CTRL_TERMINATE;
}

void acpi_device_path(const char *buf, char *path)
{
	path[0] = '\\';
	if (strlen(buf) < 80) {
		strncpy(path + 1, buf, ACPI_PATH_SIZE - 2);
		path[strlen(buf) - 1] = 0;
	} else
		path[1] = 0;
}

ssize_t acpi_method_0_0_write(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;
	char path[ACPI_PATH_SIZE];

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

static char acpi_method_name[80];
ssize_t acpi_method_0_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	acpi_status status;
	unsigned long long output;

	status = acpi_evaluate_integer(NULL, acpi_method_name, NULL, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_name);
	else
		printk("Failed to execute %s\n", acpi_method_name);

	return sprintf(buf, "0x%08llx\n", output);
}

static u32 acpi_arg0;
ssize_t acpi_arg0_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	return sprintf(buf, "0x%08x\n", acpi_arg0);
}

ssize_t acpi_arg0_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	acpi_arg0 = simple_strtoul(buf, NULL, 16);

	return count;
}

static u32 acpi_arg1;
ssize_t acpi_arg1_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	return sprintf(buf, "0x%08x\n", acpi_arg1);
}

ssize_t acpi_arg1_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	acpi_arg1 = simple_strtoul(buf, NULL, 16);

	return count;
}

ssize_t acpi_method_1_0_read(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	acpi_status status;

	status = acpi_execute_simple_method(NULL, acpi_method_name, acpi_arg0);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_name);
	else
		printk("Failed to execute %s\n", acpi_method_name);

	return sprintf(buf, "0x%08x\n", ACPI_SUCCESS(status));
}

ssize_t acpi_method_name_write(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	acpi_handle device;
	acpi_status status;

	acpi_device_path(buf, acpi_method_name);

	status = acpi_get_handle(NULL, acpi_method_name, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", acpi_method_name);
	}

	return count;
}

ssize_t acpi_method_1_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg0 = {ACPI_TYPE_INTEGER};
	struct acpi_object_list args = {1, &arg0};

	arg0.integer.value = acpi_arg0;

	status = acpi_evaluate_integer(NULL, acpi_method_name, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_name);
	else
		printk("Failed to execute %s\n", acpi_method_name);

	return sprintf(buf, "0x%08llx\n", output);
}

ssize_t acpi_method_2_0_read(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg_objs[] = {{ACPI_TYPE_INTEGER},
					{ACPI_TYPE_INTEGER}};
	struct acpi_object_list args = {2, arg_objs};

	arg_objs[0].integer.value = acpi_arg0;
	arg_objs[1].integer.value = acpi_arg1;

	status = acpi_evaluate_integer(NULL, acpi_method_name, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_name);
	else
		printk("Failed to execute %s\n", acpi_method_name);

	return sprintf(buf, "0x%08x\n", ACPI_SUCCESS(status));
}

ssize_t acpi_method_2_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg_objs[] = {{ACPI_TYPE_INTEGER},
					{ACPI_TYPE_INTEGER}};
	struct acpi_object_list args = {2, arg_objs};

	arg_objs[0].integer.value = acpi_arg0;
	arg_objs[1].integer.value = acpi_arg1;

	status = acpi_evaluate_integer(NULL, acpi_method_name, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method_name);
	else
		printk("Failed to execute %s\n", acpi_method_name);

	return sprintf(buf, "0x%08llx\n", output);
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
		ret = -EINVAL;
		goto err;
		break;
	}

	if (unlikely(copy_to_user(fg, &fd, sizeof(struct fwdt_acpi_data))))
		return -EFAULT;

err:
	return ret;
}

#endif
