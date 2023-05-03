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

int main (int argc, char * argv[]) {
	int new_file, name_bytes,length;
	mode_t mode;
	long int bytes, read_bytes=0;
	char data[512], name[200], pathname[200],*path;
	time_t access, modif;
	struct utimbuf output;
	DIR *dest;
	path=NULL;
	
	if (argc!=2) {
		fprintf(stderr, "ERROR in p2unarchive on line %d--> ",__LINE__);
		printf("No name or wrong name given.\n");
		exit(2);
	}
	else{
		dest=opendir(argv[1]);
	}
	
	if (dest==NULL && errno!=ENOENT) {
		fprintf(stderr, "ERROR in p2unarchive on line %d--> ",__LINE__);
		perror("open:");
		exit(0);
	}
	else if (dest==NULL && errno==ENOENT) {
		mkdir(argv[1],0777);
	}
	else {
		fprintf(stderr, "ERROR in p2unarchive on line %d--> ",__LINE__);
		fprintf(stderr,"Directory already exists.\n");
		exit(1);
	}
	
	path=realpath(argv[1],NULL);
	strcpy(pathname,path);
	length=strlen(pathname);
	
	while (read(0,&name_bytes, sizeof(int))>0) {
		ErrorFinder(read(0, name, name_bytes),__LINE__);
		name[name_bytes]='\0';
		ErrorFinder(read(0,&access,sizeof(time_t)),__LINE__);
		ErrorFinder(read(0,&modif,sizeof(time_t)),__LINE__);
		ErrorFinder(read(0, &mode, sizeof(mode_t)),__LINE__);
		ErrorFinder(read(0, &bytes, sizeof(long int)),__LINE__);
		strcat(pathname,"/");
		strcat(pathname,name);
		
		new_file=open(pathname, O_RDWR | O_CREAT | O_EXCL,S_IRWXU);
		if (new_file<0) {
			fprintf(stderr, "ERROR in p2unarchive on line %d--> ",__LINE__);
			perror("open");
			exit(4);
		}
		
		if (bytes>512) {
			while (bytes>512) {
				read_bytes=myread(0, data, 512);
				mywrite(new_file, data, read_bytes);
				bytes=bytes-read_bytes;
			}
		}
		
		if (bytes<512) {
			read_bytes=myread(0, data, bytes);
			mywrite(new_file, data, read_bytes);
			bytes=bytes-read_bytes;
		}
		
		output.actime=access;
		output.modtime=modif;
		utime(pathname, &output);
		chmod(pathname, mode);
		close(new_file);
		data[0]='\0';
		name[0]='\0';
		pathname[length]='\0';
	}
	free(path);
	return 0;
}

//----------------------ΣΧΟΛΙΑ----------------------
 /* new_file=== file descriptor για κάθε αρχέιο που ανοίγουμε
 * name_bytes===αντιπροσωπεύει πόσα bytes είναι το όνομα του κάθε αρχέιου
 * bytes===αντιπροσωπεύει πόσα bytes είναι τα περιεχόμενα του κάθε αρχέιου
 * read_bytes===αντιπροσωπεύει πόσα bytes διάβασε η συνάρτηση myread
 * data===ένας πίνακας χαρακτήρων στον οποίο αποθηκεύονται τα περιεχόμενα του αρχέιου που διαβάζουμε
 * name===ένας πίνακας χαρακτήρων στον οποίο αποθηκεύεται το όνομα του εκάστοτε αρχέιου
 * path===ένας δείτκης σε χαρακτήρα που χρησιμοποιούμε για να βρούμε το absolute path κάθε αρχείου χρησιμοποιώντας την συνάρτηση realpath (ουσιαστικά ένα δυναμικό string γιατί έτσι λειτουργεί η συνάρτηση realpath ) 
 * pathname===το absolute path ενός αρχέιου μαζί με το ονομά του
 * access===χρονοσήμανση της τελευταίας εισόδου του αρχέιου
 * modif===χρονοσήμανση της τελευταίας μετατροπής του αρχέιου
 * output===μια μεταβλήτη τύπου struct στην οποία αποθηκεύονται οι πληροφορίες του αρχείου (χρονοσημάνσεις).
 * 
 * 
 * 
 * Το σκεπτικό μας είναι να διαβάζουμε τα δεδομένα με την συνάρτηση myread,να τα αποθηκεύουμε σε κατάλληλες μεταβλητές,δημιουργούμε τα αρχεία και γράφουμε τα δεδομένα τους με την συνάρτηση mywrite, 
 * αλλάζουμε τις χρονοσημάνσεις και δικαιωματά τους, και συνεχίζουμε τα ίδια βήματα μέχρι να τελειώσουν όλα τα δεδομένα.
 */