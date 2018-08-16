#include "clipboard.h"
#include <signal.h>

#include <sys/wait.h>

#define max(A,B) ((A)>=(B) ? (A):(B))

typedef struct Mensagem{
  void* mensagem;
  size_t size;
  int cont;
}Mensagem;

struct Mensagem mensagens[10];
pthread_mutex_t mutex[10]= {PTHREAD_MUTEX_INITIALIZER};/*1 mutex para cada região*/
pthread_cond_t cond[10]= {PTHREAD_COND_INITIALIZER};/*1 cond para cada região*/

typedef struct ClipB{
  int fd;
  struct ClipB *next;
  struct ClipB *prev;
  pthread_t thread;
}ClipB;

struct ClipB *parent;
ClipB*filhos;
pthread_t aceita_filhos;
pthread_t aceita_apps;
pthread_t parent_thread;
int parent_fd=-1;

pthread_t thread_ids[100]={-1};/*threads apps*/
int client_fds[100];

void imprime_filhos(){
  ClipB*ptr;
  printf("vou imprimir os filhos\n");
  for(ptr=filhos;ptr!=NULL;ptr=ptr->next){
    printf("o meu fd é %d e a minha thread é %ld\n",ptr->fd,ptr->thread);
  }
}

void sigint_flagger(int sig){
  unlink(SOCK_ADDRESS);
  printf("Exiting!\n");
  ClipB * ptr;
  ClipB * aux;
  int i=0;
  /*stop=1;*/

  for(i=0;i<100;i++){
    if(thread_ids[i]!=-1)
      //close(client_fds[i]);
      pthread_cancel(thread_ids[i]);
  }
  printf("Apps - done\n");

  pthread_cancel(aceita_apps);
  printf("Accept apps thread - done;\n");

  for(ptr=filhos;ptr!=NULL;){
    close(ptr->fd);
    aux=ptr;
    ptr=ptr->next;
    #if DEBUG
    printf("Vou cancelar a thread %ld\n",aux->thread);
    #endif
    pthread_cancel(aux->thread);
    free(aux);
  }

  printf("Clipboards - Done\n");
  if(parent_fd!=-1){
    #if DEBUG
    printf("vou fazer cancel à parent_thread %ld\n",parent_thread);
    #endif
    pthread_cancel(parent_thread);
    printf("Parent Clipboard - Done\n");

  }

  pthread_cancel(aceita_filhos);
  printf("Accept clipboards thread - done;\n");

  for(i=0;i<10;i++){
    if(mensagens[i].mensagem!=NULL){
      free(mensagens[i].mensagem);
    }
  }
  printf("Messages - done!\n");

  return;
}

void atualiza_data(int region, void * data, size_t size){
  mensagens[region].cont++;
  if(mensagens[region].mensagem == NULL){/*não existe msg nessa reg*/
    mensagens[region].mensagem= (void*)malloc(size * sizeof(void));
    memcpy(mensagens[region].mensagem,data, size*sizeof(void));
    mensagens[region].size=size;
  }else{/*existe msg nessa reg*/
    free(mensagens[region].mensagem);
    mensagens[region].mensagem= (void*)malloc(size * sizeof(void));
    memcpy(mensagens[region].mensagem,data, size*sizeof(void));
    mensagens[region].size=size;
  }

  return;

}


void envia_para_filhos(void* data, char*h_aux){
  ClipB * ptr=NULL;
  ClipB * aux=NULL;
  int nbytes=-1;
  int c=0;

  int clipboard_id=-1;
  int region=-1;
  int function=-1;
  size_t size=-1;


  sscanf(h_aux,"%d;%d;%d;%ld",&clipboard_id,&region,&function, &size);
  /*enviar para os filhos!*/
  #if DEBUG
  printf("vou enviar para os meus filhos.\n");
  #endif
  for(ptr=filhos;ptr!=NULL;ptr=ptr->next){
    nbytes=send(ptr->fd,h_aux,30,0);/*envia informacoes*/
    #if DEBUG2
    printf("Enviei %s, nbytes:%d para o filho %d\n",h_aux,nbytes,ptr->fd);
    #endif
    if(nbytes==-1){
      perror("send: ");
      exit(-1);
    }
    if(nbytes==0){
      if(ptr->next!=NULL){
        if(aux!=NULL)free(aux);
        if(ptr->prev!=NULL){
          ptr->prev->next=ptr->next;
          ptr->next->prev=ptr->prev;
          aux=ptr;
          ptr=ptr->prev;
          free(aux);
        }else{
          ptr->next->prev=NULL;
          filhos=ptr->next;
          aux=ptr;
        }
      }else{/*nao ha next*/
        if(ptr->prev !=NULL){
          ptr->prev->next=NULL;
          aux=ptr;
          free(aux);
          break;
        }
      }
    }
    if(nbytes>0){/*envia data para os filhos*/
      nbytes=send(ptr->fd,data,size,0);
      #if DEBUG
      printf("Enviei %d data-> %s\n",nbytes,(char*)data);
      #endif
      c++;
      if(nbytes==-1){
        perror("send: ");
        exit(-1);
      }
    }
  }
  #if DEBUG
  printf("Enviei para %d filhos\n",c);
  #endif

  return;
}

