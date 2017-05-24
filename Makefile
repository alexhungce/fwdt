all:
	make -Werror -C src
	make -Werror -C apps hp_cmos ec_reg
	mkdir -p bin
	install src/fwdt.ko bin/
	install apps/hp_cmos bin/
	install apps/ec_reg bin/

clean:
	make -C src clean
	rm -f apps/hp_cmos apps/ec_reg bin/*
