all: myfs.c myfs.h
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`
	kldload -n fuse
	./myfs mountpoint

test: myfs.c myfs.h
	cc myfs.c -o myfs `pkgconf fuse --cflags --libs`

example=fat.c

xmp: $(example)
	cc $(example) -o xmpprog `pkgconf fuse --cflags --libs`
	kldload -n fuse
	./xmpprog example

