all:
	make -C src
	make -C apps hp_cmos
	mkdir -p bin
	install src/fwdt.ko bin/
	install apps/hp_cmos bin/

clean:
	make -C src clean
	rm -f apps/hp_cmos bin/*
