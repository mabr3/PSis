#include "clipboard.h"




void intro(){
	printf("\t\tLocal Clipoard. What do you want to do? Insert the number of your choice\n\n");
	printf("\t 0 - Exit the application.\n");
	printf("\t 1 - Copy something to the clipboard.\n");
	printf("\t 2 - Paste something from the clipboard.\n");
	printf("\t 3 - Wait for a region to be updated.\n");
}

void copy(int fd){
	char dados[100];
	int cp=-1;

	printf("Enter you message with the following format: Region Message\n");
	getchar();
	fgets(dados, 100, stdin);

	/*Check para erros de região*/
	if(atoi(dados)<0 || atoi(dados)>9 || !isdigit(dados[0])){
		printf("Region must be a number between 0 and 9\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n");
		return;
	}

	/*check para erro da mensagem*/
	if(strlen(dados)<3 || dados[1]!=' '){
		printf("Bad message input.\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n\n");
		return;
	}
	cp=clipboard_copy(fd,atoi(dados), dados, (strlen(dados)-2)*sizeof(char));
	if(cp==0){
		printf("\nError: invalid clipboard OR invalid region OR local clipboard unavailable\n");
		return;
	}
	printf("\nMessage copied. Back to the menu. Choose 1,2,3 or 0.\n");
  return;
}

void paste(int fd){
	int region=-1;
	int max=0;
	int pt=-1;

	void * buffer = NULL;
	printf("What region do you want to paste from? (from 0 to 9)\n");
	/*getchar();*/

	if(scanf("%d",&region)!=1 ||region<0 || region>9){
		printf("Region must be a number between 0 and 9\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n\n");
		return;
	}

	printf("How many characters? Must be >1\n");

	if(scanf("%d",&max)!=1 || max<2){
		printf("Max must be a number >1\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n");
		return;
	}

  buffer = (void*)malloc(max*sizeof(void)+1);

	pt=clipboard_paste(fd, region, buffer, (size_t) max);

	if(pt==0){
		printf("Error: invalid clipboard OR invalid region OR local clipboard unavailable\n");
		free(buffer);
		return;
	}

	char toPrint[max+1];
	memcpy(toPrint,buffer,max);


	toPrint[max]='\0';

	if(pt!=1){
		printf("Pasted message from region %d:\n",region);
		printf("%s",toPrint);
	}else{
		printf("There's nothing on region %d yet.\n",region);
	}
	free(buffer);
	printf("\nBack to the menu. Choose 1,2,3 or 0.\n");
	return;

}

void wait(int fd){
	int region=-1;
	int max=0;
	int npaste=-1;

	int wt=-1;

	void * buffer = NULL;
	printf("What region do you want to wait for? (from 0 to 9)\n");
	/*getchar();*/

	if(scanf("%d",&region)!=1 ||region<0 || region>9){
		printf("Region must be a number between 0 and 9\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n");
		return;
	}

	printf("How many characters? Must be >1\n");

	if(scanf("%d",&max)!=1 || max<2){
		printf("Max must be a number >1\n");
		printf("Back to the menu. Choose 1,2,3 or 0.\n");
		return;
	}

	buffer = (void*)malloc(max*sizeof(void)+1);
	#if DEBUG
	printf("Estou a espera de receber do wait\n");
	#endif
	wt=clipboard_wait(fd, region, buffer, (size_t) max);

	if(wt==0){
		printf("Error: invalid clipboard OR invalid region OR local clipboard unavailable\n");
		free(buffer);
		return;
	}

	char toPrint[max+1];
	memcpy(toPrint,buffer,max);
	toPrint[max]='\0';

	/*if(!strcmp((char*)buffer,'\0')){*/
	if(npaste!=1){
		printf("Pasted message from region %d, after waiting:\n",region);
		printf("%s",toPrint);
	}

	free(buffer);
	printf("\nBack to the menu. Choose 1,2,3 or 0.\n");

	return;
}

int main(){
	int c=-1;

	int fd=clipboard_connect("./");

	if(fd== -1){
		printf("Error opening the clipboard. Exiting.\n");
		exit(-1);
	}

	intro();
	while(c!=0){
		scanf("%d",&c);

		switch(c){
			case 0:
				clipboard_close(fd);
				break;
			case 1:
				copy(fd);
				break;

			case 2:
				paste(fd);
				break;
			case 3:
				wait(fd);
				break;
			default:
				printf("Print a valid input like described in the menu:\n");
				intro();
			break;
		}
	}
	exit(0);
}
