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
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>

typedef struct data {
	char company[3];
	char departure[4];
	char arrival[4];
	int stops;
	int seats;
} data_t;

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΜΕΤΡΑΕΙ ΠΟΣΕΣ ΣΕΙΡΕΣ ΔΕΔΟΜΕΝΩΝ ΥΠΑΡΧΟΥΝ ΣΤΟ ΑΡΧΕΙΟ ΕΙΣΟΔΟΥ
int datacounter (char name[]) {
	FILE *fp;
	int counter=0;
	char company[3], departure[4], arrival[4];
	int stops, seats;
	
	fp = fopen(name,"r");
	if(fp == NULL) {
		perror("Error in opening file");
		return(-1);
	}
	
	while(1) {
		fscanf(fp, "%2s %3s %3s %d %d", company, departure, arrival, &stops, &seats);
		if (feof(fp)) {break;}
		counter++;
	}
	
	return counter;
}

int datawriter (char name[], data_t *data, int counter) {
	FILE *fp;
	int i;
	
	fp = fopen(name,"r");
	if(fp == NULL) {
		perror("Error in opening file");
		return(-1);
	}
	
	for (i=0; i<counter; i++) {
		fscanf(fp, "%2s %3s %3s %d %d", data[i].company, data[i].departure, data[i].arrival, &data[i].stops, &data[i].seats);
		if (feof(fp)) {break;}
	}
	
	return 0;
}

