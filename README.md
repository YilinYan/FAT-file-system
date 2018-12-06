# Intro Operating Sys Assgn4
Name: Yilin Yan
ID: yyan228

## All Files
DESIGN.pdf
README.txt
test.sh
myfs.h
myfs.c
Makefile

## Manual
Compile and install file system:
$ make
Then a folder named ```mountpoint``` will be created.
Enter the folder to use the file system.

Test the file system:
$ make test
A shell script will run to test the performance of
the file system.

Change the block size and block number:
Open ```myfs.h```, you will see two macros
```#define BLOCK_SIZE 512```
```#define BLOCK_NUMBER 0x00100000```
Change these numbers and recompile.

Check debug log:
Open ```debug``` to see what's happening.
