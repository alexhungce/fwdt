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

#define pr_fmt(fmt) "fwdt: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <acpi/acpi_drivers.h>
#include <linux/pci.h>
#include <acpi/acpi_bus.h>
#include <acpi/video.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <asm/time.h>

#include "fwdt.h"

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

static acpi_status acpi_handle_locate_callback(acpi_handle handle,
			u32 level, void *context, void **return_value)
{
	*(acpi_handle *)return_value = handle;

	return AE_CTRL_TERMINATE;
}

static void acpi_device_path(const char *buf, char *path)
{
	path[0] = '\\';
	strncpy(path + 1, buf, strlen(buf));	
	path[strlen(buf)] = 0;
}

static ssize_t acpi_method_0_0_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;
	char path[255];

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
static DEVICE_ATTR(acpi_method_0_0, S_IWUSR, NULL, acpi_method_0_0_write);

static char device_path_0_1[256];
static ssize_t acpi_method_0_1_read(struct device *dev,
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

static ssize_t acpi_method_0_1_write(struct device *dev,
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
static DEVICE_ATTR(acpi_method_0_1, S_IRUGO | S_IWUSR,
		acpi_method_0_1_read, acpi_method_0_1_write);

static char acpi_method[256];
static ssize_t acpi_method_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", acpi_method);
}

static ssize_t acpi_method_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_handle device;
	acpi_status status;

	acpi_device_path(buf, acpi_method);

	status = acpi_get_handle(NULL, acpi_method, &device);
	if (!ACPI_SUCCESS(status)) {
		printk("Failed to find acpi method: %s\n", acpi_method);
	}

	return count;
}

static DEVICE_ATTR(acpi_method, S_IRUGO | S_IWUSR,
		acpi_method_read, acpi_method_write);

static u32 acpi_arg0;
static ssize_t acpi_arg1_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", acpi_arg0);
}

static ssize_t acpi_arg1_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	acpi_arg0 = simple_strtoul(buf, NULL, 16);

	return count;
}

static DEVICE_ATTR(acpi_arg0, S_IRUGO | S_IWUSR,
		acpi_arg1_read, acpi_arg1_write);

static ssize_t acpi_method_1_1_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	acpi_status status;
	unsigned long long output;
	union acpi_object arg0 = { ACPI_TYPE_INTEGER };
	struct acpi_object_list args = { 1, &arg0 };

	arg0.integer.value = acpi_arg0;

	status = acpi_evaluate_integer(NULL, acpi_method, &args, &output);
	if (ACPI_SUCCESS(status))
		printk("Executed %s\n", acpi_method);
	else
		printk("Failed to execute %s\n", acpi_method);

	return sprintf(buf, "0x%08llx\n", output);
}

static DEVICE_ATTR(acpi_method_1_1, S_IRUGO, acpi_method_1_1_read, NULL);

static int acpi_lcd_query_levels(acpi_handle *device,
				   union acpi_object **levels)
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

static acpi_handle video_device;
static ssize_t acpi_video_write_device(struct device *dev,
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

static DEVICE_ATTR(video_device, S_IWUSR, NULL, acpi_video_write_device);

static ssize_t acpi_video_read_brightness(struct device *dev,
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
		if (o->type != ACPI_TYPE_INTEGER)
			continue;
		printk("Brightness[%d] = %d\n", i, (u32) o->integer.value);
	}

      no_bcl:

	return sprintf(buf, "%lld\n", bqc_level);
}

