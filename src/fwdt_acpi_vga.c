/*
 * FWDT ACPI VGA driver
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
#include <linux/acpi.h>
#include <linux/semaphore.h>
#include "fwdt_lib.h"

#ifdef CONFIG_ACPI

int acpi_lcd_query_levels(acpi_handle *device, union acpi_object **levels)
{
	int status;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *obj;

	*levels = NULL;

	status = acpi_evaluate_object(device, "_BCL", NULL, &buffer);
	if (!ACPI_SUCCESS(status))
		return status;
	obj = (union acpi_object *)buffer.pointer;
	if (!obj || (obj->type != ACPI_TYPE_PACKAGE)) {
		printk("Invalid _BCL data\n");
		status = -EFAULT;
		goto err;
	}

	*levels = obj;
	return 0;

 err:
	kfree(buffer.pointer);
	return status;
}

acpi_handle video_device;
ssize_t acpi_video_write_device(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_status status;
	char device_path[255];

	device_path[0] = '\\';
	strncpy(device_path + 1, buf, strlen(buf));
	device_path[strlen(buf)] = 0;

	status = acpi_get_handle(NULL, device_path, &video_device);
	if (!ACPI_SUCCESS(status))
		printk("Failed to find video device: %s!\n", buf);

	return count;
}

ssize_t acpi_video_read_brightness(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	acpi_status status;
	unsigned long long bqc_level;
	union acpi_object *obj;
	union acpi_object *o;
	int i;

	if (!video_device) {
		printk("acpi_video device is not specified!\n");
		return -ENODEV;
	}

	status = acpi_evaluate_integer(video_device, "_BQC", NULL, &bqc_level);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to read brightness level!\n");
		return -ENODEV;
	}

	if (!ACPI_SUCCESS(acpi_lcd_query_levels(video_device, &obj))) {
		printk("Failed to query brightness levels\n");
		goto no_bcl;
	}

	for (i = 0; i < obj->package.count; i++) {
		o =  (union acpi_object *) &obj->package.elements[i];
		if (unlikely(o->type != ACPI_TYPE_INTEGER))
			continue;
		printk("Brightness[%d] = %d\n", i, (u32) o->integer.value);
	}

 no_bcl:

	return sprintf(buf, "%lld\n", bqc_level);
}

ssize_t acpi_video_write_brightness(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_status status;
	union acpi_object arg0 = { ACPI_TYPE_INTEGER };
	struct acpi_object_list args = { 1, &arg0 };

	if (!video_device) {
		printk("acpi_video device is not specified!\n");
		return count;
	}

	arg0.integer.value = simple_strtoul(buf, NULL, 10);

	status = acpi_evaluate_object(video_device, "_BCM", &args, NULL);
	if (!ACPI_SUCCESS(status))
		printk("Failed to set brightness level!\n");

	return count;
}

static int get_acpi_vga_brightness(struct fwdt_brightness *fb)
{
	int status;
	unsigned long long bqc_level;
	acpi_handle lcd_device;

	status = acpi_get_handle(NULL, fb->lcd_path, &lcd_device);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to find acpi lcd device: %s\n", fb->lcd_path);
		goto err;
	}

	status = acpi_evaluate_integer(lcd_device, "_BQC", NULL, &bqc_level);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to read brightness level!\n");
		goto err;
	}

	fb->brightness_level = bqc_level;
 err:
	return status;
}

static int set_acpi_vga_brightness(struct fwdt_brightness *fb)
{
	int status;
	acpi_handle lcd_device;

	union acpi_object arg0 = { ACPI_TYPE_INTEGER };
	struct acpi_object_list args = { 1, &arg0 };

	arg0.integer.value = fb->brightness_level;

	status = acpi_get_handle(NULL, fb->lcd_path, &lcd_device);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to find acpi lcd device: %s\n", fb->lcd_path);
		goto err;
	}

	status = acpi_evaluate_object(lcd_device, "_BCM", &args, NULL);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to set brightness level!\n");
		goto err;
	}

 err:
	return status;
}

static int get_acpi_vga_br_levels(struct fwdt_brightness *fbl)
{
	int status;
	union acpi_object *obj, *o;
	acpi_handle lcd_device;
	int i;

	status = acpi_get_handle(NULL, fbl->lcd_path, &lcd_device);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to find acpi lcd device: %s\n", fbl->lcd_path);
		goto err;
	}

	if (!ACPI_SUCCESS(acpi_lcd_query_levels(lcd_device, &obj))) {
		printk("Failed to query brightness levels\n");
		goto err;
	}

	fbl->num_of_levels = obj->package.count;
	for (i = 0; i < obj->package.count; i++) {
		o =  (union acpi_object *) &obj->package.elements[i];
		if (unlikely(o->type != ACPI_TYPE_INTEGER))
			continue;
		fbl->levels[i] = (u32) o->integer.value;
	}

 err:
	return status;
}

int handle_acpi_vga_cmd(fwdt_generic __user *fg)
{
	int err;

	switch (fg->parameters.func) {
	case GET_BRIGHTNESS:
		err = get_acpi_vga_brightness((struct fwdt_brightness*) fg);
		break;
	case SET_BRIGHTNESS:
		err = set_acpi_vga_brightness((struct fwdt_brightness*) fg);
		break;
	case GET_BRIGHTNESS_LV:
		err = get_acpi_vga_br_levels((struct fwdt_brightness*) fg);
		break;
/*
	case GET_VIDEO_DEVICE:
		break;
*/
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

#endif