void * thread_recebe_parent(void * input){

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  char* data=NULL;
  char h_aux[30];
  int nbytes=-1;
  int clipboard_id=-1;
  int region=-1;
  size_t size =-1;
  int function =-1;

  while(1){

    memset((void*)&h_aux, (int)'\0',sizeof(h_aux));
    nbytes = recv(parent_fd,h_aux,30,0);

    #if DEBUG
    printf("RECEBI DO PARENT- h_aux nbytes: %d\n",nbytes);
    #endif

    if(nbytes==-1){
      perror("recv: ");
      exit(-1);
    }
    if(nbytes>0){
      sscanf(h_aux,"%d;%d;%d;%ld",&clipboard_id,&region,&function, &size);
      #if DEBUG
      printf("h_aux vindo do parent->%s\n",h_aux);
      #endif

      data = (void*)malloc(size);

      nbytes = recv(parent_fd,data,size,0);
      if(nbytes ==-1){
        perror(" recv: ");
        exit(-1);
      }
      #if DEBUG
      printf("data vindo do parent->%s\n",(char*)data);
      #endif
      pthread_mutex_lock(&mutex[region]);
      atualiza_data(region,data,size);
      pthread_cond_signal(&cond[region]);
      pthread_mutex_unlock(&mutex[region]);
      envia_para_filhos(data,h_aux);
      free(data);
    }
    if(nbytes==0){/*parent fechou*/
      printf("Parent connection closed.\n");
      parent_fd=-1;
      pthread_exit(NULL);
    }
  }

  return NULL;
}

void get_data(void* cp, char * port){
  parent_fd = socket(AF_INET, SOCK_STREAM,0);
  char buffer[200]="\0";
  struct sockaddr_in parent_addr;
  int mess[10]={0};
  int i,n;

  if(parent_fd==-1){
    perror("socket: ");
    exit(-1);
  }

  if(strcmp(cp,"0")!=0){
    parent_addr.sin_family=AF_INET;
    parent_addr.sin_port=htons(atoi(port));

    #if DEBUG
    printf("o porto é %d\n",atoi(port));
    #endif

    inet_aton((char*) cp, &parent_addr.sin_addr);
    if(connect(parent_fd,(struct sockaddr*)&parent_addr,sizeof(parent_addr))==-1){
      perror("Error connecting: ");
      exit(-1);
    }

    if((n=recv(parent_fd,buffer,200,0))==-1){
      perror("recv: ");
      exit(-1);
    }else{
      if(n==0){
        printf("Parent closed\n");
        return;
      }else{
        sscanf(buffer,"%d;%d;%d;%d;%d;%d;%d;%d;%d;%d",&mess[0],&mess[1],&mess[2],&mess[3],&mess[4],&mess[5],&mess[6],&mess[7],&mess[8],&mess[9]);

        #if DEBUG
        for(i=0;i<10;i++){
          printf("%d\n",mess[i]);
        }
        #endif

        for(i=0;i<10;i++){
          if(mess[i]!=0){
            pthread_mutex_lock(&mutex[i]);
            mensagens[i].mensagem = malloc(mess[i]+1);
            if(recv(parent_fd,mensagens[i].mensagem,mess[i],0)==-1){
              perror("recv: ");
              exit(-1);
            }
            pthread_mutex_unlock(&mutex[i]);
          }
        }
        #if DEBUG
        for(i=0;i<10;i++){
          if(mensagens[i].mensagem!=NULL){
            printf("Região %d -> %s\n",i,(char*)mensagens[i].mensagem);
          }
        }
        #endif
      }
    }
  }
  #if DEBUG
  printf("já recebi tudo do meu parent. o seu fd é %d\n",parent_fd);
  #endif
  return;
}


