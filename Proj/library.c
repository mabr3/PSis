#include "clipboard.h"

int clipboard_connect(char * clipboard_dir){
	int fd_clipboard;
	struct sockaddr_un clipboard_addr;

	fd_clipboard = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd_clipboard ==-1){
		perror("Socket fd_clipboard: ");
		return -1;
	}else{
		clipboard_addr.sun_family = AF_UNIX;
		sprintf(clipboard_addr.sun_path, "./%s", SOCK_ADDRESS);
		if(connect(fd_clipboard, (const struct sockaddr *) &clipboard_addr,sizeof(clipboard_addr))==-1){
			perror("Error connecting: ");
			return(-1);
		}else{
			return fd_clipboard;
		}
	}
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){

	int nsend =-1;
	int size = count;
	char *aux=NULL;
	aux=malloc(30);
	signal(SIGPIPE, SIG_IGN);
	memset(aux,(int)'\0',30);
	sprintf(aux,"%d;%d;%d;%ld;",clipboard_id,region,1,count);


	#if DEBUG2
	printf("aux é %s e tamanho é\n",aux);
	#endif

	if(region<0 || region>9){
		printf("Region must be between 0 and 9, inclusive.\n");
		return 0;
	}


	nsend=send(clipboard_id,aux,30,0);
	#if DEBUG2
	printf("ENviei %d bytes\n",nsend);
	#endif
	if(nsend ==-1 || nsend==0){
		free(aux);
		perror("Error sending information to the clipboard: ");
		return 0;
	}
	free(aux);

	#if DEBUG
	printf("enviei %d\n",nsend);
	#endif


	/*1o envia informação da região onde escrever, clipboard id e
	*tamanho do que vem a seguir*/

	/*para ignorar o número referente à região e o espaço que vem a seguir*/
	buf = buf +2;

	nsend = send(clipboard_id,buf,size,0);
	#if DEBUG2
	printf("Enviei %d bytes-buffer\n",nsend);
	#endif
	if(nsend ==-1){
		perror("Error sending information to the clipboard: ");
		return 0;
	}

	return nsend;/*mudar returnss!*/
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){

	int nsend =-1;
	char *aux=NULL;
	signal(SIGPIPE, SIG_IGN);

	aux=malloc(30);
	memset(aux,(int)'\0',30);
	sprintf(aux,"%d;%d;%d;%ld;",clipboard_id,region,2,count);


	#if DEBUG
	printf("aux é %s e tamanho é \n",aux);
	#endif

	if(region<0 || region>9){
		printf("Region must be between 0 and 9, inclusive.\n");
		return 0;
	}
	getchar();
	nsend=send(clipboard_id,aux,30,0);
	free(aux);
	if(nsend ==-1){
		perror("Error sending information to the clipboard: ");
		return 0;
	}


	int nread = recv(clipboard_id,buf,count,0);
	if(nread ==-1){
		perror("Error receiving information from the clipboard: ");
		return 0;
	}
	#if DEBUG
	printf("nread é:%d\n",nread);
	#endif
	
	return nread;
}

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count){
	char *aux=NULL;
	aux=malloc(30);
	int n;
	signal(SIGPIPE, SIG_IGN);
	memset(aux,(int)'\0',30);
	sprintf(aux,"%d;%d;%d;%ld;",clipboard_id,region,3,count);

	#if DEBUG
	printf("aux é %s e tamanho é\n",aux);
	#endif
	if(region<0 || region>9){
		printf("Region must be between 0 and 9, inclusive.\n");
		return 0;
	}

	n=send(clipboard_id,aux,30,0);
	free(aux);
	if(n ==-1){
		perror("Error sending information to the clipboard: ");
		return 0;
	}

	#if DEBUG
	printf("Estou à espera de receber no wait na library\n");
	#endif

	n=recv(clipboard_id,buf,count,0);
	if(n ==-1){
		perror("Error receiving information from the clipboard: ");
		return 0;
	}
	return n;
}

void clipboard_close(int clipboard_id){
	close (clipboard_id);
	return;
}
