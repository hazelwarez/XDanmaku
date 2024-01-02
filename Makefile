cc=gcc -lX11 -lXext

default: build

.PHONY: rebuild

build:
	$(cc) bullet.c list.c xdanmaku.c -o xdanmaku

clean:
	-rm -f xdanmaku

rebuild:
	$(MAKE) clean
	$(MAKE) build

run:
	$(MAKE) rebuild
	./xdanmaku
