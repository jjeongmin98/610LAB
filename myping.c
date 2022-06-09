// ##myping.c
// root 권한으로 ./컴파일명 ip주소

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<netinet/in.h>

#define BUFSIZE 4096

int seqnum;					//ping 메시지 일련번호
char recvbuf[BUFSIZE];		//수신버퍼
char sendbuf[BUFSIZE];		//송신버퍼
int rawsock;				//Raw 소켓 번호
int notrecv =0;				//ping 응답을 
struct sockaddr_in sendaddr, recvaddr;

int send_ping();			//ping request
int prn_rcvping(char *ipdata, int recvsize); //ping 응답에 출력
void prn_icmp(struct icmphdr *icmp, int icmpsize); //ICMP 헤더 출력
unsigned short in_cksum(unsigned short *addr, int len); //ICMP Check sum
void errquit(char *msg) { perror(msg); exit(0); }

int main(int argc, char **argv){
	int recvsize, addrlen = sizeof(struct sockaddr);
	fd_set readset;
	struct timeval tv;
	int ret;

	if(argc != 2) {
		printf("Usage : %s ip_address \n", argv[0]);
		exit(1);
	}

	addrlen = sizeof(struct sockaddr);
	bzero(&recvaddr, sizeof(struct sockaddr));
	bzero(&sendaddr, sizeof(struct sockaddr));

	sendaddr.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &sendaddr.sin_addr.s_addr);
	sendaddr.sin_port = htons(0);

	// raw 소켓 생성

	if((rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		errquit("socket fail\n root 권한으로 실행 안되었을경우 실패할수도 있음\n");

	// 첫번째 ping 보내기
	while(1) {
		FD_ZERO(&readset);
		FD_SET(rawsock, &readset);
		tv.tv_sec = 1; //1초 타이머
		tv.tv_usec = 0;

		send_ping(); // 핑 보내기
		ret = select(rawsock + 1, &readset, NULL, NULL, &tv); //타이머 가동
		if(ret == 0){ //타임 아웃
			if(++notrecv == 3){
				notrecv = 0;
				puts("요청 타임아웃...\n");
			}

			continue;
		}
		else if(ret < 0 ) // select()의 에러
			errquit("select fail\n");

		// select()의 정상리턴, ping 응답을 읽음
		recvsize = recvfrom(rawsock, recvbuf, sizeof(recvbuf),0,
				(struct sockaddr*)&recvaddr, &addrlen);
		if( recvsize < 0 )
			errquit("recvfrom fail\n");
		notrecv = 0; // 수신된 응답에 대한 처리
		prn_rcvping(recvbuf, recvsize);
		sleep(1); // 1초 간격으로 ping을 보냄

	}
	exit(0);
}

void prn_icmp(struct icmphdr *icmp, int icmpsize){
	printf("[icmp] (id: %d ", icmp->un.echo.id);
	printf("seq: %d "		, icmp->un.echo.sequence);
	printf("code: %d "		, icmp->code);
	printf("type: %d )\n"	, icmp->type);
}

//수신된 메세지를 출력
int prn_rcvping(char *ipdata, int recvsize){
	int ip_headlen, icmp_len;
	struct icmphdr *icmp;
	struct iphdr *ip;
	char buf[512];

	ip = (struct iphdr *)ipdata;
	ip_headlen = ip-> ihl * 4;
	icmp_len = recvsize - ip_headlen;
	icmp = (struct icmphdr *) (ipdata + ip_headlen);
	if(icmp-> type != ICMP_ECHOREPLY)
		return -1;
	inet_ntop(AF_INET, (void *)&ip->saddr, buf, sizeof(buf));
	printf("%d bytes recv from (%s) ", icmp_len, buf);
	prn_icmp(icmp, icmp_len);
	return 0;
}

//핑 요청 보내기
int send_ping(){
	struct icmphdr *icmp;
	int len, sendsize;
	icmp = (struct icmphdr *) sendbuf;
	bzero((char *)icmp, sizeof(struct icmp));
	icmp-> code = 0;
	icmp-> type = ICMP_ECHO; // ICMP_ECHO = 8
	icmp-> un.echo.sequence = seqnum++;
	icmp-> un.echo.id = getpid();
	icmp-> checksum = 0;
	icmp-> checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmp));
	len = sizeof(struct icmphdr); // 8byte
	sendsize = sendto(rawsock, sendbuf, len, 0,
			(struct sockaddr *)&sendaddr, sizeof(struct sockaddr));
	prn_icmp(icmp, sendsize); // ICMP 헤더 출력
	return sendsize;
}

//checksum 구하기
unsigned short in_cksum(unsigned short *addr, int len){
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;

	while(nleft > 1){
		sum += *w++; nleft -= 2;
	}
	if(nleft == -1){
		*(unsigned char *) (&answer) = *(unsigned char *)w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	
	return (answer);
}
