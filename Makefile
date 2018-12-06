Mountpoint=mountpoint

all: myfs.c myfs.h $(Mountpoint)
	pkill myserver || true
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`
	kldload -n fuse
	./myfs $(Mountpoint)

mountpoint:
	mkdir mountpoint

compile: myfs.c myfs.h
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`

test:
	sh test.sh

clean:
	rm myfs
