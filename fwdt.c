/*
 * FWDT driver
 *
 * Copyright(C) 2012 Canonical Ltd.
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <acpi/acpi_drivers.h>
#include <linux/pci.h>
#include <acpi/acpi_bus.h>
#include <acpi/video.h>

MODULE_AUTHOR("Alex Hung");
MODULE_DESCRIPTION("FWDT Driver");
MODULE_LICENSE("GPL");

static int __devinit fwdt_setup(struct platform_device *device);
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

static acpi_status acpi_acpi_handle_locate_callback(acpi_handle handle,
			u32 level, void *context, void **return_value)
{
	*(acpi_handle *)return_value = handle;

	return AE_CTRL_TERMINATE;
}

static u32 mem_addr;
static ssize_t mem_read_address(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%08x\n", mem_addr);
}

static ssize_t mem_write_address(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	mem_addr = simple_strtoul(buf, NULL, 16);
	
	return count;
}

static DEVICE_ATTR(mem_address, S_IRUGO | S_IWUSR, mem_read_address, mem_write_address);

static ssize_t mem_read_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	u32 *mem;
	u32 data;

	mem = ioremap(mem_addr, 8);
	data = *mem;
	iounmap(mem);

	return sprintf(buf, "0x%08x\n", data);
}

static ssize_t mem_write_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	u32 *mem;
	u32 data;

	data = simple_strtoul(buf, NULL, 16) & 0xFFFFFFFF;
	
	mem = ioremap(mem_addr, 8);
	*mem = data;
	iounmap(mem);

	return count;
}

static DEVICE_ATTR(mem_data, S_IRUGO | S_IWUSR, mem_read_data, mem_write_data);

static u16 iow_addr;
static ssize_t iow_read_address(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%04x\n", iow_addr);
}

static ssize_t iow_write_address(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	iow_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;
	
	return count;
}

static DEVICE_ATTR(iow_address, S_IRUGO | S_IWUSR, iow_read_address, iow_write_address);

static ssize_t iow_read_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%04x\n", inw(iow_addr));
}

static ssize_t iow_write_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	u16 data;

	data = simple_strtoul(buf, NULL, 16);
	outw(data, iow_addr);
	
	return count;
}

static DEVICE_ATTR(iow_data, S_IRUGO | S_IWUSR, iow_read_data, iow_write_data);

static u16 iob_addr;
static ssize_t iob_read_address(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%04x\n", iob_addr);
}

static ssize_t iob_write_address(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	iob_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;
	
	return count;
}

static DEVICE_ATTR(iob_address, S_IRUGO | S_IWUSR, iob_read_address, iob_write_address);

static ssize_t iob_read_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%02x\n", inb(iob_addr));
}

static ssize_t iob_write_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	u8 data;

	data = simple_strtoul(buf, NULL, 16);
	outb(data, iob_addr);
	
	return count;
}

static DEVICE_ATTR(iob_data, S_IRUGO | S_IWUSR, iob_read_data, iob_write_data);

static struct {
	u16 vendor_id;
	u16 device_id;
	u8 reg_offset;
} pci_dev_info;

static ssize_t pci_read_config_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct pci_dev *pdev = NULL;
	int data;

	pdev = pci_get_subsys(pci_dev_info.vendor_id, pci_dev_info.device_id,
				PCI_ANY_ID, PCI_ANY_ID, NULL);
	if (pdev == NULL) {
		pr_info("pci device [%04x:%04x] is not found\n", 
			pci_dev_info.vendor_id, pci_dev_info.device_id);
		return -EINVAL;
	}

	pci_read_config_dword(pdev, pci_dev_info.reg_offset, &data);

	return sprintf(buf, "0x%08x\n", data);;
}

static ssize_t pci_write_config_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct pci_dev *pdev = NULL;
	int data;

	data = simple_strtoul(buf, NULL, 16) & 0xFFFFFFFF;
	pdev = pci_get_subsys(pci_dev_info.vendor_id, pci_dev_info.device_id,
				PCI_ANY_ID, PCI_ANY_ID, NULL);
	if (pdev)
		pci_write_config_dword(pdev, pci_dev_info.reg_offset, data);
	else 
		pr_info("pci device [%04x:%04x] is not found\n", 
			pci_dev_info.vendor_id, pci_dev_info.device_id);

	return count;
}

static DEVICE_ATTR(pci_data, S_IRUGO | S_IWUSR, pci_read_config_data, pci_write_config_data);

static ssize_t pci_read_config_offset(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%x\n", pci_dev_info.reg_offset);;
}

static ssize_t pci_write_config_offset(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	pci_dev_info.reg_offset = simple_strtoul(buf, NULL, 16) & 0xFF;
	return count;
}

static DEVICE_ATTR(pci_reg, S_IRUGO | S_IWUSR, pci_read_config_offset, pci_write_config_offset);

static ssize_t pci_read_hardware_ids(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	u32 pci_id;

	pci_id= (pci_dev_info.vendor_id << 16) + (pci_dev_info.device_id);

	return sprintf(buf, "0x%08x\n", pci_id);;
}

static ssize_t pci_write_hardware_ids(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	long pci_id;

	pci_id = simple_strtoul(buf, NULL, 16);
	pci_dev_info.device_id = (pci_id & 0xFFFF0000) >> 16;
	pci_dev_info.vendor_id = (pci_id & 0x0000FFFF);

	return count;
}

static DEVICE_ATTR(pci_id, S_IRUGO | S_IWUSR, pci_read_hardware_ids, pci_write_hardware_ids);

static acpi_handle ec_device = NULL;
static int ec_offset;
static ssize_t acpi_read_ec_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	u8 data;

	ret = ec_read(ec_offset, &data);
	if (ret)
		return -EINVAL;

	return sprintf(buf, "%x\n", data);;
}

static ssize_t acpi_write_ec_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret;
	u8 data;

	data = simple_strtoul(buf, NULL, 16);
	ret = ec_write(ec_offset, data);
	if (ret)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(ec_data, S_IRUGO | S_IWUSR, acpi_read_ec_data, acpi_write_ec_data);

static ssize_t acpi_read_ec_addr(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "0x%02x\n", ec_offset);;
}

static ssize_t acpi_write_ec_addr(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	ec_offset = simple_strtoul(buf, NULL, 16);
	return count;
}

static DEVICE_ATTR(ec_address, S_IRUGO | S_IWUSR, acpi_read_ec_addr, acpi_write_ec_addr);

static ssize_t acpi_write_ec_qxx(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
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

static DEVICE_ATTR(ec_qmethod, S_IWUSR, NULL, acpi_write_ec_qxx);

static void cleanup_sysfs(struct platform_device *device)
{
	device_remove_file(&device->dev, &dev_attr_mem_address);
	device_remove_file(&device->dev, &dev_attr_mem_data);
	device_remove_file(&device->dev, &dev_attr_iow_address);
	device_remove_file(&device->dev, &dev_attr_iow_data);
	device_remove_file(&device->dev, &dev_attr_iob_address);
	device_remove_file(&device->dev, &dev_attr_iob_data);
	device_remove_file(&device->dev, &dev_attr_pci_id);
	device_remove_file(&device->dev, &dev_attr_pci_reg);
	device_remove_file(&device->dev, &dev_attr_pci_data);

	if (ec_device) {
		device_remove_file(&device->dev, &dev_attr_ec_address);
		device_remove_file(&device->dev, &dev_attr_ec_data);
		device_remove_file(&device->dev, &dev_attr_ec_qmethod);
		ec_device = NULL;
	}
}

static int __devinit fwdt_setup(struct platform_device *device)
{
	int err;
	acpi_status status;

	err = device_create_file(&device->dev, &dev_attr_mem_address);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_mem_data);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_iow_address);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_iow_data);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_iob_address);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_iob_data);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_pci_id);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_pci_reg);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_pci_data);
	if (err)
		goto add_sysfs_error;

	status = acpi_get_devices("PNP0C09", acpi_acpi_handle_locate_callback,
				  NULL, &ec_device);
	if (ACPI_SUCCESS(status)) {
		if (!ec_device)
			goto add_sysfs_done;
		err = device_create_file(&device->dev, &dev_attr_ec_address);
		if (err)
			goto add_sysfs_error;
		err = device_create_file(&device->dev, &dev_attr_ec_data);
		if (err)
			goto add_sysfs_error;
		err = device_create_file(&device->dev, &dev_attr_ec_qmethod);
		if (err)
			goto add_sysfs_error;
	}

add_sysfs_done:

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
}

module_init(fwdt_init);
module_exit(fwdt_exit);
