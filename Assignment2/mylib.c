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

void ErrorFinder(int fd, int line) {
	if (fd<0) {
		fprintf(stderr, "ERROR in p2unarchive on line %d--> ",line);
		perror("Error: ");
		return;
	}
}

long int myread(int fd, char *str, long int max_bytes){
	long int read_bytes=0, temp=0;
	
	read_bytes=read(fd,str,max_bytes);
	if (read_bytes<0) {
		perror("read");
		exit(6);
	}
	
	while(read_bytes<max_bytes) {
		temp=read(fd,str+read_bytes,max_bytes-read_bytes);
		if (temp==0) break;
		if (temp<0) {
			perror("read2");
			exit(6);
		}
		read_bytes=read_bytes + temp;
	}
	return read_bytes;
}

long int mywrite (int fd,char str[],long int max_bytes) {
	long int writen_bytes=0, temp=0;
	
	writen_bytes=write(fd,str, max_bytes);
	if(writen_bytes<0) {
		perror("write");
		exit(6);
	}
	while(writen_bytes<max_bytes) {
		temp=write(fd,str+writen_bytes, max_bytes-writen_bytes);
		if (temp<0) {
			perror("write2");
			exit(6);
		}
		writen_bytes=writen_bytes + temp;
	}
	return writen_bytes;
}

//----------------------------------------------ΣΧΟΛΙΑ------------------------------------
/*ΕrrorFinder: η συνάρτηση αυτή παίρνει ως όρισμα έναν ακέραιο ο οποίος συνήθως είναι το αποτέλεσμα των συναρτήσεων read, write ,open ,fork κλπ και αν η τιμή του είναι μικρότερη του μηδενός (δηλαδή error) εκτυπώνει με την βοήθεια της 
 *  συνάρτησης perror ποιο είναι το πρόβλημα και μετά εκτυπώνει το δεύτερο όρισμα που είναι η γραμμή στην οποία υπάρχει το πρόβλημα.
 * 
 * myread: η συνάρτηση αυτή παίρνει ως ορίσματα έναν ακέραιο fd (ο οποίος είναι file descriptor και αντιπροσωπεύει το απο που θα διαβάσουμε τα δεδομένα) ένα δείκτη σε string (στον οποίο θα αποθηκευτούν τα δεδομένα και αυτός θα τα γυρίσει στο πρόγραμμα που το κάλεσε και θα τα αποθηκεύσει στο string στο οποίο δείχνει) και τέλος ένα long int ο οποίος αντιπροσωπεύει πόσα bytes θέλουμε να διαβάσουμε.
 * Το σκεπτικό μας είναι πως η myread θα διαβάζει όσα bytes ζητήσαμε και αν απέτυχε η πρώτη read θα συνεχίσει να διαβάζει μέχρι να διαβαστούν όσα ακριβώς ζητήσαμε.
 * 
 * mywrite: το σκεπτικό είναι το ίδιο με την myread αλλα αντί να διαβάζει γράφει δεδομένα και τα ορίσματα αντιπροσωπεύουν ακριβώς τα ίδια πράγματα.
 */