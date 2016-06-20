obj-m += fwdt.o

fwdt-objs := fwdt_core.o fwdt_cmos.o fwdt_ec.o fwdt_pci.o fwdt_io.o fwdt_mem.o \
	     fwdt_msr.o

all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean
