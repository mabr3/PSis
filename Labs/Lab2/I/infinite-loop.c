#include <stdio.h>
#include <stdlib.h>

long int random(void);

int main(){
	int v[100];
	int i;


	for (i=0; i<100; i++){
		v[i] = random();
	}

	printf("vector initialized\n");
	exit(0);

}
