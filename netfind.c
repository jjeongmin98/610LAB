#include<stdio.h>
#include<pcap.h>

int main(int argc, char **argv)
{
        int ret=0, i=0;
        pcap_if_t *alldevs;
        pcap_if_t *d;
        char errbuf[PCAP_ERRBUF_SIZE];

        ret = pcap_findalldevs(&alldevs, errbuf);
        if(ret == -1){
                printf("rcap_findalldevs : %s \n",errbuf);
                printf("\n");
        }
        for(d=alldevs; d; d=d->next){
                printf("%p : %d .%s",d,++i,d->name);
                if(d->description){
                        printf("%d description : %s \n",i,d->description);
                        printf("\n");
                }
        }
        pcap_freealldevs(alldevs);
        return 0;
}
