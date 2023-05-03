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
#include <limits.h>

int main (int argc, char * argv[]) {
	int i,file, name_bytes;
	mode_t mode;
	long int bytes, read_bytes=0;
	char data[512], input[128], *ptr;
	FILE *fil=stdin;
	struct stat status; 
	
	while (fgets(input, 128*sizeof(char),fil)!=NULL) {
		for (i=0; i<strlen(input); i++) {
			if (input[i]=='\n') {
				input[i]='\0';
				break;
			}
		}
		file=open(input,O_RDONLY);
		if (file<0) {
			perror("open");
			fprintf(stderr,"%s\n",strerror(errno));
			exit(0);
			
		}
		
		stat(input, &status);
		ptr=strrchr(input,'/');
		strcpy(input,ptr+1);
		name_bytes=strlen(input);
		mode=status.st_mode;
		bytes=status.st_size;
		ErrorFinder(write(1, &name_bytes, sizeof(int)),__LINE__);
		ErrorFinder(write(1, input, name_bytes),__LINE__);
		ErrorFinder(write(1,&status.st_atime, sizeof(time_t)),__LINE__);
		ErrorFinder(write(1,&status.st_mtime, sizeof(time_t)),__LINE__);
		ErrorFinder(write(1, &mode, sizeof(mode_t)),__LINE__);
		ErrorFinder(write(1, &bytes, sizeof(long int)),__LINE__);
		
		if (bytes>512) {
			while(bytes>512) {
				read_bytes=myread(file,data,512);
				mywrite(1,data,read_bytes);
				bytes=bytes-read_bytes;
			}
		}
		
		if (bytes<512) {
			read_bytes=myread(file,data,bytes);
			mywrite(1,data,read_bytes);
			bytes=bytes-read_bytes;
		}
		
		close(file);
		input[0]='\0';
	}
	return 0;
}

//----------------------ΣΧΟΛΙΑ----------------------
/* i===ένας απλός μετρητής που χρησιμοποιούμε στην for
 * file=== file descriptor για κάθε αρχέιο που ανοίγουμε
 * name_bytes===αντιπροσωπεύει πόσα bytes είναι το όνομα του κάθε αρχέιου
 * bytes===αντιπροσωπεύει πόσα bytes είναι τα περιεχόμενα του κάθε αρχέιου
 * read_bytes===αντιπροσωπεύει πόσα bytes διάβασε η συνάρτηση myread
 * data===ένας πίνακας χαρακτήρων στον οποίο αποθηκεύονται τα περιεχόμενα του αρχέιου που διαβάζουμε
 * input===ένας πίνακας χαρακτήρων στον οποίο αποθηκεύεται κάθε νέα γραμμή που διαβάζουμε απο την καθιερωμένη είσοδο (stdin)
 * ptr===ένας δείτκης σε χαρακτήρα που χρησιμοποιούμε για να αποσπάσουμε απο την εκάστοτε γραμμή μόνο το όνομα του αρχέιου
 * status===μια μεταβλήτη τύπου struct στην οποία αποθηκεύονται οι πληροφορίες του αρχείου που μας επιστρέφει η συνάρτηση stat
 */
