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
#include <stddef.h>
#include <sys/sem.h>

typedef struct data {
	char company[3];
	char departure[4];
	char arrival[4];
	int stops;
	int seats;
	int changes; //ΑΥΤΟΣ ΕΙΝΑΙ ΕΝΑΣ ΑΚΕΡΑΙΟΣ ΠΟΥ ΧΡΗΣΙΜΟΠΟΙΕΤΑΙ ΩΣ ΕΝΔΕΙΞΗ ΑΝ ΕΧΕΙ ΓΙΝΕΙ ΑΛΛΑΓΗ ΣΤΑ ΔΕΔΟΜΕΝΑ ΠΑΙΡΝΏΝΤΑΣ ΤΙΜΕΣ ΜΟΝΟ 0 ΚΑΙ 1 (1==ΝΑΙ ΚΑΙ 0==ΟΧΙ)
	int line;   //ΚΙ ΕΔΩ ΑΠΟΘΗΚΕΥΕΤΑΙ ΣΕ ΠΟΙΑ ΣΕΙΡΑ ΤΩΝ ΔΕΔΟΜΕΝΩΝ ΕΓΙΝΕ Η ΑΛΛΑΓΗ
} data_t;

typedef struct agent {
	int pid;
	int tickets;
	int fd;
	int order;      //ΕΔΩ ΑΠΟΘΗΚΕΥΕΤΑΙ Η ΣΕΙΡΑ ΜΕ ΤΗΝ ΟΠΟΙΑ ΣΥΝΔΕΘΗΚΕ Ο AGENT ΣΤΟΝ SERVER
	char pipe[50]; //ΕΔΩ ΑΠΟΘΗΚΕΥΕΤΑΙ ΤΟ ΟΝΟΜΑ ΤΟΥ PIPE ΣΤΟ ΟΠΟΙΟ ΣΥΝΔΕΕΤΑΙ Ο ΚΑΘΕ AGENT
} agent_t;

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΜΕΤΡΑΕΙ ΠΟΣΕΣ ΣΕΙΡΕΣ ΔΕΔΟΜΕΝΩΝ ΥΠΑΡΧΟΥΝ ΣΤΟ ΑΡΧΕΙΟ ΕΙΣΟΔΟΥ
int datacounter(char name[]) {
	FILE *fp;
	int counter=0;
	char str[100];
	
	fp = fopen(name,"r");
	if(fp == NULL) {
		perror("Error in opening file");
		return(-1);
	}
	
	while (fgets(str, 100*sizeof(char),fp)!=NULL) {
		if (feof(fp)) {break;}
		counter++;
	}
	
	return counter;
	
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΑΝΤΙΓΡΑΦΕΙ ΤΑ ΔΕΔΟΜΕΝΑ ΑΠΟ ΤΟ ΑΡΧΕΙΟ ΣΤΗΝ ΚΟΙΝΟΧΡΗΣΤΗ ΜΝΗΜΗ
int datawriter (char name[], data_t *data, int counter) {
	FILE *fp;
	int i;
	
	fp = fopen(name,"r");
	if(fp == NULL) {
		perror("Error in opening file");
		exit(3);
	}
	
	for (i=0; i<counter; i++) {
		fscanf(fp,"%2s %3s %3s %d %d", data[i].company, data[i].departure, data[i].arrival, &data[i].stops, &data[i].seats);
		if (feof(fp)) {break;}
	}
	
	
	return 0;
}

int main(int argc, char *argv[]) {
	//ΜΕΤΑΒΛΗΤΕΣ ΚΟΙΝΟΧΡΗΣΤΗΣ ΜΝΗΜΗΣ
	int shm_file, counter, shmid;
	data_t *data;
	char space=' ', c2, temp[5];
	key_t key, key2;
	int semid, tickets;
	
	//ΜΕΤΑΒΛΗΤΕΣ ΤΩΝ PIPES
	int i, j, accept_fd, now_working, num2, max_agents, new_fd, activity, max_fd, check, read_check, last;
	char acception_fifo[50], new_fifo[50], option;
	agent_t *agents;
	fd_set read_set, fd_set;
	
	if (argc!=4) {
		fprintf(stderr, "Wrong Parameters.\n");
		exit(0);
	}
	
	
	max_agents=atoi(argv[1]);
	agents=(struct agent*)malloc(max_agents*sizeof(struct agent));
	if (agents==NULL) {
		perror("malloc");
		exit(1);
	}
	
	//ΑΡΧΙΚΟΠΟΙΗΣΗ ΤΟΥ STRUCT ΤΩΝ AGENTS
	for (i=0; i<max_agents; i++) {
		agents[i].fd=-1;
		agents[i].tickets=0;
		agents[i].pid=-1;
		agents[i].order=-1;
		agents[i].pipe[0]='\0';
	}
	
	counter=datacounter(argv[2]);
	shm_file=open(argv[2],O_RDWR);
	
	if (shm_file<0) {
		perror("open");
		exit(2);
	}
	
	key = ftok(".",'r');
	if ((shmid=shmget(key, counter*(sizeof(data_t)),IPC_CREAT|0777))<0) {
		perror("shmget");
		exit(1);
	}
	
	data=shmat(shmid, NULL, 0);
	if (data==(data_t*)(-1)) {
		perror("shmat");
		exit(-1);
	}
	
	//ΑΡΧΙΚΟΠΟΙΗΣΗ ΚΟΙΝΟΧΡΗΣΤΗΣ ΜΝΗΜΗΣ
	for (i=0; i<counter; i++) {
		data[i].company[0]='\0';
		data[i].departure[0]='\0';
		data[i].arrival[0]='\0';
		data[i].stops=0;
		data[i].seats=0;
		data[i].changes=0;
		data[i].line=-1;
	}
	
	///ΑΠΟΘΗΚΕΥΣΗ ΤΩΝ ΔΕΔΟΜΕΝΩΝ ΤΟΥ ΑΡΧΕΙΟΥ ΣΤΗΝ ΚΟΙΝΟΧΡΗΣΤΗ ΜΝΗΜΗ
	datawriter(argv[2], data, counter);
	
	//ΔΗΜΙΟΥΡΓΙΑ ΣΗΜΑΤΟΦΟΡΟΥ
	key2=ftok(".",'a');
	semid = semget(key2,1,S_IRWXU);
	semctl(semid,0,SETVAL,1); // init to 
	
	//PIPES
	now_working=0;
	FD_ZERO(&fd_set);
	FD_SET(0, &fd_set);
	max_fd=0;
	
	strcpy(acception_fifo,argv[3]);
	mkfifo(acception_fifo,0666);
	accept_fd=open(acception_fifo,O_RDONLY);
	if (accept_fd>0) {
		FD_SET(accept_fd, &fd_set);
		max_fd=accept_fd;
	}
	else {
		perror("accept_fd open()");
		exit(1);
	}
	
	while (1) {
		read_set=fd_set;
		activity = select( max_fd + 1 , &read_set , NULL , NULL , NULL);
		if (activity < 0){
			perror("select");
			exit(2);
		}
		
		if (FD_ISSET(0, &read_set)) { 
			read_check=read(0, &option, sizeof(char));
			if (read_check>0) {
				if (option=='Q') {break;}
			}
			else if (read_check==0) {
				break;
			}
			else if (read_check<0) {
				perror("read");
				exit(3);
			}
			
		}
		else {
			if (FD_ISSET(accept_fd, &read_set)) {
				read_check=read(accept_fd, &num2, sizeof(int));
				if (read_check>0) {
					if (now_working<0) {
						now_working=0;
					}
					agents[now_working].pid=num2;
					sprintf(new_fifo, "%dpipe", num2);
					check=mkfifo(new_fifo, 0666);
					if (check<0 && errno!=EEXIST) {
						perror("new_fifo mkfifo()");
						exit(1);
					}
						
					new_fd=open(new_fifo, O_WRONLY);
					if (now_working<max_agents) {
						if (new_fd>0) {
							write(new_fd, &key, sizeof(key_t));
							write(new_fd, &counter, sizeof(int));
							close(new_fd);
							new_fd=open(new_fifo, O_RDONLY);
							FD_SET(new_fd , &fd_set);
							if(new_fd> max_fd) {
								max_fd =new_fd; 
							}
							strcpy(agents[now_working].pipe, new_fifo);
							agents[now_working].fd=new_fd;
							agents[now_working].order=now_working;
							now_working++;
						}
						else if (new_fd<0){
							perror("open");
							exit(2);
						}
					}
					else {
						close(new_fd);
						unlink(new_fifo);
					}
				}
				else if (read_check<0) {
					perror("read");
					break;
				}
				else if (read_check==0) {
					//ΑΝ ΥΠΑΡΧΕΙ ΜΟΝΟ ΕΝΑΣ AGENT ΣΥΝΔΕΜΕΝΟΣ ΚΑΙ ΒΓΕΙ
					for (j=0; j<now_working; j++) {
						if (agents[j].fd==new_fd) {
							FD_CLR(new_fd, &fd_set);
							close(new_fd);
							unlink(agents[j].pipe);
							agents[j].fd=-1;
							agents[j].tickets=0;
							agents[j].pid=-1;
							agents[j].order=-1;
							agents[j].pipe[0]='\0';
						}
					}
					now_working--;
					if (now_working<=0) {
						max_fd=accept_fd;
					}
					else {
						for (j=0; j<now_working; j++) {
							if (agents[j].fd>0) {
								if (agents[j].fd>max_fd) {
									max_fd=agents[j].fd;
								}
							}
						}
					}
				}
			}
			else {
				for (i=0; i<now_working; i++) {
					if (FD_ISSET(agents[i].fd, &read_set)) {
						read_check=read(agents[i].fd, &tickets, sizeof(int));
						if (read_check>0) {
							agents[i].tickets=agents[i].tickets + tickets;
						}
						else if (read_check==0) {
							//ΑΝ ΕΝΑΣ AGENT ΒΓΕΙ ΤΟΝ ΚΛΕΙΝΩ ΚΑΙ ΑΝ ΔΕΝ ΕΙΝΑΙ Ο ΤΕΛΕΥΤΑΙΟΣ ΒΑΖΩ ΤΟΝ ΤΕΛΕΥΤΑΙΟ ΣΤΗ ΘΕΣΗ ΤΟΥ
							
							//ΒΡΙΣΚΩ ΠΟΙΟΣ ΕΙΝΑΙ ΤΕΛΕΥΤΑΙΟΣ
							last=0;
							for (j=0; j<now_working; j++) {
								if (agents[j].order>last) {
									last=agents[j].order;
								}
							}
							if (last==agents[i].order) {
								//ΑΝ ΑΥΤΟΣ ΠΟΥ ΒΓΗΚΕ ΕΙΝΑΙ Ο ΤΕΛΕΥΤΑΙΟΣ
								FD_CLR(agents[i].fd, &fd_set);
								close(agents[i].fd);
								unlink(agents[i].pipe);
								agents[i].fd=-1;
								agents[i].tickets=0;
								agents[i].pid=-1;
								agents[i].order=-1;
								agents[i].pipe[0]='\0';
							}
							else {
								//ΑΝ ΑΥΤΟΣ ΠΟΥ ΒΓΗΚΕ ΔΕΝ ΕΙΝΑΙ Ο ΤΕΛΕΥΤΑΙΟΣ
								FD_CLR(agents[i].fd, &fd_set);
								close(agents[i].fd);
								unlink(agents[i].pipe);
								agents[i].fd=agents[last].fd;
								agents[i].pid=agents[last].pid;
								agents[i].tickets=agents[last].tickets;
								agents[last].order=agents[i].order;
								strcpy(agents[i].pipe, agents[last].pipe);
							}
							//ΑΛΛΑΖΩ ΤΟΝ max_fd
							max_fd=agents[0].fd;
							for (j=0; j<now_working; j++) {
								if (agents[j].fd>max_fd) {
									max_fd=agents[j].fd;
								}
							}
							
							now_working--;
						}
					}
				}
			}
		}
	}
	
	
	//Εδώ για όλους τους ενεργούς agents εκτυπώνω πόσα εισητήρια έκλεισε ο καθένας και πόσα όλοι μαζί συνολικά.
	tickets=0;
	for (i=0; i<now_working; i++) {
		if (agents[i].fd>0) {
			tickets=tickets + agents[i].tickets;
			printf("Agent %d booked %d tickets.\n", agents[i].pid, agents[i].tickets);
			close(agents[i].fd);
			agents[i].fd=-1;
		}
	}
	printf("Total tickets for all active agents: %d\n",tickets);
	
	//Εδώ καταγράφω τις αλλαγές απο την κοινόχρηστη μνήμη πίσω στο αρχείο και όπου περισεύει χώρος βάζω κενά (π.χ. αν απο τριψήφιος έγινε διψήφιος κ.λ.π).
	for (i=0; i<counter; i++) {
		if (data[i].changes>0) {
			temp[0]='\0';
			lseek(shm_file, 0, SEEK_SET);
			for (j=0; j<data[i].line; j++) {
				lseek(shm_file, 14, SEEK_CUR);
				do {
				read(shm_file, &c2, sizeof(char));
				}while (c2!='\n');
			}
			sprintf(temp, "%d", data[i].seats);
			lseek(shm_file, 13, SEEK_CUR);
			write(shm_file, temp, strlen(temp));
			do{
				read(shm_file, &c2, sizeof(char));
				if (c2!='\n') {
					lseek(shm_file, -1, SEEK_CUR);
					write(shm_file, &space , sizeof(char));
				}
			}while(c2!='\n');
		}
	}
	
	//Εδώ κλείνω όλους τους ενεργούς agents και τους αφαιρώ απο το set.
	for(i=0; i<max_agents; i++) {
		if (agents[i].fd>0) {
			agents[i].fd=-1;
			FD_CLR(agents[i].fd, &fd_set);
			close(agents[i].fd);
			unlink(agents[i].pipe);
		}
	}
	
	//ΑΠΟΔΕΥΣΜΕΥΣΗ ΔΥΝΑΜΙΚΗΣ ΜΝΗΜΗΣ (STRUCT AGENT)
	free(agents);
	
	close(accept_fd);
	close(shm_file);
	shmdt(data);
	unlink(argv[3]);
	semctl(semid,0,IPC_RMID);
	shmctl(shmid,0,IPC_RMID);
	return(0);
}
