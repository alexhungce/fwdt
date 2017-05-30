/*
 * FWDT driver
 *
 * Copyright(C) 2014-2017 Canonical Ltd.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <acpi/video.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#include "fwdt_lib.h"

MODULE_AUTHOR("Alex Hung");
MODULE_DESCRIPTION("FWDT Driver");
MODULE_LICENSE("GPL");

static int fwdt_setup(struct platform_device *device);
static int __exit fwdt_remove(struct platform_device *device);

static struct platform_driver fwdt_driver = {
	.driver = {
		.name = "fwdt",
		.owner = THIS_MODULE,
	},
	.probe = fwdt_setup,
	.remove = fwdt_remove,
};

static struct platform_device *fwdt_platform_dev;

#ifdef CONFIG_ACPI

/* ACPI */
static DEVICE_ATTR(acpi_method_0_0, S_IWUSR, NULL, acpi_method_0_0_write);
static DEVICE_ATTR(acpi_method_0_1, S_IRUGO | S_IWUSR, acpi_method_0_1_read, acpi_method_0_1_write);
static DEVICE_ATTR(acpi_arg0, S_IRUGO | S_IWUSR, acpi_arg0_read, acpi_arg0_write);
static DEVICE_ATTR(acpi_arg1, S_IRUGO | S_IWUSR, acpi_arg1_read, acpi_arg1_write);
static DEVICE_ATTR(acpi_method_1_0, S_IRUGO | S_IWUSR, acpi_method_1_0_read, acpi_method_1_0_write);
static DEVICE_ATTR(acpi_method_1_1, S_IRUGO | S_IWUSR, acpi_method_1_1_read, acpi_method_1_1_write);

/* ACPI VGA */
extern acpi_handle video_device;
static DEVICE_ATTR(video_device, S_IWUSR, NULL, acpi_video_write_device);
static DEVICE_ATTR(video_brightness, S_IRUGO | S_IWUSR, acpi_video_read_brightness, acpi_video_write_brightness);

/* ACPI EC */
extern acpi_handle ec_device;
static DEVICE_ATTR(ec_data, S_IRUGO | S_IWUSR, ec_read_data, ec_write_data);
static DEVICE_ATTR(ec_address, S_IRUGO | S_IWUSR, ec_read_addr, ec_write_addr);
static DEVICE_ATTR(ec_qmethod, S_IWUSR, NULL, ec_exec_qmethod);

#endif

/* Memory */
static DEVICE_ATTR(mem_address, S_IRUGO | S_IWUSR, mem_read_address, mem_write_address);
static DEVICE_ATTR(mem_data, S_IRUGO | S_IWUSR, mem_read_data, mem_write_data);

/* I/O */
static DEVICE_ATTR(io_address, S_IRUGO | S_IWUSR, io_read_address, io_write_address);
static DEVICE_ATTR(iow_data, S_IRUGO | S_IWUSR, iow_read_data, iow_write_data);
static DEVICE_ATTR(iob_data, S_IRUGO | S_IWUSR, iob_read_data, iob_write_data);

/* PCI */
extern Pci_dev pci_dev;
static DEVICE_ATTR(pci_data, S_IRUGO | S_IWUSR, pci_read_data, pci_write_data);
static DEVICE_ATTR(pci_reg, S_IRUGO | S_IWUSR, pci_read_offset, pci_write_offset);
static DEVICE_ATTR(pci_id, S_IRUGO | S_IWUSR, pci_read_ids, pci_write_ids);

/* CMOS */
static DEVICE_ATTR(cmos, S_IRUGO | S_IWUSR, cmos_read_data, cmos_write_addr);

/* MSR */
static DEVICE_ATTR(msr, S_IRUGO | S_IWUSR, msr_read_data, msr_set_register);

#ifdef CONFIG_ACPI
static struct attribute *fwdt_acpi_sysfs_entries[] = {
	&dev_attr_acpi_arg0.attr,
	&dev_attr_acpi_arg1.attr,
	&dev_attr_acpi_method_1_0.attr,
	&dev_attr_acpi_method_1_1.attr,
	&dev_attr_acpi_method_0_1.attr,
	&dev_attr_acpi_method_0_0.attr,
	&dev_attr_video_device.attr,
	&dev_attr_video_brightness.attr,
	NULL,
};

static struct attribute_group acpi_attr_group = {
	.name   = NULL,         /* put in device directory */
	.attrs  = fwdt_acpi_sysfs_entries,
};

static struct attribute *fwdt_acpi_ec_sysfs_entries[] = {
	&dev_attr_ec_address.attr,
	&dev_attr_ec_data.attr,
	&dev_attr_ec_qmethod.attr,
	NULL,
};

static struct attribute_group acpi_ec_attr_group = {
	.name   = NULL,         /* put in device directory */
	.attrs  = fwdt_acpi_ec_sysfs_entries,
};
#endif

static struct attribute *fwdt_memory_sysfs_entries[] = {
	&dev_attr_mem_address.attr,
	&dev_attr_mem_data.attr,
	NULL,
};

static struct attribute_group memory_attr_group = {
	.name   = NULL,         /* put in device directory */
	.attrs  = fwdt_memory_sysfs_entries,
};

static struct attribute *fwdt_io_sysfs_entries[] = {
	&dev_attr_io_address.attr,
	&dev_attr_iow_data.attr,
	&dev_attr_iob_data.attr,
	NULL,
};

static struct attribute_group io_attr_group = {
	.name   = NULL,         /* put in device directory */
	.attrs  = fwdt_io_sysfs_entries,
};