void * thread_code_clipboard_filhos(void * num){
  #if DEBUG
  printf("Para tratar de uma clipboard filho esotu na thread %ld\n",pthread_self());
  #endif

  int * _aux = num;
  int client_fd = *_aux;
  /*signal(SIGINT, sigintHandler_ctrlc);*/

  ClipB *aux,*ptr;

  int nbytes=-1;

  char h_aux[30];
  void * data=NULL;

  int clipboard_id=-1;
  int region=-1;
  int function=-1;
  size_t size=-1;


  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

  while(1){

    memset((void*)&h_aux, (int)'\0',sizeof(h_aux));
    nbytes = recv(client_fd,h_aux,30,0);

    #if DEBUG2
    printf("filho com o fd %d contactou-me!h_aux bytes:%d\n",client_fd,nbytes);
    #endif

   if(nbytes==0 || nbytes==-1){
      printf("Son connection closed\n");
      for(ptr=filhos;ptr!=NULL;ptr=ptr->next){
        if(ptr->fd==client_fd){
          break;
        }
      }

      close(ptr->fd);
      if(ptr->prev!=NULL){
        ptr->prev->next =ptr->next;
      }else{
        filhos=ptr->next;
      }
      if(ptr->next!=NULL){
        ptr->next->prev =ptr->prev;
      }
      /*close(ptr->fd);*/
      aux=ptr;
      free(aux);
      break;
    }
    if(parent_fd==-1){/*nao tem parent!*/

      sscanf(h_aux,"%d;%d;%d;%ld",&clipboard_id,&region,&function, &size);

      data = (void*)malloc(size);
      nbytes = recv(client_fd,data,size,0);

      pthread_mutex_lock(&mutex[region]);
      atualiza_data(region,data,size);
      pthread_cond_signal(&cond[region]);
      pthread_mutex_unlock(&mutex[region]);
      envia_para_filhos(data,h_aux);

      if(data!=NULL){
        free(data);
      }

    }else{/*tem parent. envia para ele 1o*/
      nbytes=send(parent_fd,h_aux,30,0);
      if(nbytes==-1){
        perror("send: ");
        exit(-1);
      }

      sscanf(h_aux,"%d;%d;%d;%ld",&clipboard_id,&region,&function, &size);

      data = (void*)malloc(size);

      if(nbytes==0){/*parent fechou. precisa de ficar com as mensagens na mesma*/
        printf("Parent connection closed.\n");
        parent_fd=-1;
        nbytes = recv(client_fd,data,size,0);
        if(nbytes==-1){
          perror("recv: ");
          exit(-1);
        }
        pthread_mutex_lock(&mutex[region]);
        atualiza_data(region,data,size);
        pthread_mutex_unlock(&mutex[region]);
        envia_para_filhos(data,h_aux);
        if(data!=NULL){
          free(data);
        }
      }
      if(nbytes>0){
        nbytes = recv(client_fd,data,size,0);
        if(nbytes==-1){
          perror("recv: ");
          exit(-1);
        }
        nbytes = send(parent_fd,data,size,0);
        if(nbytes==-1){
          perror("send: ");
          exit(-1);
        }
      }
    }
  }

  pthread_exit(NULL);
}



