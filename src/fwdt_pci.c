/*
 * FWDT PCI driver
 *
 * Copyright(C) 2016-2021 Canonical Ltd.
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
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/semaphore.h>

#ifdef CONFIG_PCI

Pci_dev pci_dev;

ssize_t pci_read_data(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	struct pci_dev *pdev = NULL;
	int data;

	pdev = pci_get_device(pci_dev.vid, pci_dev.did, NULL);
	if (pdev == NULL) {
		pr_info("pci device [%04x:%04x] is not found\n", pci_dev.vid,
			pci_dev.did);
		return -EINVAL;
	}

	pci_read_config_dword(pdev, pci_dev.offset, &data);

	return sprintf(buf, "0x%08x\n", data);
	;
}

ssize_t pci_write_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct pci_dev *pdev = NULL;
	int data;

	data = simple_strtoul(buf, NULL, 16) & 0xFFFFFFFF;
	pdev = pci_get_device(pci_dev.vid, pci_dev.did, NULL);
	if (pdev)
		pci_write_config_dword(pdev, pci_dev.offset, data);
	else
		pr_info("pci device [%04x:%04x] is not found\n", pci_dev.vid,
			pci_dev.did);

	return count;
}

ssize_t pci_read_offset(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%x\n", pci_dev.offset);
}

ssize_t pci_write_offset(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	pci_dev.offset = simple_strtoul(buf, NULL, 16) & 0xFF;
	return count;
}

ssize_t pci_read_ids(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	if (pci_dev.vid == 0xFFFF || pci_dev.did == 0xFFFF)
		strcpy(buf, "ex. 8086:1c2d\n");
	else
		sprintf(buf, "%04x:%04x\n", pci_dev.vid, pci_dev.did);

	return strlen(buf);
}

ssize_t pci_write_ids(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count)
{
	unsigned int vendor_id, device_id;

	if (strlen(buf) > 10)
		return 0;

	sscanf(buf, "%4x:%4x\n", &vendor_id, &device_id);

	pci_dev.did = device_id;
	pci_dev.vid = vendor_id;

	return count;
}

#endif
