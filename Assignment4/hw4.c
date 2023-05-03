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

typedef struct proccess{
	int pid;
	char name[100];
	char **args;
	int args_num;
	char status;
	struct proccess *next;
}proccess_t;


//GLOBAL VARIABLES
proccess_t *CURRENT;
proccess_t *HEAD;

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΑΝΑΔΙΟΡΓΑΝΩΝΕΙ ΤΟ ΠΟΙΑ ΔΙΕΡΓΑΣΙΑ ΤΡΕΧΕΙ ΣΤΕΛΝΩΝΤΑΣ SIGSTOP ΚΑΙ SIGCONT ΑΝΤΙΣΤΟΙΧΑ ΚΑΙ ΑΡΧΙΚΟΠΟΙΕΙ ΞΑΝΑ ΤΟ ΞΥΠΝΗΤΗΡΙ
void List_Scheduler () {
	struct itimerval t = { {0} };
	
	if (CURRENT!=NULL) {
		if (HEAD->next==HEAD) {
			return;
		} 
		else {
			CURRENT->status='S';
			kill(CURRENT->pid, SIGSTOP);
			
			/* switch to the next waiting proccess */
			CURRENT=CURRENT->next;
			CURRENT->status='R';
			kill(CURRENT->pid, SIGCONT);
			
			/* Reseting the timer*/
			t.it_value.tv_sec = 20;
			t.it_value.tv_usec = 0;
			t.it_interval.tv_sec =20;
			t.it_interval.tv_usec=0;
			setitimer(ITIMER_REAL,&t,NULL);
		}
	}
	else {
		if (HEAD!=NULL) {
			/* only one process is on list make it running process */
			CURRENT=HEAD;
			CURRENT->status='R';
			kill(CURRENT->pid, SIGCONT);
			
			/* Reseting the timer*/
			t.it_value.tv_sec = 20;
			t.it_value.tv_usec = 0;
			t.it_interval.tv_sec =20;
			t.it_interval.tv_usec=0;
			setitimer(ITIMER_REAL,&t,NULL);
		}
	}
	
}

