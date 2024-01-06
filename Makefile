libs=`pkg-config --libs x11 xext xft freetype2`
cflags=`pkg-config --cflags x11 xext xft freetype2`
cc=cc $(libs) $(cflags)

default: build

.PHONY: rebuild

build:
	$(cc) main.c xdanmaku.c -o xdanmaku

clean:
	-rm -f xdanmaku

rebuild:
	$(MAKE) clean
	$(MAKE) build

run:
	$(MAKE) rebuild
	./xdanmaku $(XDANMAKU_FLAGS)

woto:
	$(MAKE) rebuild
	./examples/xdanmaku-weechatlog.sh
