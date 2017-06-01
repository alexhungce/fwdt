all:
	make -Werror -C src
	make -Werror -C apps hp_cmos ec_reg fwdt
	mkdir -p bin
	install src/fwdt.ko bin/
	install apps/hp_cmos bin/
	install apps/ec_reg bin/
	install apps/fwdt bin/

clean:
	make -C src clean
	rm -f apps/fwdt apps/hp_cmos apps/ec_reg bin/*