void *thread_code_apps(void * num){
  int * aux = num;
  int client_fd = *aux;

  int nbytes=-1;
  void * data=NULL;
  char h_aux[30];

  int clipboard_id=-1;
  int region=-1;
  int function=-1;
  size_t size=-1;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

  while(1){

    memset((void*)&h_aux, (int)'\0',sizeof(h_aux));
    nbytes = recv(client_fd,h_aux,30,0);
    #if DEBUG2
    printf("recebi %s, nbytes:%d da app %d\n",h_aux,nbytes,client_fd);
    #endif

    sscanf(h_aux,"%d;%d;%d;%ld",&clipboard_id,&region,&function, &size);

    if(nbytes==-1){
      perror("recv: ");
      exit(-1);
    }else{
      if(nbytes==0){
        printf("client connection closed\n");
        close(client_fd);
        return 0;
      }
    }

    /*copy*/
    if(function==1){
      data = (void*)malloc(size);
      nbytes = recv(client_fd,data,size,0);
      #if DEBUG
      printf("recebi %d bytes com %s \n",nbytes, (char*)data);
      #endif
      if(nbytes==-1){
        perror("recv: ");
        exit(-1);
      }
      if(parent_fd==-1){/*n tem parent, tem de fazer update e enviar para os filhos*/
        pthread_mutex_lock(&mutex[region]);
        atualiza_data(region,data,size);
        pthread_cond_signal(&cond[region]);
        pthread_mutex_unlock(&mutex[region]);
        envia_para_filhos(data,h_aux);

      }else{/*tem parent, enviar para o parent 1o*/
        nbytes=send(parent_fd,h_aux,30,0);
        #if DEBUG2
        printf("Enviei %s, nbytes:%d para o pai %d\n",h_aux,nbytes,parent_fd);
        #endif
        if(nbytes==-1){
          perror("send: ");
          exit(-1);
        }
        nbytes=send(parent_fd,data,size,0);
        if(nbytes==-1){
          perror("send: ");
          exit(-1);
        }

      }
      if(data!=NULL){
        free(data);
      }

    }
    /*paste*/
    if(function==2){
      pthread_mutex_lock(&mutex[region]);
      if(mensagens[region].mensagem==NULL){/*nothing on that region*/
        #if DEBUG
        printf("Não havia nada em %d\n", region);
        #endif

        nbytes=send(client_fd,"",sizeof(""),0);
        if(nbytes==-1){
          perror("send: ");
          exit(-1);
        }
      }else{
        if(sizeof(mensagens[region].mensagem)==size){/*size asked == size of the contents of the region*/
          nbytes=send(client_fd,mensagens[region].mensagem,size,0);
          if(nbytes==-1){
            perror("send: ");
            exit(-1);
          }
        }else{/*size asked > size of the contents*/
          if(sizeof(mensagens[region].mensagem)<size){
            nbytes=send(client_fd,mensagens[region].mensagem,size,0);
            if(nbytes==-1){
              perror("send: ");
              exit(-1);
            }
          }else{/*size asked < size of the content*/
            void *sub;
            sub=(void*)malloc(size);
            memcpy(sub,mensagens[region].mensagem,size);
            nbytes=send(client_fd,sub,size,0);
            free(sub);
            if(nbytes==-1){
              perror("send: ");
              exit(-1);
            }
          }
        }
      }
      /*pthread_cond_signal(&cond[region]);*/
      pthread_mutex_unlock(&mutex[region]);

    }

    if(function==3){
      #if DEBUG
      printf("Função de Wait\n");
      #endif
      pthread_mutex_lock(&mutex[region]);
      while(1){

      pthread_cond_wait(&cond[region],&mutex[region]);
      #if DEBUG
      printf("FIM da WAIT na região %d\n",region);
      #endif
        if(sizeof(mensagens[region].mensagem)==size){/*size asked == size of the contents of the region*/
          nbytes=send(client_fd,mensagens[region].mensagem,size,0);
          if(nbytes==-1){
            perror("send: ");
            exit(-1);
          }
        }else{/*size asked > size of the contents*/
          if(sizeof(mensagens[region].mensagem)<size){
            nbytes=send(client_fd,mensagens[region].mensagem,size,0);
            if(nbytes==-1){
              perror("send: ");
              exit(-1);
            }
          }else{/*size asked < size of the content*/
            void *sub;
            sub=(void*)malloc(size);
            memcpy(sub,mensagens[region].mensagem,size);
            nbytes=send(client_fd,sub,size,0);
            free(sub);
            if(nbytes==-1){
              perror("send: ");
              exit(-1);
            }
          }
        }
        pthread_mutex_unlock(&mutex[region]);
        break;
      }

    }
  }
  return NULL;
}


