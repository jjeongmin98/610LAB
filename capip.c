#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <arpa/inet.h>

void my_packet_read(pcap_t *pd);
void callback(	u_char *user, const struct pcap_pkthdr *h, const u_char *p);
void prn_iphdr(struct iphdr *hdr);

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
	pcap_compile(pd, &fcode, "ip", 0, netmask.s_addr);
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
	struct iphdr *iph;
	struct ethhdr *eh;
	int ethlen;

	// 이더넷 시작 지점
	eh = (struct ethhdr *) packet;
	// 이더넷 헤더 길이
	ethlen = sizeof(struct ethhdr);
	// 이더넷 프레임 헤더의 다음 부력분이 ip헤더
	// ip헤더 위치
	iph = (struct iphdr *)(packet + ethlen);
	// 이더넷 데이터 부분이 IP datagram 인지 확인
	if(ntohs(eh->h_proto) == ETHERTYPE_IP) {
		prn_iphdr(iph); // ip헤더 출
		puts("\n==================================\n");
	}
	return;
} // callback()


void prn_iphdr(struct iphdr *iph){
	char buf[20];
	int fragoff;
	int DF; // don't fragment

	printf("(ver:%d)", iph->version);
	printf("(hlen:%d)", iph->ihl*4);
	printf("(tos:%d)", iph->tos);
	printf("(tlen:%d)", ntohs(iph->tot_len));
	printf("(id:%d)", ntohs(iph->id));

	DF = ntohs(iph->frag_off) >> 13; // 앞의 3비트만 유효
	fragoff = ntohs(iph->frag_off) & 0x1FFF; // 뒤의 13비트만 유효
	if(DF == 1) printf("More Fragment, DF = %d, offset:%d)",DF,fragoff);
	if(DF == 2) printf("Don't Fragment, DF = %d)",DF);
	printf("ttl:%d)\n", iph->ttl);
	printf("(proto:0x%x)", iph->protocol);
	printf("(checksum:0x%x)", ntohs(iph->check));

	if(inet_ntop(AF_INET, &iph->saddr, buf, sizeof(buf)) < 0) {
		perror("inet_ntop fail");
		exit(1);
	}
	printf("(saddr:%s)", buf);
	if(inet_ntop(AF_INET, &iph->daddr, buf, sizeof(buf)) < 0 ) {
		perror("inet_ntop fail");
		exit(1);
	}
	printf("(daddr:%s)", buf);
	fflush(stdout);
} // prn_iphder()
