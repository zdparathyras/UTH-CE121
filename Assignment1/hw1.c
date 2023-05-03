#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void ErrorFinder(int fd);
int MyFstat(int fd);
int FindTheFile(int database, char name[]);
void import(char name[], int database);
void find(char name[], int database);
void export(char src[], char dest[], int database);
void delete(char name[],int database, int database2);

int main(int argc,char* argv[]){
	char magic[10]="2186-2165", check[10], option, name[100],source[100],dest[100];
	int  database, database2;
	if (argc!=2) {
		printf("wrong number of parameters\n");
		exit(0);
	}
	
	//Εδώ ελέγχουμε αν το αρχείο που θέλουμε να ανοίξουμε είναι αρχέιο βάσης δεδομένων(αν έχει δηλαδή την υπογραφή μας (το string magic "2186-2165")), και αν δεν την έχει εκτυπώνουμε κατάλληλο μήνυμα και το πρόγραμμα τερματίζει.  
	database=open(argv[1],O_RDWR);
	if (database<0) {
		database=open(argv[1],O_RDWR|O_CREAT,S_IRWXU);
		if (database<0) {
			perror("open");
			return 0;
		}
		ErrorFinder(write(database,magic,9));
	}
	else {
		ErrorFinder(read(database,check,9));
		check[9]='\0';
		if (strcmp(check,magic)!=0) {
			printf("Not a database file.\n");
			exit(0);
		}
	}
	
	
	do {
		printf("(i)mport\n(f)ind\n(e)xport\n(d)elete\n(q)uit\n");
		printf("Select option\n");
		scanf(" %c", &option);
		
		while (option!='q') {
			
			switch (option) {
			
			case 'i' :
				printf("Write the file's name or path you want to import.\n");
				scanf("%99s", name);
				import(name,database);
				break;
				
			case 'f' :
				printf("Write the file's name you want to find.\n");
				scanf("%99s", name);
				find(name,database);
				break;
				
			case 'e' :
				printf("Write the file's name you want to export.\n");
				scanf("%99s", source);
				printf("Write the file's name you want to export to.\n");
				scanf("%99s", dest);
				export(source, dest,database);
				break;
				
			case 'd' :
				database2=open(argv[1],O_RDWR,S_IRWXU);
				ErrorFinder(database2);
				printf("Write the file's name you want to delete.\n");
				scanf("%99s", name);
				delete(name,database, database2);
				close(database2);
				break;
				
			case 'q' :
				exit(0);
			}
			printf("Select option\n");
			scanf(" %c", &option);
		}
	
	}while ( (option!='i') && (option!='f') && (option!='e') && (option!='d') && (option!='q') );
	
	close(database);
	return 0;
}

//Αυτή η συνάρτηση βρίσκει αν μια εντολή δεν λειτούργησε και εκτυπώνει τα error της, χρησιμοποιώντας την έτοιμη συνάρτηση perror.
void ErrorFinder(int fd) {
	if (fd<0) {
		perror("Error: ");
		return;
	}
}

//Αυτή η συνάρτηση βρίσκει και επιστρέφει το μέγεθος ενός αρχείου σε bytes χρησιμοποιώντας την έτοιμη συνάρτηση fstat.
int MyFstat(int fd) {
	int bytes;
	struct stat st;
	fstat(fd, &st);
	bytes=st.st_size;
	return(bytes);
}

