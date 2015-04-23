CFLAGS = -I.

all: dtfs_tree

libdtfs.o: libdtfs.c

dtfs_tree.o: dtfs_tree.c

libdtfs.a: libdtfs.o
	$(AR) r $@ $<

dtfs_tree: libdtfs.a dtfs_tree.o

clean:
	-rm libdtfs.a dtfs_tree.o libdtfs.o