int main(int argc, char *argv[]) {
	//ΜΕΤΑΒΛΗΤΕΣ ΚΟΙΝΟΧΡΗΣΤΗΣ ΜΝΗΜΗΣ
	int shm_file, counter, shmid;
	data_t *data;
	key_t key=1234;
	
	//ΜΕΤΑΒΛΗΤΕΣ ΤΩΝ SOCKETS
	int max_agents=atoi(argv[1]), i, master_socket, addrlen, client_len, client_sock, sock2, check;
	int now_working, acception_fd, num2, pids[max_agents], opt = 1;
	char option, new_socket[20];
	struct sockaddr_un addr, client_addr, cl2;
	struct pollfd fds[max_agents+2];
	
	if (argc!=3) {
		fprintf(stderr, "Wrong Parameters.\n");
		exit(0);
	}
	
	for (i=0; i<max_agents; i++) {
		pids[i]=-1;
	}
	
	
	shm_file=open(argv[2],O_RDWR);
	counter=datacounter(argv[2]);
	fprintf(stderr,"lines of data: %d\n", counter);
	
	if (shm_file<0) {
		perror("open");
		exit(2);
	}
	
	if ((shmid=shmget(key, counter*(sizeof(data_t)),IPC_CREAT|0777))<0) {
		perror("shmget");
		exit(1);
	}
	
	data=shmat(shmid, NULL, 0);
	if (data==(data_t*)(-1)) {
		perror("shmat");
		exit(-1);
	}
	
	for (i=0; i<counter; i++) {
		data[i].company[0]='\0';
		data[i].departure[0]='\0';
		data[i].arrival[0]='\0';
		data[i].stops=0;
		data[i].seats=0;
	}
	
	///ΑΠΟΘΗΚΕΥΣΗ ΤΩΝ ΔΕΔΟΜΕΝΩΝ ΤΟΥ ΑΡΧΕΙΟΥ ΣΤΗΝ ΚΟΙΝΟΧΡΗΣΤΗ ΜΝΗΜΗ
	datawriter(argv[2], data, counter);
	
	///SOCKETS
	master_socket=socket(AF_UNIX,SOCK_STREAM,0);
	addr.sun_family=AF_UNIX; 
	strcpy(addr.sun_path,"myserver");
	addrlen=sizeof(addr);
	
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0 ){
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	if ((bind(master_socket,(struct sockaddr *)&addr,sizeof(addr)))<0) {
		perror("bind");
		exit(2);
	}
	if (listen(master_socket,max_agents)<0) {
		perror("listen");
		exit(2);
	}
	
	fds[0].fd=STDIN_FILENO;
	fds[0].events=POLLIN;
	fds[1].fd=master_socket;
	fds[1].events=POLLIN;
	now_working=2;
	
	int nread=0;
	
	fprintf(stderr, "Waiting for client (%d total)...\n", max_agents);
	while(1) {
		nread=poll(fds, now_working, 10000);
		if (nread==0) continue;
		if (nread<0) {
			perror("poll");
			break;
		}
		if (fds[0].revents & POLLIN) {
			fprintf(stderr,"\nServing STDIN----------\n");
			scanf(" %c", &option);
			if (option=='Q') {
				break;
			}
			else {
				fprintf(stderr, "WRONG OPTION\n");
			}
		}
		else if (fds[1].revents & POLLIN) {
			fprintf(stderr,"\nServing ACCEPTION----------\n");
			
			acception_fd=accept(master_socket, (struct sockaddr *)&addr, (socklen_t*)&addrlen);
			
			read(acception_fd, &num2, sizeof(int));
			close(acception_fd);
			//ACCEPTION
			if (now_working<max_agents+2) {
				pids[now_working-2]=num2;
				new_socket[0]='\0';
				sprintf(new_socket,"%d", num2);
				fprintf(stderr, "new socket path: %s\n", new_socket);
				
				//NEW AGENT 
				fprintf(stderr,"Adding client on pos %d\n", now_working);
				client_sock=socket(AF_UNIX,SOCK_STREAM,0);
				client_addr.sun_family=AF_UNIX; 
				strcpy(client_addr.sun_path,new_socket);
				client_len=sizeof(client_addr);
				
				check=connect(client_sock,(struct sockaddr *)&client_addr,sizeof(client_addr));
				if (check==0) {
					write(client_sock, &shmid, sizeof(int));
					write(client_sock, &counter, sizeof(int));
					printf("sent: %d and %d\n", shmid, counter);
					close(client_sock);
					unlink(new_socket);
				}
				else {
					perror("connect");
					exit(2);
				}
				
				//PHASE 2
				sprintf(new_socket, "s");
				
				sock2=socket(AF_UNIX,SOCK_STREAM,0);
				cl2.sun_family=AF_UNIX; 
				strcpy(cl2.sun_path,new_socket);
				client_len=sizeof(cl2);
				
				do{
					check=bind(sock2,(struct sockaddr *)&cl2,sizeof(cl2));
				}while (check!=0);
				
				if (listen(sock2,1)<0) {
					perror("client listen");
					exit(2);
				}
				
				fds[now_working].fd=accept(sock2, (struct sockaddr *)&cl2, (socklen_t*)&client_len);
				
				if (fds[now_working].fd>0) {
					fds[now_working].events=POLLIN;
					now_working++;
				}
				else {
					perror("2-accept");
					break;
				}
			}
			else if (now_working>=max_agents+2){
				fprintf(stderr, "No more agents can connect.\n");
				break;
			}
		}
		else { 
			for(i=2; i<now_working; i++) {
				
				if (fds[i].revents & POLLIN) {
					nread=read(fds[i].fd, &num2, sizeof(int));
					if (nread==0) {
						close(fds[i].fd);
						fds[i].events=0;
						fprintf(stderr, "Removed client on fd %d\n", fds[i].fd);
						fds[i]=fds[now_working-1];
						now_working--;
						i--;
					}
					else {
						fprintf(stderr, "Read from client %d\n", num2);
					}
				}
			}
		}
	}
	
	for (i=0; i<max_agents; i++) {
		fprintf(stderr, "%d\n", pids[i]);
	}
	
	for (i=0; i<max_agents+2; i++) {
		fprintf(stderr, "%d\n", fds[i].fd);
	}
	
	close(master_socket);
	unlink("myserver");
	shmdt(data);
	shmctl(shmid,0,IPC_RMID);
	return(0);
}