//Αυτή η συνάρτηση βρίσκει αν ένα αρχείο υπάρχει μέσα στην βάση δεδομένων και επιστρέφει έναν ακέραιο (flag) με τιμή 0 αν το αρχείο δεν υπάρχει και 1 αντίθετα.
int FindTheFile(int database, char name[]){
	//Η μεταβλητή all_bytes σημαίνει το μέγεθος όλης της βάσης δεδομένων.
	//Η μεταβλητή current είναι ένας μετρητής που μας δείχνει σε ποιο byte της βάσης βρισκόμαστε.
	//Η μεταβλητή name_size παριστάνει το μέγεθος του ονόματος του αρχείου.
	//Η μεταβλητή temp_name παριστάνει το όνομα του αρχείου που διαβάσαμε απο την βάση.
	//Η μεταβλητή bytes_of_file παριστάνει το μέγεθος του περιεχομένου του αρχείου που διαβάσαμε απο την βάση.
	int all_bytes, current, name_size, bytes_of_file, result=0;
	char temp_name[100];
	all_bytes=MyFstat(database);
	
	ErrorFinder(lseek(database,(off_t)+9,SEEK_SET));
	
	current=9;
	while(current<all_bytes) {
		ErrorFinder(read(database, &name_size, sizeof(int)));
		current=current + sizeof(int);
		
		ErrorFinder(read(database, temp_name, name_size));
		current=current + name_size;
		temp_name[name_size]='\0'; 
		
		if (strcmp(name, temp_name)==0) { 
			result=1;
			return(result);
		}
		ErrorFinder(read(database,&bytes_of_file,sizeof(int)));
		current=current + sizeof(int);
		ErrorFinder(lseek(database,bytes_of_file,SEEK_CUR));
		current=current + bytes_of_file;
	}
	
	
	return(result);
}

void import( char name[], int database) {
	int fd, bytes, bytes_of_name, found;
	char *ptr,str[512];
	ptr=NULL;
	
	ErrorFinder(lseek(database,(off_t)+9,SEEK_SET));
	
	fd=open(name,O_RDONLY);
	ErrorFinder(fd);
	
	//Εδώ αποσπούμε το όνομα του αρχείου απο το path.
	ptr=strchr(name,'/');
	if (ptr!=NULL) {
		ptr=strrchr(name,'/');
		strcpy(name,ptr+1);
	}
	
	//Εδώ ψάχνουμε αν υπάρχει ήδη το αρχείο στη βάση χρησιμοποιώντας την συνάρτηση FindTheFile που έχουμε δημιουργήσει. 
	found=FindTheFile(database, name);
	if (found==1) {
		printf("File already exists in database.\n");
		return;
	}
	
	
	ErrorFinder(lseek(database,0,SEEK_END));
	
	//Εδώ παίρνουμε απο την MyFstat το μέγεθος του αρχείου σε bytes.
	bytes=MyFstat(fd);
	//Και το μέγεθος του ονόματος του χρησιμοποιώντας την strlen.
	bytes_of_name=strlen(name);
	
	if (bytes>0) {
		//Eδώ εκχωρoύμε το μέγεθος του ονόματος του αρχείου (σε binary) στη βάση δεδομένων.
		ErrorFinder(write(database,&bytes_of_name,sizeof(int)));
		//Eδώ εκχωρoύμε το όνομα του αρχείου στη βάση δεδομένων.
		ErrorFinder(write(database,name,bytes_of_name));
		//Eδώ εκχωρoύμε το μέγεθος του περιεχομένου του αρχείου (σε binary) στη βάση δεδομένων.
		ErrorFinder(write(database,&bytes,sizeof(int)));
		
		//Εδώ μοιράζουμε το αρχείο σε κομμάτια (blocks των 512) αν είναι μεγαλύτερο των 512 bytes και τα εκχωρoύμε.
		if (bytes>512) {
			while (bytes>512) {
				ErrorFinder(read(fd,str,512));
				ErrorFinder(write(database,str,512));
				bytes=bytes-512;
				
			}
		}
		if (bytes<=512) {
			ErrorFinder(read(fd,str,bytes));
			ErrorFinder(write(database,str,bytes));
		}
		
	}
	
	fsync(database);
	close(fd);
	
}

