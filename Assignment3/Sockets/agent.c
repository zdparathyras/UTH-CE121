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

int main(int argc, char *argv[]) {
	int master_socket, s2, s3, agent_pid, shmid, counter, nread, nread2, fd, input_len, temp=456;
	char pid_str[20];
	struct sockaddr_un output, input, in2;
	
	if (argc!=2) {
		fprintf(stderr, "Wrong Parameters.\n");
		exit(0);
	}
	agent_pid=getpid();
	pid_str[0]='\0';
	sprintf(pid_str,"%d", agent_pid);
	fprintf(stderr,"pid socket : %s\n", pid_str);
	
	memset(&input, 0, sizeof(struct sockaddr_un));
	memset(&output, 0, sizeof(struct sockaddr_un));
	
	//SOCKET FOR GETTING ACCEPTED BY SERVER
	master_socket=socket(AF_UNIX,SOCK_STREAM,0);
	output.sun_family=AF_UNIX; 
	strcpy(output.sun_path, argv[1]);
	
	connect(master_socket,(struct sockaddr *)&output,sizeof(output));
	
	write(master_socket, &agent_pid, sizeof(int));
	close(master_socket);
	
	//SOCKET FOR GETTING SHM DATA
	s2=socket(AF_UNIX,SOCK_STREAM,0);
	input.sun_family=AF_UNIX; 
	strcpy(input.sun_path, pid_str);
	input_len=sizeof(input);
	
	if ((bind(s2,(struct sockaddr *)&input,sizeof(input)))<0) {
		perror("input bind");
		exit(2);
	}
	if (listen(s2,1)<0) {
		perror("input listen");
		exit(2);
	}
	
	fd=accept(s2, (struct sockaddr *)&input, (socklen_t*)&input_len);
	if (fd>=0) {
		nread=read(fd, &shmid, sizeof(int));
		nread2=read(fd, &counter, sizeof(int));
		close(fd);
	}
	else {
		perror("accept");
		exit(2);
	}
	
	if(nread>0 && nread2>0) {
		fprintf(stderr,"shmid is: %d\n", shmid);
		fprintf(stderr,"counter is: %d\n", counter);
		unlink(pid_str);
		close(s2);
		
		sprintf(pid_str,"s"); 
		//SOCKET FOR SENDING RESERVATIONS TO SERVER
		s3=socket(AF_UNIX,SOCK_STREAM,0);
		in2.sun_family=AF_UNIX;
		strcpy(in2.sun_path, pid_str);
		input_len=sizeof(in2);
		
		do {
			fd=connect(s3,(struct sockaddr *)&in2,sizeof(input_len));
		}while (fd!=0);
		
		write(s3, &temp, sizeof(int));
		printf("agent send: %d\n", temp);
		close(s3);
	}
	else if (nread<=0 || nread2<=0){
		close(s2);
	}
	
	unlink(pid_str);
	return(0);
}
