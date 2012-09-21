PREFIX=/usr/local

all:
	@make -C entityx all

test:
	@make -C entityx test

clean:
	@make -C entityx clean

cleantests:
	@make -C entityx cleantests

distclean:
	@make -C entityx distclean

install: all
	install -m755 ./entityx/libentity.so $(PREFIX)/lib
	install -m644 ./entityx/libentity.a $(PREFIX)/lib
	install -m644 ./entityx/*.h $(PREFIX)/include
