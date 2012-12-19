
#ifndef __FWDT_H__
#define __FWDT_H__

typedef struct {
	union {
		u16 func;
		u16 func_status;
	};
} fwdt_parameter;

#define FWDT_SUCCESS		0
#define FWDT_FAIL		1


#endif
