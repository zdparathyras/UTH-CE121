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

static volatile sig_atomic_t gotsig = -1;

static void handler(int sig) { gotsig=sig; }

int main(int argc, char *argv[]) {
	int counter=0, block=-1, max=0, pid=-1;
	struct sigaction act = { {0} };
	sigset_t s;
	
	if (argc!=5) {
		fprintf(stderr, "Wrong Parameters %d.\n", argc);
		exit(0);
	}
	
	if (argc==5) {
		if (strcmp(argv[1],"-m")!=0) {
			fprintf(stderr, "Wrong second parameter.\n");
			exit(0);
		}
		
		if (strcmp(argv[3],"-b")!=0) {
			fprintf(stderr, "Wrong third parameter.\n");
			exit(0);
		}
	}
	
	
	max=atoi(argv[2]);
	block=atoi(argv[4]);
	if (block==1) {
		sigemptyset(&s); sigaddset(&s,SIGUSR1);
		sigprocmask(SIG_BLOCK,&s,NULL);
	}
	
	act.sa_handler = handler;
	sigaction(SIGUSR1,&act,NULL);
	
	while (counter<max) {
		sleep(5);
		counter++;
		pid=getpid();
		printf("pid: %d, counter: %d, max: %d\n",pid, counter, max);
		
		if (counter<max/2 && block==1) {
			sigprocmask(SIG_UNBLOCK,&s,NULL);
		}
		if (gotsig != -1) {
			counter=0;
			gotsig=-1;
		}
		
	}
	
	return 0;
}