static ssize_t acpi_video_write_brightness(struct device *dev,
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

static DEVICE_ATTR(video_brightness, S_IRUGO | S_IWUSR,
	acpi_video_read_brightness, acpi_video_write_brightness);

static u32 mem_addr;
static ssize_t mem_read_address(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%08x\n", mem_addr);
}

static ssize_t mem_write_address(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	mem_addr = simple_strtoul(buf, NULL, 16);
	
	return count;
}

static DEVICE_ATTR(mem_address, S_IRUGO | S_IWUSR,
		mem_read_address, mem_write_address);

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
static ssize_t iow_read_address(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%04x\n", iow_addr);
}

static ssize_t iow_write_address(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	iow_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;
	
	return count;
}

static DEVICE_ATTR(iow_address, S_IRUGO | S_IWUSR,
		iow_read_address, iow_write_address);

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
static ssize_t iob_read_address(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%04x\n", iob_addr);
}

static ssize_t iob_write_address(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	iob_addr = simple_strtoul(buf, NULL, 16) & 0xFFFF;
	
	return count;
}

static DEVICE_ATTR(iob_address, S_IRUGO | S_IWUSR,
		iob_read_address, iob_write_address);

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
	u16 vid;
	u16 did;
	u8 offset;
} pci_dev;

static ssize_t pci_read_config_data(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct pci_dev *pdev = NULL;
	int data;

	pdev = pci_get_subsys(pci_dev.vid, pci_dev.did,
				PCI_ANY_ID, PCI_ANY_ID, NULL);
	if (pdev == NULL) {
		pr_info("pci device [%04x:%04x] is not found\n", 
			pci_dev.vid, pci_dev.did);
		return -EINVAL;
	}

	pci_read_config_dword(pdev, pci_dev.offset, &data);

	return sprintf(buf, "0x%08x\n", data);;
}

static ssize_t pci_write_config_data(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct pci_dev *pdev = NULL;
	int data;

	data = simple_strtoul(buf, NULL, 16) & 0xFFFFFFFF;
	pdev = pci_get_subsys(pci_dev.vid, pci_dev.did,
				PCI_ANY_ID, PCI_ANY_ID, NULL);
	if (pdev)
		pci_write_config_dword(pdev, pci_dev.offset, data);
	else 
		pr_info("pci device [%04x:%04x] is not found\n", 
			pci_dev.vid, pci_dev.did);

	return count;
}

static DEVICE_ATTR(pci_data, S_IRUGO | S_IWUSR,
		pci_read_config_data, pci_write_config_data);

static ssize_t pci_read_config_offset(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", pci_dev.offset);
}

static ssize_t pci_write_config_offset(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	pci_dev.offset = simple_strtoul(buf, NULL, 16) & 0xFF;
	return count;
}

static DEVICE_ATTR(pci_reg, S_IRUGO | S_IWUSR,
		pci_read_config_offset, pci_write_config_offset);

static ssize_t pci_read_hardware_ids(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (pci_dev.vid == 0xFFFF || pci_dev.did == 0xFFFF)
		strcpy(buf,"ex. 8086:1c2d\n");
	else
		sprintf(buf, "%04x:%04x\n", pci_dev.vid, pci_dev.did);

	return strlen(buf);
}

static ssize_t pci_write_hardware_ids(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int vendor_id, device_id;

	if (strlen(buf) > 10)
		return 0;

	sscanf(buf, "%4x:%4x\n", &vendor_id, &device_id);

	pci_dev.did = device_id;
	pci_dev.vid = vendor_id;

	return count;
}

static DEVICE_ATTR(pci_id, S_IRUGO | S_IWUSR,
	pci_read_hardware_ids, pci_write_hardware_ids);

static acpi_handle ec_device = NULL;
static int ec_offset;
static ssize_t acpi_read_ec_data(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	u8 data;

	ret = ec_read(ec_offset, &data);
	if (ret)
		return -EINVAL;

	return sprintf(buf, "%x\n", data);;
}

