#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <pcap.h>

void my_packet_read(pcap_t *pd);
void callback(	u_char *user, const struct pcap_pkthdr *h, const u_char *p);
void prn_tcphdr(struct tcphdr *tcph);

int main(int argc, char **argv){
	pcap_t 	*pd;
	char 	ebuf[PCAP_ERRBUF_SIZE],
			*dev,
			buf1[50],buf2[50];
	int 	snapsize = 68;
	struct in_addr 	netmask,
					network;
	struct bpf_program fcode;

	dev = pcap_lookupdev(ebuf);

	//타임아웃 2초
	pd = pcap_open_live(dev, snapsize, 1, 2000, ebuf);
	if(pd == NULL){
		fprintf(stderr, "pcap_open_live fail: %s", ebuf);
		exit(1);
	}
	
	//디바이스에 해당하는 localnet과 netmask를 얻음
	pcap_lookupnet(dev, &network.s_addr, &netmask.s_addr, ebuf);

	//필터링 규칙 컴파일
	pcap_compile(pd, &fcode, "tcp", 0, netmask.s_addr);
	pcap_setfilter(pd, &fcode); // 디스크립터에 필터링 규칙적용

	inet_ntop(2, &network, buf1, sizeof(buf1));
	inet_ntop(2, &netmask, buf2, sizeof(buf2));
	printf("device= '%s' (network=%s, netmask=%s) \n",
			dev, buf1, buf2);
	puts("capture start... \n");
	my_packet_read(pd);
	pcap_close(pd);
	exit(0);

}

// 패킷을 읽고 callback 함수를 호출
void my_packet_read(pcap_t *pd){
	if(pcap_loop(pd, -1, callback, NULL) < 0){
		fprintf(stderr, "pcap_loop fail: %s\n", pcap_geterr(pd));
		exit(1);
	}
}

// 수신된 패킷에 대한 처리
void callback(u_char *user, const struct pcap_pkthdr *h,
				const u_char *packet){
	struct tcphdr *tcph;
	struct iphdr *iph;
	int iphlen;

	// ip 시작 지점
	iph = (struct iphdr *) (packet + sizeof(struct ethhdr));
	// ip 헤더 길이
	iphlen = iph->ihl * 4; // 헤더길이 * (32bit)4byte 
	// ip헤더 다음부분이 tcp 헤더
	// tcp헤더 위치
	tcph = (struct tcphdr *)(packet + sizeof(struct ethhdr) + iphlen);
	// IP Datagram의 data 부분이 TCP Segment 인지 확인
	if((iph->protocol) == IPPROTO_TCP) {
		prn_tcphdr(tcph); // tcp헤더 출력
		puts("\n==================================\n");
	}
	return;
} // callback()


void prn_tcphdr(struct tcphdr *tcph){
	printf("(th_sport:%d)", ntohs(tcph->source));
	printf("(th_dport:%d)", ntohs(tcph->dest));
	printf("(th_seq:%u)", (unsigned int) ntohl(tcph->seq));
	printf("(th_ack:%u)", (unsigned int) ntohs(tcph->ack_seq));
	// TCP 헤더 길이 tcph->doff * 4
	printf("(th_off:%d)\n", tcph->doff);
	printf("{th_flags [ ");
	if(tcph->fin) printf("FIN ");
	if(tcph->syn) printf("SYN ");
	if(tcph->rst) printf("RST ");
	if(tcph->psh) printf("PSH ");
	if(tcph->ack) printf("ACK ");
	if(tcph->urg) printf("URG ");
	printf("]}");
	printf("(th_window:%d)", ntohs(tcph->window));
	printf("(th_chksum:%d)", ntohs(tcph->check));
	printf("(th_urg_ptr:%d)", ntohs(tcph->urg_ptr));

	fflush(stdout);
} // prn_tcphdr()
