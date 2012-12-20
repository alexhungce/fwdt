
#ifndef __FWDT_H__
#define __FWDT_H__

#define FWDT_ACPI_VGA_CMD	0x01
enum fwdt_acpi_vga_sub_cmd {
	GET_BRIGHTNESS		= 	0x01,
	SET_BRIGHTNESS		=	0x02,
	GET_BRIGHTNESS_LV	=	0x03,
	GET_VIDEO_DEVICE	=	0x04,
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

typedef struct {
	fwdt_parameter	parameters;
	char		lcd_path[256];
	u32 		brightness_level;
} fwdt_brightness;

typedef struct {
	fwdt_parameter	parameters;
	char		lcd_path[256];
	u32 		num_of_levels;
	u32		levels[256];
} fwdt_brightness_levels;


typedef struct {
	fwdt_parameter	parameters;
} fwdt_generic;

#endif
