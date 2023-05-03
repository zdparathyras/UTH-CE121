compile: mylib hw2 dirlist p2archive p2crypt p2unarchive

mylib: mylib.c
	gcc -Wall -c mylib.c -o mylib.o

hw2: hw2.c
	gcc -Wall -g hw2.c mylib.o -o hw2

dirlist: dirlist.c
	gcc -Wall -g dirlist.c mylib.o -o dirlist

p2archive: p2archive.c
	gcc -Wall -g p2archive.c mylib.o -o p2archive

p2crypt: p2crypt.c
	gcc -Wall -g p2crypt.c mylib.o -o p2crypt

p2unarchive: p2unarchive.c
	gcc -Wall -g p2unarchive.c mylib.o -o p2unarchive

