//--------------------------------------
// 파일명 : 멀티스레드 에코 서버
// 컴파일 : gcc -o mth_echoserv mth_echoserv.c -lpthread
// 실행 : mth_echoserv msqkey port
// -------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_BUFSZ 512
struct sockaddr_in servaddr;
int sock;
pthread_t tid[6];

void *echo_recv(void *argv); // 에코 수신 스리드
void *echo_resp(void *arg); // 에코 송신 스레드

void errquit(char *msg)
{
	perror(msg);
	exit(-1);
}

void thr_errquit(char *msg, int errcode)
{
	printf("%s:%s\n",msg, strerror(errcode));
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
//	pthread_t tid[6];
	struct sockaddr_in cliaddr;
	int port, status, i, len = sizeof(struct sockaddr);

	if(argc != 2) {
		printf("Usage: %s port\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

 	// 소켓 생성
 	if( ( sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errquit("sock fail");
	
	bzero(&servaddr, len);
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_family = AF_INET;
	bind(sock, (struct sockaddr *)&servaddr, len);

	// 스레드 생성
	for(i=0;i<5;i++)
		if((status=pthread_create(&tid[i], NULL, echo_recv, NULL)) != 0)
			thr_errquit("pthread_create", status);
	for(i=0;i<5;i++)
		pthread_join(tid[i], NULL);
	
} // main

// 에코 수신 스레드
void *echo_recv(void *arg) {
	int nbytes, status, len = sizeof(struct sockaddr);
	int size;
    char buf[MAX_BUFSZ];
    struct sockaddr_in cli_sock;

	printf("recv_thread(%ld)\n", pthread_self());
	while(1) {
		// 에코 메시지 수신
		nbytes = recvfrom(sock, buf, MAX_BUFSZ, 0,
                (struct sockaddr *)&cli_sock,&len);
		if(nbytes < 0)
			thr_errquit("recvfrom fail", errno);
		buf[nbytes] = 0;
		printf("recv thread = %ld : 수신:%s\n",pthread_self(), buf);

		nbytes = sendto(sock, buf, strlen(buf),	0, 
                (struct sockaddr *)&(cli_sock), len);
		if(nbytes < 0)
			thr_errquit("sendto fail", errno);
        
		buf[0] = 0;
	} // while
} // echo_recv

// 에코 송신 스레드
void *echo_resp(void *arg) 
{
	int nbytes, len=sizeof(struct sockaddr);
    char buf[MAX_BUFSZ];
	printf("resp_thread(%ld)\n",pthread_self());
	// 타입구분없이 메시지큐 입력 순서대로 읽음
	// pmsg.msg_type = getpid();

	while(1) {
		
		// 에코 응답
		nbytes = sendto(sock, buf, strlen(buf),	0, 0, 0);
		if(nbytes < 0)
			thr_errquit("sendto fail", errno);

		printf("response thread = %ld\n", pthread_self());
		buf[0] = 0;
        
	}

}
