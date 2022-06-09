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

typedef struct _msg {
	long msg_type;
	struct sockaddr_in addr;
	char msg_text[MAX_BUFSZ];
} msg_t;
struct sockaddr_in servaddr;
struct sockaddr_in cli_addr;
int sock;
int msqid;
pthread_t tid[6];

msg_t pmsg;
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
	key_t msqkey;

	if(argc != 3) {
		printf("Usage: %s msgkey port\n", argv[0]);
		exit(1);
	}
	msqkey = atoi(argv[1]);
	port = atoi(argv[2]);

	// 메시지큐 생성
	if((msqid = msgget(msqkey, IPC_CREAT | 0660)) < 0)
		errquit("msgsnd fail");	

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

	if((status=pthread_create(&tid[5], NULL, echo_resp, NULL)) != 0)
		thr_errquit("pthread_create", status);
	for(i=0;i<6;i++)
		pthread_join(tid[i], NULL);
	
	// 메시지 큐 삭제
	msgctl(msqid, IPC_RMID, 0);
	return 0;
} // main

// 에코 수신 스레드
void *echo_recv(void *arg) {
	int nbytes, status, len = sizeof(struct sockaddr);
	int size;

	printf("recv_thread(%d)\n", pthread_self());
	pmsg.msg_type = pthread_self();
	// pmsg.msg_type = getpid();
	while(1) {
		// 에코 메시지 수신
		nbytes = recvfrom(sock, pmsg.msg_text, MAX_BUFSZ, 0,
				(struct sockaddr *)&cli_addr, &len);
		if(nbytes < 0)
			thr_errquit("recvfrom fail", errno);
		pmsg.msg_text[nbytes] = 0;
		size = strlen(pmsg.msg_text);
		printf("recv thread = %ld : 수신:%s\n",pthread_self(), pmsg.msg_text);
		// 메시지 큐로 전송
		// msgsnd(msqid, msgp, msgsz, msgflg)
		// msgflg(0): 메시지 큐 공간 부족하면 블럭
		// msgflg(IPC_NOWAIT): 부족하면 EAGAIN 에러코드와 -1 리턴
		if(msgsnd(msqid, &pmsg, size, 0) == -1)
			thr_errquit("msgsnd fail", errno);
        if(strstr(pmsg.msg_text, "exit")!=NULL){
            printf("exit program    \n");
            msgctl(msqid, IPC_RMID, 0);
            close(sock);
            exit(0);
        }
	} // while
} // echo_recv

// 에코 송신 스레드
void *echo_resp(void *arg) 
{
	int nbytes, len=sizeof(struct sockaddr);
	int size;
    msg_t rmsg;

	printf("resp_thread(%ld)\n",pthread_self());
	size = sizeof(pmsg) - sizeof(long);
	// 타입구분없이 메시지큐 입력 순서대로 읽음
	pmsg.msg_type = 0; 
	// pmsg.msg_type = getpid();

	while(1) {
		printf("msqid = %d\n", msqid);
		// 타입 구분없이 메시지큐에서 읽음
		if(msgrcv(msqid, (void *)&pmsg, size, 0, 0) < 0) {
		//if(msgrcv(msqid, (void *)&pmsg, MAX_BUFSZ, pmsg.msg_type, 0) < 0) {
			thr_errquit("msgrcv fail", errno);
			exit(0);
		}
		
        printf("송신할 메세지:   %s \n", pmsg.msg_text);
		// 에코 응답
		nbytes = sendto(sock, pmsg.msg_text, strlen(pmsg.msg_text), 
				0, (struct sockaddr *)&cli_addr, len);
		if(nbytes < 0)
			thr_errquit("sendto fail", errno);

		printf("response thread = %ld\n", pthread_self());
		pmsg.msg_text[0] = 0;

        
	}

}