static struct attribute *fwdt_pci_sysfs_entries[] = {
	&dev_attr_pci_id.attr,
	&dev_attr_pci_reg.attr,
	&dev_attr_pci_data.attr,
	NULL,
};

static struct attribute_group pci_attr_group = {
	.name   = NULL,         /* put in device directory */
	.attrs  = fwdt_pci_sysfs_entries,
};

static long fwdt_runtime_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	int err;

	switch (cmd) {
#ifdef CONFIG_ACPI
	case FWDT_ACPI_VGA_CMD:
		err = handle_acpi_vga_cmd((fwdt_generic __user *) arg);
		break;
	case FWDT_ACPI_EC_CMD:
		err = handle_acpi_ec_cmd((fwdt_generic __user *) arg);
		break;
	case FWDT_ACPI_AML_CMD:
		err = handle_acpi_aml_cmd((fwdt_generic __user *) arg);
		break;
#endif
	case FWDT_HW_ACCESS_IO_CMD:
		err = handle_hardware_io_cmd((fwdt_generic __user *) arg);
		break;
	case FWDT_HW_ACCESS_MEMORY_CMD:
		err = handle_hardware_memory_cmd((fwdt_generic __user *) arg);
		break;
	case FWDT_HW_ACCESS_CMOS_CMD:
		err = handle_hardware_cmos_cmd((fwdt_generic __user *) arg);
		break;

	default:
		err = -ENOTTY;
		break;
	}

	return err;
}

static struct semaphore fwdt_lock;
static int fwdt_runtime_open(struct inode *inode, struct file *file)
{
	if (down_trylock(&fwdt_lock))
		return -EBUSY;

	return 0;
}

static int fwdt_runtime_close(struct inode *inode, struct file *file)
{
	up(&fwdt_lock);
	return 0;
}

static const struct file_operations fwdt_runtime_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = fwdt_runtime_ioctl,
	.open		= fwdt_runtime_open,
	.release	= fwdt_runtime_close,
	.llseek		= no_llseek,
};

static struct miscdevice fwdt_runtime_dev = {
	MISC_DYNAMIC_MINOR,
	"fwdt",
	&fwdt_runtime_fops
};

static void cleanup_sysfs(struct platform_device *device)
{
#ifdef CONFIG_ACPI
	sysfs_remove_group(&device->dev.kobj, &acpi_attr_group);

	if (video_device)
		video_device = NULL;

	if (ec_device) {
		sysfs_remove_group(&device->dev.kobj, &acpi_ec_attr_group);
		ec_device = NULL;
	}
#endif

	sysfs_remove_group(&device->dev.kobj, &memory_attr_group);
	sysfs_remove_group(&device->dev.kobj, &io_attr_group);
	sysfs_remove_group(&device->dev.kobj, &pci_attr_group);
	device_remove_file(&device->dev, &dev_attr_cmos);
	device_remove_file(&device->dev, &dev_attr_msr);
}

static int fwdt_setup(struct platform_device *device)
{
	int err;

#ifdef CONFIG_ACPI
	acpi_status status;

	err = sysfs_create_group(&device->dev.kobj, &acpi_attr_group);
	if (err)
		goto add_sysfs_error;

	status = acpi_get_devices("PNP0C09", acpi_handle_locate_callback,
				  NULL, &ec_device);
	if (ACPI_SUCCESS(status)) {
		if (ec_device) {
			err = sysfs_create_group(&device->dev.kobj, &acpi_ec_attr_group);
			if (err)
				goto add_sysfs_error;
		}
	}
#endif

	err = sysfs_create_group(&device->dev.kobj, &memory_attr_group);
	if (err)
		goto add_sysfs_error;
	err = sysfs_create_group(&device->dev.kobj, &io_attr_group);
	if (err)
		goto add_sysfs_error;
	err = sysfs_create_group(&device->dev.kobj, &pci_attr_group);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_cmos);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_msr);
	if (err)
		goto add_sysfs_error;

	return 0;

 add_sysfs_error:
	cleanup_sysfs(device);
	return err;
}

static int __exit fwdt_remove(struct platform_device *device)
{
	cleanup_sysfs(device);
	return 0;
}

static int __init fwdt_init(void)
{
	int err;
	pr_info("initializing fwdt module\n");

	err = platform_driver_register(&fwdt_driver);
	if (err)
		goto err_driver_reg;
	fwdt_platform_dev = platform_device_alloc("fwdt", -1);
	if (!fwdt_platform_dev) {
		err = -ENOMEM;
		goto err_device_alloc;
	}
	err = platform_device_add(fwdt_platform_dev);
	if (err)
		goto err_device_add;

	err = misc_register(&fwdt_runtime_dev);
	if (err) {
		printk(KERN_ERR "fwdt: can't misc_register on minor=%d\n",
					MISC_DYNAMIC_MINOR);
		goto err_driver_reg;
	}

	sema_init(&fwdt_lock, 1);

	memset(&pci_dev, 0xFF, sizeof(pci_dev));

	return 0;

 err_device_add:
	platform_device_put(fwdt_platform_dev);
 err_device_alloc:
	platform_driver_unregister(&fwdt_driver);
 err_driver_reg:

	return err;
}

static void __exit fwdt_exit(void)
{
	pr_info("exiting fwdt module\n");
	if (fwdt_platform_dev) {
		platform_device_unregister(fwdt_platform_dev);
		platform_driver_unregister(&fwdt_driver);
	}

	misc_deregister(&fwdt_runtime_dev);
}

module_init(fwdt_init);
module_exit(fwdt_exit);
