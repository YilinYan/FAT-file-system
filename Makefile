Mountpoint=mountpoint

all: myfs.c myfs.h $(Mountpoint)
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`
	kldload -n fuse
	./myfs $(Mountpoint)

mountpoint:
	mkdir mountpoint

test: myfs.c myfs.h
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`

example=hello.c

xmp: $(example)
	cc $(example) -o xmpprog `pkgconf fuse --cflags --libs`
	kldload -n fuse
	./xmpprog example

