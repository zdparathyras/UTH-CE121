#include <stdio.h>
#include "mylib.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int main (int argc, char * argv[]) {
	char *key,data[512];
	long int bytes=512, read_bytes;
	int i,j;
	
	if (argc!=2) {
		fprintf(stderr, "ERROR in P2CRYPT on line %d--> ",__LINE__);
		fprintf(stderr, "Wrong number of parameters.\n");
		exit(2);
	}
	key=strdup(argv[1]);
	i=0;
	j=0;
	while (1) {
		read_bytes=myread(0,data,bytes);
		
		for (i=0; i<read_bytes; i++) {
			data[i]=data[i]^key[j];
			j++;
			if (j>=strlen(key)) {
				j=0;
			}
		}
		mywrite(1,data,read_bytes);
		if (read_bytes<bytes) {
			break;
		}
	}
	
	free(key);
	return 0;
}

/*-----------------------------------
Σχόλια:
Οι συναρτήσεις myread και mywrite που έχουμε φτίαξει βρίσκονται στο mylib.c το αρχείο της δικής μας βιβλιοθήκης mylib.h.
Το σκεπτικό της p2crypt είναι πως διαβάζουμε τα δεδομένα (σε blocks των 512 byte ή λιγότερα) με την συνάρτηση myread ,
	τα κάνουμε XOR byte προς byte, αν έχουμε φτάσει στο τέλος του κλειδιού ξεκινάμε πάλι απο την αρχή του ( του κλειδιού) ,
	και τέλος τα γράγουμε στην καθιερωμένη έξοδο με την συνάρτηση mywrite. 
Κάνουμε τα ίδια ακριβώς βήματα ξανά και ξανά μέχρι να τελειώσουν τα δεδομένα.*/