void * thread_code_filhos(void * input){
  /*para aceitar novas clipboards*/

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

  int sock_fd =socket(AF_INET,SOCK_STREAM,0);
  struct sockaddr_in local_addr;
  struct sockaddr_in new_addr;
  socklen_t size_addr=sizeof(new_addr);
  int i=0;


  int client_fd;
  char buffer[200]="\0";

  ClipB *aux,*ptr,*last;

  local_addr.sin_family=AF_INET;
  srand(time(NULL));
  int r = rand() % (100 + 1);
  int port = 8000 + r;
  printf("My random port between 8000 and 8100 is %d\n",port);
  local_addr.sin_port=htons(port);
  local_addr.sin_addr.s_addr=INADDR_ANY;

  if(-1==bind(sock_fd, (struct sockaddr *)&local_addr,sizeof(local_addr))){
    perror("bind: ");
    exit(-1);
  }

  listen(sock_fd,10);

  while(1){
    #if DEBUG
    printf("Thread %ld: Waiting for new clipboards\n",pthread_self());
    #endif

    client_fd= accept(sock_fd, (struct sockaddr*)&new_addr, &size_addr);
    if(client_fd==-1){
      perror("Clipboard accept: ");
      exit(-1);
    }

    #if DEBUG
    printf("aceitei uma clipboard, cujo fd é %d\n",client_fd);
    #endif
    if(filhos==NULL){
      filhos=malloc(sizeof(ClipB));
      filhos->fd=client_fd;
      filhos->prev=NULL;
      filhos->next=NULL;
      last=filhos;
    }else{
      ptr=filhos;
      while(ptr->next!=NULL){
        ptr=ptr->next;
      }
      last=ptr;
      aux=malloc(sizeof(ClipB));
      aux->next=NULL;
      aux->fd=client_fd;

      last->next=aux;
      aux->prev=last;
      last=aux;
      aux=NULL;
    }
    memset(buffer,(int)'\0',200);
    sprintf(buffer,"%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld\n",mensagens[0].size,mensagens[1].size,mensagens[2].size,mensagens[3].size,mensagens[4].size,mensagens[5].size,mensagens[6].size,mensagens[7].size,mensagens[8].size,mensagens[9].size);
    #if DEBUG
      printf("vou enviar %s\n",buffer);
    #endif
    send(client_fd,buffer,200,0);

    for(i=0;i<10;i++){
      if(mensagens[i].size !=0){
        pthread_mutex_lock(&mutex[i]);
        if(send(client_fd,mensagens[i].mensagem,mensagens[i].size,0)==-1){
          perror("send: ");
          exit(-1);
        }
        pthread_mutex_unlock(&mutex[i]);
      }
    }

      /*cria thread só para este filho novo*/
    #if DEBUG
    printf("vou criar uma thread para o filho com fd %d\n",client_fd);
    #endif
    pthread_create(&last->thread,NULL,thread_code_clipboard_filhos,&client_fd);
    pthread_detach(last->thread);
    #if DEBUG
    imprime_filhos();
    #endif
  }

  return NULL;
}

void * thread_aceita_apps(void * input){
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

  unlink(SOCK_ADDRESS);/*just in case*/
  struct sockaddr_un local_addr;
  struct sockaddr_un client_addr;
  socklen_t size_addr = sizeof(client_addr);

  int sock_fd= socket(AF_UNIX, SOCK_STREAM,0);
  if(sock_fd==-1){
    perror("socket: ");
    exit(-1);
  }

  local_addr.sun_family = AF_UNIX;
  strcpy(local_addr.sun_path, SOCK_ADDRESS);

  int error = bind(sock_fd, (struct sockaddr *)&local_addr,sizeof(local_addr));
  if(error==1){
    perror("bind: ");
    exit(-1);
  }

  if(listen(sock_fd, 10)==-1){
    perror("listen: ");
    exit(-1);
  }

  int c=0;
  int client_fd=0;
  int i=0;
  for(;i<100;i++){
    thread_ids[i]=-1;
    client_fds[i]=-1;
  }

	while(1){
    #if DEBUG
		printf("Thread %ld: Waiting for new test apps.\n",pthread_self());
    #endif

    client_fd= accept(sock_fd, (struct sockaddr*)&client_addr, &size_addr);
    if(client_fd==-1){
      perror("client accept: ");
      exit(-1);
    }
    client_fds[c]= client_fd;
    /*void * ret_val;*/
    pthread_create(&thread_ids[c],NULL,thread_code_apps, &client_fd);
    pthread_detach(thread_ids[c]);

    c++;

	}
  return NULL;

}

int main(int argc, char * argv[]){

  int i=-1;
  int p=0;

  struct sigaction sa = {0};

  sa.sa_handler = &sigint_flagger;
  sigaction(SIGINT,&sa,NULL);
  signal(SIGPIPE, SIG_IGN);

  for(i=0;i<10;i++){
    mensagens[i].mensagem = NULL;
    mensagens[i].size = 0;
    mensagens[i].cont =0;
  }

  if(argc==1){/*nao tem parent clipboard*/
    pthread_create(&aceita_filhos,NULL,thread_code_filhos,"0");
  }else{/*tem parent clipboard*/
    if(argc==4 && strcmp(argv[1],"-c")==0){
      get_data(argv[2], argv[3]);
      pthread_create(&parent_thread,NULL,thread_recebe_parent,"0");
      p=1;
      #if DEBUG
      printf("parent_thread: %ld\n",parent_thread);
      #endif
      pthread_create(&aceita_filhos,NULL,thread_code_filhos,"0");
    }else{
      printf("Wrong input. Usage: /.clipboard [-c IPaddr]\n");
      exit(-1);
    }
  }

  pthread_create(&aceita_apps,NULL,thread_aceita_apps,"0");
  if(p==1)
  pthread_join(parent_thread,NULL);
  pthread_join(aceita_apps, NULL);
  pthread_join(aceita_filhos, NULL);


	 exit(0);

}
