/*
 * FWDT driver
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

#ifndef __FWDT_LIB_H__
#define __FWDT_LIB_H__

#include <linux/acpi.h>
#include "fwdt.h"

#ifdef CONFIG_ACPI

#define ACPI_PATH_SIZE  80

/* ACPI functions */
acpi_status acpi_handle_locate_callback(acpi_handle handle, u32 level,
					void *context, void **return_value);
void acpi_device_path(const char *buf, char *path);
ssize_t acpi_method_0_0_write(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count);
ssize_t acpi_method_0_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
ssize_t acpi_arg0_read(struct device *dev, struct device_attribute *attr,
		       char *buf);
ssize_t acpi_arg0_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);
ssize_t acpi_arg1_read(struct device *dev, struct device_attribute *attr,
		       char *buf);
ssize_t acpi_arg1_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);
ssize_t acpi_method_1_0_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
ssize_t acpi_method_1_0_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
ssize_t acpi_method_name_write(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count);
ssize_t acpi_method_1_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
ssize_t acpi_method_2_0_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
ssize_t acpi_method_2_1_read(struct device *dev, struct device_attribute *attr,
			     char *buf);
int handle_acpi_aml_cmd(fwdt_generic __user *fg);

/* ACPI VGA */
int acpi_lcd_query_levels(acpi_handle *device, union acpi_object **levels);
ssize_t acpi_video_write_device(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count);
ssize_t acpi_video_read_brightness(struct device *dev,
				   struct device_attribute *attr, char *buf);
ssize_t acpi_video_write_brightness(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count);
int handle_acpi_vga_cmd(fwdt_generic __user *fg);

/* ACPI EC functions */
ssize_t ec_read_data(struct device *dev, struct device_attribute *attr,
		     char *buf);
ssize_t ec_write_data(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count);
ssize_t ec_read_addr(struct device *dev, struct device_attribute *attr,
		     char *buf);
ssize_t ec_write_addr(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count);
ssize_t ec_exec_qmethod(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);
int handle_acpi_ec_cmd(fwdt_generic __user *fg);

#endif

/* Memory functions */
ssize_t mem_read_address(struct device *dev, struct device_attribute *attr,
			 char *buf);
ssize_t mem_write_address(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count);
ssize_t mem_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf);
ssize_t mem_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count);
int handle_hardware_memory_cmd(fwdt_generic __user *fg);

#ifdef CONFIG_X86

/* I/O functions */
ssize_t io_read_address(struct device *dev, struct device_attribute *attr,
			char *buf);
ssize_t io_write_address(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
ssize_t iow_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf);
ssize_t iow_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count);
ssize_t iob_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf);
ssize_t iob_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count);
int handle_hardware_io_cmd(fwdt_generic __user *fg);

/* CMOS functions */
ssize_t cmos_read_data(struct device *dev, struct device_attribute *attr,
		       char *buf);
ssize_t cmos_write_addr(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);
int handle_hardware_cmos_cmd(fwdt_generic __user *fg);

/* MSR functions */
ssize_t msr_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf);
ssize_t msr_set_register(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);

#endif

/* PCI functions */
typedef struct {
	u16 vid;
	u16 did;
	u8 offset;
} Pci_dev;

#ifdef CONFIG_PCI

ssize_t pci_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf);
ssize_t pci_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count);
ssize_t pci_read_offset(struct device *dev, struct device_attribute *attr,
			char *buf);
ssize_t pci_write_offset(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
ssize_t pci_write_ids(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count);
ssize_t pci_read_ids(struct device *dev, struct device_attribute *attr,
		     char *buf);
;

#endif

#endif
