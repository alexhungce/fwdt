/*
 * FWDT driver
 *
 * Copyright(C) 2014-2016 Canonical Ltd.
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

#ifndef __FWDT_H__
#define __FWDT_H__

enum fwdt_acpi_vga_sub_cmd {
	GET_BRIGHTNESS		= 	0x01,
	SET_BRIGHTNESS		=	0x02,
	GET_BRIGHTNESS_LV	=	0x03,
	GET_VIDEO_DEVICE	=	0x04,
};

enum fwdt_acpi_ec_sub_cmd {
	GET_EC_REGISTER		= 	0x01,
	SET_EC_REGISTER		=	0x02,
};

enum fwdt_hw_access_sub_cmd {
	GET_DATA_BYTE		= 	0x01,
	SET_DATA_BYTE		=	0x02,
	GET_DATA_WORD		=	0x03,
	SET_DATA_WORD		=	0x04,
	GET_DATA_DWORD		= 	0x05,
	SET_DATA_DWORD		= 	0x06,
};

typedef struct {
	union {
		u16 func;
		u16 func_status;
	};
	u16	reserved;
} fwdt_parameter;

#define FWDT_SUCCESS		0
#define FWDT_FAIL		1
#define FWDT_DEVICE_NOT_FOUND	-2
#define FWDT_FUNC_NOT_SUPPORTED	-1

struct fwdt_brightness {
	fwdt_parameter	parameters;
	char		lcd_path[256];
	union {
		u32 		brightness_level;
		u32 		num_of_levels;
	};
	u32             levels[256];
} __attribute__ ((packed));

struct fwdt_io_data {
	fwdt_parameter	parameters;
	u16		io_address;
	union {
		u8	io_byte;
		u16	io_word;
	};
} __attribute__ ((packed));

struct fwdt_mem_data {
	fwdt_parameter	parameters;
	u64		mem_address;
	u32		mem_data;
} __attribute__ ((packed));

struct fwdt_cmos_data {
	fwdt_parameter	parameters;
	u8		cmos_address;
	u8		cmos_data;
} __attribute__ ((packed));

struct fwdt_ec_data {
	fwdt_parameter	parameters;
	u8		address;
	u8		data;
} __attribute__ ((packed));

typedef struct {
	fwdt_parameter	parameters;
} fwdt_generic;


#define FWDT_ACPI_VGA_CMD \
        _IOWR('p', 0x01, struct fwdt_brightness)

#define FWDT_HW_ACCESS_IO_CMD \
        _IOWR('p', 0x02, struct fwdt_io_data)

#define FWDT_HW_ACCESS_MEMORY_CMD \
        _IOWR('p', 0x03, struct fwdt_mem_data)

#define FWDT_HW_ACCESS_CMOS_CMD \
        _IOWR('p', 0x04, struct fwdt_cmos_data)

#define FWDT_ACPI_EC_CMD \
        _IOWR('p', 0x05, struct fwdt_brightness)
#endif
