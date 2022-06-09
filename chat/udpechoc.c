/* 
 * udoechoc.c
 * UDP Echo Client 프로그램
 * gcc -o udpechoc udpechoc.c
 * 사용법 : udpechoc hostname port번호
 * 예 : udpechoc ice.anyang.ac.kr 9999
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXLINE 512

int main(argc, argv)
int	argc;
char	*argv[];
{
	int			i,nbyte,sockfd,servlen;
	struct sockaddr_in	cli_addr, serv_addr;
	struct hostent *hp, *gethostbyname();
	char buf[1024];
	char abuf[20];

	if(argc < 3)
	{
		printf("Usage: udpechoc hostname portnumber\n");
		exit(0);
	}

	/*
	 * 데이터그램 소켓 생성(소켓 패밀리, 소켓 종류, 하위계층 프로토콜)
	 * 새로운 소켓 디스크립터를 리턴(sockfd)
	 */
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("client: can't open datagram socket");
		exit(1);
	}

	printf("SOCKET CREATED sockfd = %d\n",sockfd);

	bzero((char *)&serv_addr, sizeof(serv_addr));

	/*
	 * 서버 정보 구하고, 채우기
	 *   [서버소켓 패밀리, 서버IP주소, 서버프로그램식별자(포트번호)]
	 */

	/* 패밀리 정보를 serv_addr에 저장 */
	serv_addr.sin_family	  = AF_INET;

	/*
	 * 서버 호스트의 IP 주소 및 관련 정보 구하기 
	 * gethostbyname() : name resolver라고 함 
	 * /etc/hosts 파일을 검색하고, 없으면 Name Server에게 요청함
	 */
	hp = gethostbyname(argv[1]);
	if (hp == (struct hostent *) 0) {
		fprintf(stderr, "%s: unknown host\n", argv[1]);
		exit(1);
	}

	/* 구한 서버 호스트 정보를 serv_addr에 저장 */
	bcopy((char *) &serv_addr.sin_addr, 
		(char *) hp->h_addr_list[0], 
		hp->h_length);

	/* 서버프로그램 식별자(포트번호)는 이미 알고 있어야 함 */
	/* 서버 소켓의 포트 값을 serv_addr에 저장 */
	serv_addr.sin_port	  = htons(atoi(argv[2]));

	/* 키보드 입력 대기 : stdin(standadrd input) */
	printf("입력 : ");
	if(fgets(buf,sizeof(buf), stdin) == NULL)
		exit(0);
	nbyte = strlen(buf);

	servlen = sizeof(serv_addr);

	/* 에코 서버로 메시지 전송 */
	/* UDP에게 데이터 전송 요청 */
	if(sendto(sockfd, buf, nbyte, 0,
		  (struct sockaddr *)&serv_addr, 
		  servlen) < 0) 
	{
		printf("Echo Client: data trafer error\n");
		exit(1);
	}

	printf("수신 : ");
	/* UDP에게 데이터 수신 요청 */
	nbyte = recvfrom(sockfd, buf, MAXLINE,0,
		 	 (struct sockaddr *)&serv_addr, 
		 	 &servlen);
	if(nbyte < 0)
	{ 
		perror("read fail");
		exit(0);
	}
	buf[nbyte] = 0;

	/* 수신된 에코 메시지 화면 출력 */
	printf("%s", buf);

	/* 소켓 닫기 */
	close(sockfd);
	printf("CLOSED\n");
	return(0);
}