void find(char name[], int database){
	int bytes_of_file, name_size, current=0, found=-1, all_bytes;
	char temp_name[100];
	all_bytes=MyFstat(database);
	
	if (all_bytes!=0) {
		ErrorFinder(lseek(database,(off_t)+9, SEEK_SET));
	}
	
	if (all_bytes==9) {
		printf("Database is empty.\n");
		return;
	}
	
	
	//Εδώ ο κώδικας είναι ίδιος με αυτόν που έχουμε γράψει στη συνάρτηση FindTheFile απλά αλλάζουμε την συνθήκη της if.
	current=9;
	while(current<all_bytes) {
		ErrorFinder(read(database, &name_size, sizeof(int)));
		current=current + sizeof(int);
			
		ErrorFinder(read(database, temp_name, name_size));
		current=current+ name_size;
		temp_name[name_size]='\0';
		if (strcmp(name,"*")==0 || strstr(temp_name,name)!=NULL) {
			printf("%s\n", temp_name);
			found++;
		}
		ErrorFinder(read(database,&bytes_of_file,sizeof(int)));
		current=current + sizeof(int);
		ErrorFinder(lseek(database,(off_t)+bytes_of_file,SEEK_CUR));
		current=current + bytes_of_file;
	}
	if(found==-1) {
		printf("File \"%s\" does not exists on database.\n", name);
		return;
	}
	
}

void export(char source[], char dest[], int database){
	int dest_fd, src_bytes, found, all_bytes;
	char transport[512];
	all_bytes=MyFstat(database);
	
	if (all_bytes!=0) {
		ErrorFinder(lseek(database,(off_t)+9,SEEK_SET));
	}
	else {
		printf("Database is empty.\n");
		return;
	}
	
	//Εδώ ψάχνουμε αν υπάρχει ήδη το αρχείο στη βάση χρησιμοποιώντας την συνάρτηση FindTheFile που έχουμε δημιουργήσει. 
	found=FindTheFile(database, source);
	if (found==1) {
		printf("Found source file.\n");
	}
	else{
		printf("Source file does not exist on database.\n");
		return;
	}
	
	
	//Εδώ ελέγχουμε αν το αρχείο στο οποίο θέλουμε να αντιγράψουμε τα δεδομένα υπάρχει ή όχι, και εκτυπώνουμε τα κατάλληλα μηνύματα.
	dest_fd=open(dest,O_RDWR|O_CREAT|O_EXCL,S_IRWXU);
	if (dest_fd<0 && errno == EEXIST) {
		printf("Destination file already existed.\n");
		return;
	}
	else if (dest_fd<0) {
		perror("open");
		return;
	} 
	else {
		printf("File didnt exist and its been created.\n");
	}
	
	//Εδώ μοιράζουμε το αρχείο σε κομμάτια (blocks των 512) αν είναι μεγαλύτερο των 512 bytes και τα εκχωρoύμε.
	ErrorFinder(read(database,&src_bytes,sizeof(int)));
	if (src_bytes>512) {
		while (src_bytes>512) {
			ErrorFinder(read(database,transport,512));
			ErrorFinder(write(dest_fd,transport,512));
			src_bytes=src_bytes-512;
			
		}
	}
	
	if(src_bytes<512) {
		ErrorFinder(read(database,transport,src_bytes));
		ErrorFinder(write(dest_fd,transport,src_bytes));
	}
	
	
	fsync(dest_fd);
	close(dest_fd);
}



