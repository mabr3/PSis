#include <stdlib.h>
#include <stdio.h>


int main(int argc, char * argv[]){
	int i=0;
	int j=0;
	int c=0;
	char * result_str;
	int str_length = 0;

	if(argc==1){
		printf("No arguments. Exiting...\n");
		exit(0);
	}

	/*Ver tamanho total a alocar*/
	for(i=1;i<argc; i++){
		for(j=0;argv[i][j]!='\0';j++){
			str_length++;
		}
	}

	result_str = (char*)malloc(str_length*sizeof(char) + 1);

	for(i=1;i<argc; i++){
		for(j=0;argv[i][j]!='\0';j++){
			result_str[c]=argv[i][j];
			c++;
		}
	}
	free(result_str);
	printf("\n%s\n", result_str);
	exit(0);
}
