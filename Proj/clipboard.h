#define INBOUND_FIFO "INBOUND_FIFO"
#define OUTBOUND_FIFO "OUTBOUND_FIFO"
/*not needed anymore*/

#define SOCK_ADDRESS "CLIPBOARD_SOCKET"

#define DEBUG 0
#define DEBUG2 0

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h> /*para o isdigit*/
#include <sys/socket.h> /* para as sockets*/
#include <sys/un.h>/*UNIX socket*/
#include <pthread.h>/*threads. compilar com -pthread*/
#include <arpa/inet.h> /*inet sockets*/
#include <netinet/in.h>
#include <sys/select.h>



int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);