//Το σκεπτικό μας είναι να έχουμε δύο file descriptors έναν εκεί που αρχίζουν οι πληροφορίες του αρχείου που θέλουμε να διαγράψουμε(database) και έναν εκεί που αρχίζουν οι πληροφορίες του αμέσως επόμενου αρχείου (database2) και να αντιγράψουμε όσα bytes υπάρχουν απο τον δεύτερο και μετά πάνω στον πρώτο (overwrite).
void delete(char name[], int database, int database2){
	int  current=0, name_size=0, bytes_of_file=0, temp_number, copied=0, all_bytes;
	char temp_name[100], overwrite[512];
	all_bytes=MyFstat(database);
	
	ErrorFinder(lseek(database2,0,SEEK_SET));
	ErrorFinder(lseek(database,0,SEEK_SET));
	
	if (all_bytes!=0) {
		ErrorFinder(lseek(database,(off_t)+9, SEEK_SET));
		current=9;
	}
	else {
		printf("Database is empty.\n");
		return;
	}
	
	//Εδώ ψάχνουμε αν υπάρχει ήδη το αρχείο στη βάση χρησιμοποιώντας τον κώδικα της συνάρτησης FindTheFile που έχουμε δημιουργήσει.
	//Θα αλλάζαμε τον κώδικα έτσι ώστε να καλούμε την FindTheFile αλλά θέλουμε να κρατήσουμε κάποιες επιπλέον πληροφορίες που αν την καλούσαμε δεν θα τις είχαμε.
	//Δηλαδή τις μεταβλητές current ,bytes_of_file και την θέση του file descriptor database.
	while(current<all_bytes) {
		ErrorFinder(read(database, &name_size, sizeof(int))); 
		current=current + sizeof(int);
		ErrorFinder(read(database, temp_name, name_size));
		current=current + name_size;
		temp_name[name_size]='\0';
		if (strcmp(temp_name,name)==0) {
			printf("Found file.\n");
			break;
		}
		ErrorFinder(read(database,&bytes_of_file,sizeof(int)));
		current=current + sizeof(int);
		ErrorFinder(lseek(database,(off_t)+bytes_of_file,SEEK_CUR));
		current=current + bytes_of_file;
	}
	
	//Εδώ διαβάζουμε απο την βάση δεδομένων το μέγεθος του αρχείου που θέλουμε να διαγράψουμε και το εκχωρoύμε στην μεταβλητή bytes_of_file.
	ErrorFinder(read(database,&bytes_of_file,sizeof(int)));
	
	//Αλλάζουμε την τιμή του current γιατί χρησιμοποιούμε αυτή τη μεταβλητή για να ξέρουμε σε ποιο byte της βάσης βρισκόμαστε.
	current=current - sizeof(int) - name_size;
	
	//Μετακινούμε τον file descriptor database κατάλληλα με τον current, έτσι ώστε να βρίσκεται ακριβώς εκεί που αρχίζουν οι πληροφορίες για το αρχείο που θέλουμε να διαγράψουμε.
	ErrorFinder(lseek(database,(off_t)+current,SEEK_SET));
	
	//Αλλάζουμε την τιμή του current γιατί χρησιμοποιούμε αυτή τη μεταβλητή για να ξέρουμε σε ποιο byte της βάσης βρισκόμαστε.
	current=current + bytes_of_file+ name_size + 2 * sizeof(int); 
	
	//Μετακινούμε τον file descriptor database2 κατάλληλα με τον current, έτσι ώστε να βρίσκεται ακριβώς εκεί που αρχίζουν οι πληροφορίες για το επομένο αρχείο απο το αρχείο που θέλουμε να διαγράψουμε.
	ErrorFinder(lseek(database2,(off_t)+current, SEEK_CUR));
	
	//Χρησιμοποιούμε την μεταβλητή copied ως μετρητή που μετράει πόσα bytes έχουμε αντιγράψει.
	while(copied<all_bytes-current) { //Αll_bytes - current είναι το πλήθος των bytes που βρίσκονται μετά το αρχείο που θέλουμε να διαγράψουμε.
		ErrorFinder(read(database2,&temp_number,sizeof(int)));
		ErrorFinder(write(database,&temp_number,sizeof(int)));
		copied=copied+sizeof(int);
			
		ErrorFinder(read(database2,overwrite,temp_number));
		ErrorFinder(write(database,overwrite,temp_number));
		copied=copied+temp_number;
			
		ErrorFinder(read(database2,&temp_number,sizeof(int)));
		ErrorFinder(write(database,&temp_number, sizeof(int)));
		copied=copied+sizeof(int);
		copied=copied+temp_number;
		
		//Εδώ μοιράζουμε το αρχείο σε κομμάτια (blocks των 512) αν είναι μεγαλύτερο των 512 bytes και τα εκχωρoύμε.
		if (temp_number>512) {
			while (temp_number>512) {
				ErrorFinder(read(database2,overwrite,512));
				ErrorFinder(write(database,overwrite,512));
				temp_number=temp_number-512;
			}
		}
		if (temp_number<512) {
			ErrorFinder(read(database2,overwrite,temp_number));
			ErrorFinder(write(database,overwrite,temp_number));
		}
	}
	ErrorFinder(ftruncate(database,all_bytes - bytes_of_file - name_size- 2*sizeof(int) ));
	
	fsync(database);
}