static void handler(int sig) {
	write(STDOUT_FILENO,"BEEP!\n",6);
	List_Scheduler();
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΠΡΟΣΘΕΤΕΙ ΝΕΟ ΚΟΜΒΟ ΣΤΗΝ ΛΙΣΤΑ (ΣΤΗΝ ΟΥΡΑ ΤΗΣ ΑΝ ΑΥΤΗ ΔΕΝ ΕΙΝΑΙ ΑΔΕΙΑ)
int Insert_New(int proc_pid, char proc_name[], char **arguments, int arguments_num) {
	int i;
	proccess_t *new=NULL, *last=NULL;
	
	new=(proccess_t*)malloc(sizeof(proccess_t));
	if (new==NULL) {
		perror("malloc");
		return 0;
	}
	
	if (HEAD==NULL) {
		HEAD=new;
		new->next=HEAD;
	}
	else {
		for (last=HEAD; last->next!=HEAD; last=last->next);
		last=last->next;
		if (last!=NULL) {
			new->next=HEAD;
			last->next=new;
			last=new;
		}
	}
	
	new->pid=proc_pid;
	strcpy(new->name, proc_name);
	
	if (arguments!=NULL) {
		new->args=(char **)malloc(arguments_num*sizeof(char*));
		if (new->args==NULL) {
			perror("malloc");
			return 0;
		}
		for (i=0; i<arguments_num; i++) {
			new->args[i]=(char *)malloc(30*sizeof(char));
			if (new->args[i]==NULL) {
				perror("malloc");
				return 0;
			}
			if (arguments[i]!=NULL) {
				strcpy(new->args[i], arguments[i]);
			}
			else {
				new->args[i]=NULL;
			}
		}
	}
	else {
		new->args=NULL;
	}
	new->args_num=arguments_num;
	new->status='S';
	
	return 1;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΒΡΙΣΚΕΙ ΚΑΙ ΕΠΙΣΤΡΕΦΕΙ ΤΟ ΠΡΟΗΓΟΥΜΕΝΟ ΚΟΜΒΟ ΑΠΟ ΑΥΤΟΝ ΠΟΥ ΔΩΣΑΜΕ ΩΣ ΟΡΙΣΜΑ
proccess_t *Find_Prev(proccess_t *temp) {
	proccess_t *prev=NULL;
	
	for (prev=temp; prev->next!=temp; prev=prev->next) {
	}
	
	return prev;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΒΡΙΣΚΕΙ ΚΑΙ ΔΙΑΓΡΑΦΕΙ ΤΟ ΖΗΤΟΥΜΕΝΟ ΚΟΜΒΟ ΜΕ ΣΤΟΙΧΕΙΟ ΑΝΑΓΝΩΡΙΣΗΣ ΤΟΥ ΤΟ PID
int Delete(int wan_pid) {
	int i;
	proccess_t *temp, *epomenos=NULL, *proigoumenos=NULL;
	
	if (HEAD!=NULL) {
		if (CURRENT->pid==wan_pid) {
			CURRENT=NULL;
		}
		
		temp=HEAD;
		do {
			if (temp->pid==wan_pid) {
				epomenos=temp->next;
				proigoumenos=Find_Prev(temp);
				break;
			}
			temp=temp->next;
		}while (temp!=HEAD);
		
		if (temp!=HEAD) {
			proigoumenos->next=epomenos;
			for (i=0; i<temp->args_num; i++) {
				free (temp->args[i]);
			}
			free (temp->args);
			free(temp);
		}
		else {
			proigoumenos->next=epomenos;
			HEAD=epomenos;
			for (i=0; i<temp->args_num; i++) {
				free (temp->args[i]);
			}
			free (temp->args);
			free(temp);
		}
		
		if (CURRENT==NULL) {HEAD=NULL;}
		
	}
	return 0;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΒΡΙΣΚΕΙ ΚΑΙ ΕΠΙΣΤΡΕΦΕΙ ΤΟ ΖΗΤΟΥΜΕΝΟ ΚΟΜΒΟ ΜΕ ΣΤΟΙΧΕΙΟ ΑΝΑΓΝΩΡΙΣΗΣ ΤΟΥ ΤΟ PID
proccess_t *Find_PID(int pid) {
	proccess_t *wanted=NULL;
	
	wanted=HEAD;
	do {
		if (wanted->pid==pid) {
			break;
		}
		wanted=wanted->next;
	}while (wanted!=HEAD);
	
	
	return wanted;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΕΛΕΓΧΕΙ ΑΝ Η ΛΙΣΤΑ ΕΙΝΑΙ ΑΔΕΙΑ
int Is_Empty() {
	if (HEAD==NULL) {
		printf("List is empty.\n");
		return 0;
	}
	else {
		return 1;
	}
}

//ΑΤΥΗ Η ΣΥΝΑΡΤΗΣΗ ΣΤΕΛΝΕΙ ΣΕ ΟΛΕΣ ΤΙΣ ΔΙΕΡΓΑΣΙΕΣ (ΚΟΜΒΟΥΣ) ΣΗΜΑ ΘΑΝΑΤΩΣΗΣ
void KILL_ALL () {
	proccess_t *temp;
	
	if (HEAD!=NULL) {
		temp=HEAD;
		do {
			kill(temp->pid, SIGKILL);
			temp=temp->next;
		}while (temp!=HEAD);
	}
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΠΕΡΙΜΕΝΕΙ ΝΑ ΤΕΛΕΙΩΣΕΙ ΜΙΑ ΔΙΕΡΓΑΣΙΑ ΚΑΙ ΜΕΤΑ ΤΗΝ ΔΙΑΓΡΑΦΕΙ ΑΠΟ ΤΗΝ ΛΙΣΤΑ ΚΑΙ ΚΑΛΕΙ ΤΗΝ ΣΥΝΑΡΤΗΣΗ ΠΟΥ ΑΝΑΔΙΟΡΓΑΝΩΝΕΙ ΤΗ ΛΙΣΤΑ
int KILL_TERMINATED() {
	pid_t p1;
	int p_stat;
	int result=0;
	
	p1=waitpid(-1, &p_stat, WNOHANG);
	if (p1>0) {
		if (CURRENT->pid==p1) {
			List_Scheduler();
			result =1;
		}
		Delete(p1);
		List_Scheduler();
	}
	
	return result;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΕΚΤΥΠΩΝΕΙ ΤΑ ΔΕΔΟΜΕΝΑ ΟΛΗΣ ΤΗΣ ΛΙΣΤΑΣ
void Print_List () {
	int i;
	proccess_t *temp=NULL;
	
	if (HEAD!=NULL) {
		temp=HEAD;
		do {
			printf("pid: %d", temp->pid);
			printf(", name: (%s", temp->name);
			if (temp->args!=NULL) {
				printf(" ,");
				for (i=1; i<temp->args_num-2; i++) {
					printf(" %s,", temp->args[i]);
				}
				printf(" %s", temp->args[temp->args_num-2]);
			}
			printf(")");
			if (temp->status=='R') {
				printf(" (R)\n");
			}
			else {
				printf("\n");
			}
			
			temp=temp->next;
		}while (temp!=HEAD);
	}
	else {
		printf("List is empty.\n");
	}
	
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΑΠΕΛΕΥΘΕΡΩΝΕΙ ΟΛΗ ΤΗΝ ΔΥΝΑΜΙΚΗ ΜΝΗΜΗ ΠΟΥ ΧΡΗΣΙΜΟΠΟΙΟΥΣΕ Η ΛΙΣΤΑ ΚΑΙ ΤΗΝ ΜΝΗΜΗ ΠΟΥ ΧΡΗΣΙΜΟΠΟΙΟΥΣΕ ΚΑΘΕ ΚΟΜΒΟΣ
void Free_List () {
	int i;
	proccess_t *temp, *to_free;
	
	if (HEAD!=NULL) {
		temp=HEAD;
		do {
			to_free=temp;
			temp=temp->next;
			if (to_free->args!=NULL) {
				for (i=0; i<to_free->args_num; i++) {
					free(to_free->args[i]);
				}
				free(to_free->args);
			}
			free(to_free);
		}while (temp->next!=HEAD);
		free(temp);
	}
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΔΙΑΒΑΖΕΙ ΑΠΟ ΤΗΝ ΚΑΘΙΕΡΩΜΕΝΗ ΕΙΣΟΔΟ ΤΗΝ ΕΝΤΟΛΗ ΠΟΥ ΔΙΝΕΙ Ο ΧΡΗΣΤΗΣ
int readcommand(char input[]) {
	FILE *file=stdin;
	int i;
	input[0]='\0';
	if (fgets(input, 300*sizeof(char), file)!=NULL) {	
		for (i=0; i<strlen(input); i++) {
			if (input[i]=='\n') {
				input[i]='\0';
				break;
			}
		}
	} else {
		return 0;
	}
	return 1;
}


//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΣΠΑΕΙ ΤΗΝ ΑΡΧΙΚΗ ΕΙΣΟΔΟ ΣΕ ΚΟΜΜΑΤΙΑ (ΕΝΤΟΛΗ, ΟΝΟΜΑ ΠΡΟΓΡΑΜΜΑΤΟΣ ΚΑΙ ΟΡΙΣΜΑΤΑ ΑΝ ΕΧΕΙ)
int devide_command(char input[], char command[], char progname[], char temp[]) {
	int i, j, k, max=0, command_len, progname_len;
	
	sscanf(input, "%9s", command);
	if (strcmp(command, "exec")==0){
		command_len=strlen(command);
		sscanf(input+command_len, "%99s", progname);
		progname_len=strlen(progname);
		j=command_len + progname_len +1;
		
		k=0;
		for (i=j; input[i]!='\0'; i++) {
			temp[k]=input[i];
			k++;
			if (input[i]==' ' || input[i]=='\t') {
				max++;
				do {
					i++;
				} while(input[i]==' ' || input[i]=='\t');
				i--;
			}
		}
		temp[k]='\0';
	}
	else if (strcmp(command, "term")==0){
		command_len=strlen(command);
		sscanf(input+command_len, "%99s", progname);
	}
	else if (strcmp(command, "sig")==0){
		command_len=strlen(command);
		sscanf(input+command_len, "%99s", progname);
	}
	
	return max;
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΑΠΟΗΘΚΕΥΕΙ ΤΑ ΟΡΙΣΜΑΤΑ ΣΤΗΝ ΚΑΤΑΛΛΗΛΗ ΘΕΣΗ ΤΟΥ ΔΥΝΑΜΙΚΟΥ ΠΙΝΑΚΑ ΤΩΝ ΟΡΙΣΜΑΤΩΝ 
void make_args(char temp[], int max, char **args) {
	int i, j;
	char *pointer;
	
	pointer=&temp[0];
	j=0;
	for (i=1; i<max-1; i++) {
		sscanf(pointer, "%9s", args[i]);
		j=j+strlen(args[i]);
		j++;
		pointer=&temp[j];
	}
	
}

//ΑΥΤΗ Η ΣΥΝΑΡΤΗΣΗ ΕΚΤΕΛΕΙ ΤΗΝ ΕΝΤΟΛΗ ΠΟΥ ΕΔΩΣΕ Ο ΧΡΗΣΤΗΣ
int execute_command (char input[]) {
	char command[10], progname[100], temp[40], **args=NULL;
	int i, max=0, pid=0, check=-1, res=1;
	proccess_t *found=NULL;
	pid_t p1=-1;
	i=0;
	
	command[0]='\0';
	progname[0]='\0';
	temp[0]='\0';
	
	//ΕΔΩ ΚΑΛΟΥΜΕ ΤΗΝ ΣΥΝΑΡΤΗΣΗ ΠΟΥ ΔΙΑΣΠΑ ΣΕ ΚΟΜΜΑΤΙΑ ΤΗΝ ΕΙΣΟΔΟ
	max=devide_command(input, command, progname, temp);
	
	if (strcmp(command,"exec")==0) {
			if (max>0){
				max=max+2;
				
				args=(char **)malloc(max*sizeof(char*));
				if (args==NULL) {
					perror("malloc");
					exit(2);
				}
				for (i=0; i<max; i++) {
					args[i]=(char *)malloc(30*sizeof(char));
					if (args[i]==NULL) {
						perror("malloc");
						exit(2);
					}
				}
				
				//SET ARGUMENTS
				strcpy(args[0], progname);
				args[max-1]=NULL;
				//ΚΙ ΕΔΩ ΑΠΟΗΘΚΕΥΟΥΜΕ ΤΑ ΟΡΙΣΜΑΤΑ ΣΤΙΣ ΚΑΤΑΛΛΗΛΕΣ ΘΕΣΕΙΣ
				make_args(temp, max, args);
				
			}
			else {
				max=0;
				args=NULL;
			}
			
			p1=fork();
			if(p1<0) {
				perror("fork");
				exit(1);
			}
				
			if(p1==0) {
				if (execvp(progname,args)==-1) {
					perror("execvp");
					exit(0);
				}
				else {
					return 0;
				}
			}
			kill(p1, SIGSTOP);
			check=Insert_New(p1, progname, args, max);
			if (check==0) {
				printf("Malloc didnt work.\n");
				exit(1);
			}
			List_Scheduler();
				
			//FREE DYNAMIC MEMORY
			if (args!=NULL) {
				for (i=0; i<max-1; i++) {
					free(args[i]);
				}
				free(args);
			}
		}
		else if (strcmp(command, "term")==0){
			pid=atoi(progname);
			found=Find_PID(pid);
			if (found!=NULL) {
				kill(pid, SIGTERM);
			}
		}
		else if (strcmp(command, "sig")==0){
			pid=atoi(progname);
			found=Find_PID(pid);
			if (found!=NULL) {
				kill(pid, SIGUSR1);
			}
		}
		else if (strcmp(command, "list")==0){
			Print_List();
		}
		else if (strcmp(command, "quit")==0){
			KILL_ALL();
			Free_List();
			res=0;
		}
	return res;
}

int main(int argc, char *argv[]) {
	char input[300];
	int res, flags;
	CURRENT=NULL;
	HEAD=NULL;
	struct sigaction act = { {0} };
	struct itimerval t = { {0} };
	
	act.sa_handler = handler;
	act.sa_flags=SA_RESTART;
	sigaction(SIGALRM,&act,NULL);
	
	t.it_value.tv_sec = 20;
	t.it_value.tv_usec = 0;
	t.it_interval.tv_sec =20;
	t.it_interval.tv_usec=0;
	
	//ΕΔΩ ΚΑΝΟΥΜΕ ΤΗΝ ΚΑΘΙΕΡΩΜΕΝΗ ΕΙΣΟΔΟ ΝΑ ΜΗΝ ΜΠΛΟΚΑΡΕΙ ΜΕΧΡΙ ΝΑ ΔΙΑΒΑΣΕΙ ΚΑΤΙ
	flags=fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	
	res=1;
	setitimer(ITIMER_REAL,&t,NULL);
	do {
		if (readcommand(input)!=0) {
			if (strcmp(input,"")!=0) {
				res=execute_command(input);
			}
		}
		if (KILL_TERMINATED()) { //ΕΔΩ ΑΝ ΤΕΛΕΙΩΣΕ ΜΙΑ ΔΙΕΡΓΑΣΙΑ ΑΡΧΙΚΟΠΟΙΟΥΜΕ ΞΑΝΑ ΤΟ ΞΥΠΝΗΤΗΡΙ
			t.it_value.tv_sec = 20;
			t.it_value.tv_usec = 0;
			t.it_interval.tv_sec =20;
			t.it_interval.tv_usec=0;
			setitimer(ITIMER_REAL,&t,NULL);
		}
		
	}while (res!=0);
	
	return 0;
}
