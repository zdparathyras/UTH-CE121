#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stddef.h>
#include <sys/sem.h>

typedef struct data {
	char company[3];
	char departure[4];
	char arrival[4];
	int stops;
	int seats;
	int changes;
	int line;
} data_t;

void findforme(char SRC[],char  DEST[], int NUM, data_t *data, int counter) {
	int i;
	
	printf("I FOUND THESE FLIGHTS THAT MIGHT SUIT YOU.\n");
	for (i=0; i<counter; i++) {
		if (strcmp(data[i].departure, SRC)==0) {
			if(strcmp(data[i].arrival, DEST)==0) {
				if (NUM<=data[i].seats) {
					printf("%s %s %s %d %d\n", data[i].company, data[i].departure, data[i].arrival, data[i].stops, data[i].seats);
				}
			}
		}
	}
}

int reserveforme(char SRC[],char  DEST[], char AIR[], int NUM, data_t *data, int counter) {
	int i, error=-1, res=0;
	
	for (i=0; i<counter; i++) {
		if (strcmp(data[i].company, AIR)==0) {
			if (strcmp(data[i].departure, SRC)==0) {
				if(strcmp(data[i].arrival, DEST)==0) {
					if (NUM<=data[i].seats) {
						printf("%s %s %s %d %d\n", data[i].company, data[i].departure, data[i].arrival, data[i].stops, data[i].seats);
						data[i].seats=data[i].seats -NUM;
						data[i].changes=1;
						data[i].line=i;
						res=NUM;
						break;
					}
					else {
						error=0;
					}
				}
			}
		}
	}
	
	if (error==0) {
		fprintf(stderr,"THIS FLIGHT CANT BE BOOKED NOT ENOUGH TICKETS.\n");
	}
	
	if (error==-1 && res==0) {
		fprintf(stderr, "FLIGHT NOT FOUND.\n");
	}
	
	return res;
}

int main(int argc, char *argv[]) {
	int agent_pid, shmid, counter, fd, data_fd, nread, nread2, semid, tickets=-1;
	char pid_str[50], acception_fifo[50], data_fifo[50], option[10];
	data_t *data;
	key_t key,key2;
	struct sembuf op;
	
	//ΜΕΤΑΒΛΗΤΕΣ ΓΙΑ ΤΑ ΑΙΤΗΜΑΤΑ
	char SRC[4], DEST[4], AIR[3];
	int NUM;
	
	if (argc!=2) {
		fprintf(stderr, "Wrong Parameters.\n");
		exit(0);
	}
	
	agent_pid=getpid();
	sprintf(pid_str, "%dpipe", agent_pid);
	
	strcpy(acception_fifo,argv[1]);
	fd=open(acception_fifo, O_WRONLY);
	if (fd>0) {
		write(fd, &agent_pid, sizeof(int));
	}
	else {
		perror("acception_fifo open()");
		exit(0);
	}
	
	strcpy(data_fifo, pid_str);
	do {
		data_fd=open(data_fifo, O_RDONLY);
	} while (data_fd<0);
	
	nread=read(data_fd, &key, sizeof(key_t));
	nread2=read(data_fd, &counter, sizeof(int));
	
	if (nread>0 && nread2>0) {
		
		//ΣΥΝΔΕΣΗ ΚΟΙΝΟΧΡΗΣΤΗΣ ΜΝΗΜΗΣ
		if ((shmid=shmget(key,0,0))<0) {
			perror("shmget");
			exit(1);
		}
		
		data=shmat(shmid, NULL, 0);
		if (data==(data_t*)(-1)) {
			perror("shmat");
			exit(-1);
		}
		
		//ΔΗΜΙΟΥΡΓΙΑ ΣΗΜΑΤΟΦΟΡΟΥ
		key2=ftok(".",'a');
		semid = semget(key2,0,0);
		
		close(data_fd);
		data_fd=open(data_fifo, O_WRONLY);
		if (data_fd<0) {
			perror("open");
			exit(1);
		}
		do {
			printf("WHAT IS YOUR REQUEST:\n");
			scanf("%9s", option);
			if (strcmp(option, "FIND")==0){
				scanf("%3s", SRC);
				scanf("%3s", DEST);
				scanf("%d", &NUM);
				
				op.sem_num = 0; op.sem_op = -1; op.sem_flg = 0;
				semop(semid,&op,1);
				findforme(SRC, DEST, NUM, data, counter);
				op.sem_num = 0; op.sem_op = 1; op.sem_flg = 0;
				semop(semid,&op,1);
			}
			else if (strcmp(option, "RESERVE")==0) {
				scanf("%3s", SRC);
				scanf("%3s", DEST);
				scanf("%2s", AIR);
				scanf("%d", &NUM);
				
				op.sem_num = 0; op.sem_op = -1; op.sem_flg = 0;
				semop(semid,&op,1);
				tickets=reserveforme(SRC, DEST, AIR, NUM, data,counter);
				if (tickets>0) {
					write(data_fd, &tickets, sizeof(int));
				}
				op.sem_num = 0; op.sem_op = 1; op.sem_flg = 0;
				semop(semid,&op,1);
			}
			
		}while (strcmp(option, "EXIT")!=0);
		
		
	}
	
	shmdt(data);
	close(data_fd);
	close(fd);
	unlink(data_fifo);
	semctl(semid,0,IPC_RMID);
	
	return(0);
}
