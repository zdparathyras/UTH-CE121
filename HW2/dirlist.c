#include "mylib.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

int main (int argc, char * argv[]) {
	char *path;
	DIR *dir;
	struct dirent *pDirent;
	
	if (argc!=2) {
		fprintf(stderr, "Wrong number of parameters.\n");
		exit(0);
	}
	
	path=realpath(argv[1],NULL);
	if (path!=NULL) {
		dir=opendir(path);
	}
	else {
		fprintf(stderr, "REALPATH didn't work.\n");
		exit(3);
	}
	
	if (dir==NULL) {
		perror("opendir");
		exit(2);
	} 
	
	while ((pDirent = readdir(dir)) != NULL) {
		 if (strcmp(pDirent->d_name,".")!=0 && strcmp(pDirent->d_name,"..")!=0 && pDirent->d_type!=DT_DIR ){
			ErrorFinder(write(1, path, strlen(path)),__LINE__);
			ErrorFinder(write(1,"/", sizeof(char)),__LINE__);
			ErrorFinder(write(1,pDirent->d_name, strlen(pDirent->d_name)),__LINE__);
			ErrorFinder(write(1,"\n",sizeof(char)),__LINE__);
		 }
	}
	
	free(path);
	closedir (dir);
	
	
	return 0;
}
