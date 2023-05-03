//my library
#ifndef _mylib_h
#define _mylib_h

void ErrorFinder(int fd, int line);
long int myread(int fd, char * result, long int max_bytes);
long int mywrite (int fd, char str[],long int max_bytes);

#endif
