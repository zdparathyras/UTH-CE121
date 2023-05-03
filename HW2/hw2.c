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
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/wait.h> 

int main (int argc, char * argv[]) {
	int file_dest, pipe1[2], pipe2[2], pipe3[2],n;
	pid_t p2, p3 ,p4;
	char path[100], checktype[10];;
	strcpy(path, "/home/damianos/Desktop/CE121/HW2/");
	strcat(path,"\0");
	
	if (argc!=5) {
		fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
		fprintf(stderr,"Wrong number of parameters.\n");
		exit(5);
	}
	
	if (strcmp(argv[1],"-E")==0) {
		ErrorFinder(pipe(pipe1),__LINE__);
		ErrorFinder(pipe(pipe2),__LINE__);
		
		file_dest=open(argv[4],O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
		if (file_dest<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			perror("open");
			exit(4);
		}
		
		write(file_dest,"P2CRYPTAR",9);
		fsync(file_dest);
		
		p2=fork();
		
		if(p2<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			fprintf(stderr,"Fork for p2 didn't work.\n");
			exit(3);
		}
		else if (p2==0) {
			dup2(pipe1[1],STDOUT_FILENO);
			close(pipe1[0]);
			close(pipe1[1]);
			close(pipe2[0]);
			close(pipe2[1]);
			close(file_dest);
			execlp("./dirlist", "dirlist", argv[2],NULL);
			return (0);
		}
		
		
		p3=fork();
		
		if(p3<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			fprintf(stderr,"Fork for p3 didn't work.\n");
			exit(3);
		}
		else if (p3==0) {
			dup2(pipe1[0], STDIN_FILENO);
			dup2(pipe2[1], STDOUT_FILENO);
			close(pipe1[0]);
			close(pipe1[1]);
			close(pipe2[0]);
			close(pipe2[1]);
			close(file_dest);
			execlp("./p2archive","p2archive",NULL);
			return (0);
		}
		
		p4=fork();
		
		if(p4<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			fprintf(stderr,"Fork for p4 didn't work.\n");
			exit(3);
		}
		else if (p4==0) {
			dup2(pipe2[0], STDIN_FILENO);
			dup2(file_dest,STDOUT_FILENO);
			close(pipe1[0]);
			close(pipe1[1]);
			close(pipe2[0]);
			close(pipe2[1]);
			close(file_dest);
			execlp("./p2crypt","p2crypt",argv[3],NULL);
			return (0);
		}
		close(pipe1[0]);
		close(pipe1[1]);
		close(pipe2[0]);
		close(pipe2[1]);
		close(file_dest);
		waitpid(-1, NULL, 0);
		waitpid(-1, NULL, 0);
		waitpid(-1, NULL, 0);
	}
	else if (strcmp(argv[1],"-D")==0) {
		ErrorFinder(pipe(pipe3),__LINE__);
		
		file_dest=open(argv[4],O_RDONLY);
		if (file_dest<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			perror("open");
			exit(1);
		}
		else {
			n=read(file_dest,checktype,9);
			checktype[n]='\0';
			if (strcmp(checktype,"P2CRYPTAR")!=0) {
				fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
				fprintf(stderr,"NOT A P2AR FILE.\n");
				exit(1);
			}
		}
		
		p2=fork();
		
		if(p2<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			fprintf(stderr,"Fork for p2 didn't work.\n");
			exit(3);
		}
		else if (p2==0) {
			dup2(file_dest, STDIN_FILENO);
			dup2(pipe3[1],STDOUT_FILENO);
			close(pipe3[0]);
			close(pipe3[1]);
			close(file_dest);
			execlp("./p2crypt","p2crypt",argv[3],NULL);
			return (0);
		}
		
		p3=fork();
		
		if(p3<0) {
			fprintf(stderr, "ERROR in HW2 on line %d--> ",__LINE__);
			fprintf(stderr,"Fork for p3 didn't work.\n");
			exit(3);
		}
		else if (p3==0) {
			dup2(pipe3[0], STDIN_FILENO);
			close(pipe3[0]);
			close(pipe3[1]);
			close(file_dest);
			execlp("./p2unarchive","p2unarchive", argv[2],NULL);
			return (0);
		}
		close(pipe3[0]);
		close(pipe3[1]);
		close(file_dest);
		waitpid(-1, NULL, 0);
		waitpid(-1, NULL, 0);
	}
	else {
		fprintf(stderr,"Wrong option: please select \"-E\" or \"-D\" \n"); 
	}
	
	return 0;
}
