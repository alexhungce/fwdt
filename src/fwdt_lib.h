/*
 * FWDT driver
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

#ifndef __FWDT_LIB_H__
#define __FWDT_LIB_H__

#include "fwdt.h"

/* CMOS functions */
ssize_t cmos_read_data(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t cmos_write_addr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
int handle_hardware_cmos_cmd(fwdt_generic __user *fg);

/* ACPI EC functions */
ssize_t ec_read_data(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t ec_write_data(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t ec_read_addr(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t ec_write_addr(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t ec_exec_qmethod(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
int handle_acpi_ec_cmd(fwdt_generic __user *fg);

#endif