static ssize_t acpi_write_ec_data(struct device *dev,
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

static DEVICE_ATTR(ec_data, S_IRUGO | S_IWUSR,
		acpi_read_ec_data, acpi_write_ec_data);

static ssize_t acpi_read_ec_addr(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%02x\n", ec_offset);;
}

static ssize_t acpi_write_ec_addr(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ec_offset = simple_strtoul(buf, NULL, 16);
	return count;
}

static DEVICE_ATTR(ec_address, S_IRUGO | S_IWUSR,
		acpi_read_ec_addr, acpi_write_ec_addr);

static ssize_t acpi_write_ec_qxx(struct device *dev,
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

static DEVICE_ATTR(ec_qmethod, S_IWUSR, NULL, acpi_write_ec_qxx);

static int cmos_offset;
static ssize_t cmos_read_data(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (cmos_offset > 0xFF)
		return -EINVAL;

	return sprintf(buf, "0x%02x\n", CMOS_READ(cmos_offset));
}

static ssize_t cmos_write_addr(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (kstrtoint(buf, 16, &cmos_offset))
		return -EINVAL;
	return count;
}

static DEVICE_ATTR(cmos, S_IRUGO | S_IWUSR, cmos_read_data, cmos_write_addr);

static int get_acpi_vga_brightness(struct fwdt_brightness *fb)
{
	int status;
	unsigned long long bqc_level;
	acpi_handle lcd_device;

	status = acpi_get_handle(NULL, fb->lcd_path, &lcd_device);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to find acpi lcd device: %s\n", fb->lcd_path);
		fb->parameters.func_status = FWDT_DEVICE_NOT_FOUND;
		goto err;
	}

	status = acpi_evaluate_integer(lcd_device, "_BQC", NULL, &bqc_level);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to read brightness level!\n");
		fb->parameters.func_status = FWDT_FAIL;
		goto err;
	}

	fb->brightness_level = bqc_level;
	fb->parameters.func_status = FWDT_SUCCESS;
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
		fb->parameters.func_status = FWDT_DEVICE_NOT_FOUND;
		goto err;
	}

	status = acpi_evaluate_object(lcd_device, "_BCM", &args, NULL);
	if (!ACPI_SUCCESS(status)) {
		pr_info("Failed to set brightness level!\n");
		fb->parameters.func_status = FWDT_FAIL;
		goto err;
	}

	fb->parameters.func_status = FWDT_SUCCESS;
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
		fbl->parameters.func_status = FWDT_DEVICE_NOT_FOUND;
		goto err;
	}

	if (!ACPI_SUCCESS(acpi_lcd_query_levels(lcd_device, &obj))) {
		printk("Failed to query brightness levels\n");
		fbl->parameters.func_status = FWDT_FAIL;
		goto err;
	}

	fbl->num_of_levels = obj->package.count;
	for (i = 0; i < obj->package.count; i++) {
		o =  (union acpi_object *) &obj->package.elements[i];
		if (o->type != ACPI_TYPE_INTEGER)
			continue;
		fbl->levels[i] = (u32) o->integer.value;
	}

	fbl->parameters.func_status = FWDT_SUCCESS;
 err:
	return status;
}

static int handle_acpi_vga_cmd(fwdt_generic __user *fg) 
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
		err = FWDT_FUNC_NOT_SUPPORTED;
		break;
	}

	return err;
}

static int handle_hardware_io_cmd(fwdt_generic __user *fg) 
{
	int ret = 0;
	struct fwdt_io_data *fid = (struct fwdt_io_data*) fg;

	switch (fg->parameters.func) {
	case GET_DATA_BYTE:
		fid->io_byte = inb(fid->io_address);
		break;
	case GET_DATA_WORD:
		fid->io_word = inw(fid->io_address);
		break;	
	case SET_DATA_BYTE:
		outb(fid->io_byte, fid->io_address);
		break;
	case SET_DATA_WORD:
		outw(fid->io_word, fid->io_address);
		break;	
	default:
		ret = FWDT_FUNC_NOT_SUPPORTED;
		goto err;
		break;
	}

	fid->parameters.func_status = FWDT_SUCCESS;
 err:
	return ret;
}

static int handle_hardware_memory_cmd(fwdt_generic __user *fg) 
{
	int ret = 0;
	u64 *mem;
	struct fwdt_mem_data *fmd = (struct fwdt_mem_data*) fg;

	mem = ioremap(fmd->mem_address, 8);
	switch (fg->parameters.func) {
	case GET_DATA_DWORD:
		fmd->mem_data = *mem;
		break;	
	case SET_DATA_DWORD:
		*mem = fmd->mem_data;
		break;	
	default:
		ret = FWDT_FUNC_NOT_SUPPORTED;
		goto err;
		break;
	}

	fmd->parameters.func_status = FWDT_SUCCESS;
 err:
	iounmap(mem);
	return ret;
}

static int handle_hardware_cmos_cmd(fwdt_generic __user *fg) 
{
	int ret = 0;
	struct fwdt_cmos_data *fcd = (struct fwdt_cmos_data*) fg;

	switch (fg->parameters.func) {
	case GET_DATA_BYTE:
		fcd->cmos_data = CMOS_READ(fcd->cmos_address);
		break;	
	default:
		ret = FWDT_FUNC_NOT_SUPPORTED;
		goto err;
		break;
	}

	fcd->parameters.func_status = FWDT_SUCCESS;
 err:
	return ret;
}

static long fwdt_runtime_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	int err;

	switch (cmd) {
	case FWDT_ACPI_VGA_CMD:
		err = handle_acpi_vga_cmd((fwdt_generic __user *) arg);
		break;	
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
		err = FWDT_FUNC_NOT_SUPPORTED;
		break;
	}

	return err;
}

static int fwdt_runtime_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fwdt_runtime_close(struct inode *inode, struct file *file)
{
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
	device_remove_file(&device->dev, &dev_attr_acpi_method);
	device_remove_file(&device->dev, &dev_attr_acpi_arg0);
	device_remove_file(&device->dev, &dev_attr_acpi_method_1_1);
	device_remove_file(&device->dev, &dev_attr_acpi_method_0_1);
	device_remove_file(&device->dev, &dev_attr_acpi_method_0_0);
	device_remove_file(&device->dev, &dev_attr_video_device);
	device_remove_file(&device->dev, &dev_attr_video_brightness);
	device_remove_file(&device->dev, &dev_attr_mem_address);
	device_remove_file(&device->dev, &dev_attr_mem_data);
	device_remove_file(&device->dev, &dev_attr_iow_address);
	device_remove_file(&device->dev, &dev_attr_iow_data);
	device_remove_file(&device->dev, &dev_attr_iob_address);
	device_remove_file(&device->dev, &dev_attr_iob_data);
	device_remove_file(&device->dev, &dev_attr_pci_id);
	device_remove_file(&device->dev, &dev_attr_pci_reg);
	device_remove_file(&device->dev, &dev_attr_pci_data);
	device_remove_file(&device->dev, &dev_attr_cmos);

	if (video_device)
		video_device = NULL;

	if (ec_device) {
		device_remove_file(&device->dev, &dev_attr_ec_address);
		device_remove_file(&device->dev, &dev_attr_ec_data);
		device_remove_file(&device->dev, &dev_attr_ec_qmethod);
		ec_device = NULL;
	}
}

static int fwdt_setup(struct platform_device *device)
{
	int err;
	acpi_status status;

	err = device_create_file(&device->dev, &dev_attr_acpi_method);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_acpi_arg0);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_acpi_method_1_1);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_acpi_method_0_1);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_acpi_method_0_0);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_video_device);
	if (err)
		goto add_sysfs_error;
	err = device_create_file(&device->dev, &dev_attr_video_brightness);
	if (err)
		goto add_sysfs_error;
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
	err = device_create_file(&device->dev, &dev_attr_cmos);
	if (err)
		goto add_sysfs_error;

	status = acpi_get_devices("PNP0C09", acpi_handle_locate_callback,
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

	err = misc_register(&fwdt_runtime_dev);
	if (err) {
		printk(KERN_ERR "fwdt: can't misc_register on minor=%d\n",
					MISC_DYNAMIC_MINOR);
		goto err_driver_reg;
	}

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
