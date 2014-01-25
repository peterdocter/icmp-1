#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

#include <netdb.h>
#include <netinet/in.h>
//commit test

#include <arpa/inet.h>


#include <ctype.h>
#include <errno.h>

#include "ip.h"
#include "icmp.h"


#define PORT       "8000"
#define BUF_SIZE   1024

///////////////////////global structs

struct sockaddr_storage peer_addr;
socklen_t               peer_addrlen;
struct sockaddr_in      dstaddr;  //destination addrees
//time structures
struct timeval          sent;
struct timeval          end;
struct timeval          timeout;
//buffers
char                    buff[BUF_SIZE];//buffer for icmp message storage
char                    ipbuff[INET_ADDRSTRLEN];    //buffer for ip string

int     skt;  //file descriptor

/////////////////////////////////////////////////////FUNCTIONS
/*calculate time differance; returns int differance*/
float timeDifferance(struct timeval * s, struct timeval * r){
    float rtt = 0;
    rtt =  (r->tv_usec - s->tv_usec) / 100;
    rtt += (r->tv_sec - s->tv_sec) * 10000;

    return rtt;
}
/*calculate checksum; returs unsigned int 16 bits sum*/
uint16_t sum(uint16_t initial, void* buffer, int bytes){
    uint32_t    total;
    uint16_t *  ptr;
    int         words;

    total   = initial;  //allows update previously calculated sun
    ptr     = (uint16_t *) buffer;
    words   = (bytes+1)/2;  //+1&truncation on/handles any od bytes



    /*
     * the sum is stored in a 32 but int, but the sum its self is 16
     * bit, this means we can store any carries in the top and add
     * them in later.
     */
    while(words--)total += *ptr++;

    /*
     * add in carries, which can result in more carries so the loop
     * is used
     */
    while(total & 0xffff0000)total=(total>>16)+(total & 0xffff);

    return (uint16_t) total;
}
/*reads messages, prints addinfo, prints times*/
void traceloop(){
    struct iphdr   * ip;
    struct iphdr   * ret_ip;
    struct icmphdr * icmp;
    struct icmphdr * sicmp;
    struct icmphdr * returned_icmp;
    int              optval;//used as TTL option value
    long             length,rlength;//size of packets, used for checksum

    //buffers
    char     hostbuff[BUF_SIZE];         //buffer for hostname string
    
    //controllers
    int      probeNum, rval;             //just for some loops

    fd_set   rdfds;
    
    int     pid = (getpid() & 0xffff);  //unique ID

    //TTL
    for(optval=1;optval<31;optval++){ 
        //set number
        printf("%d   ",optval);
        //sets of 3 probes
        for(probeNum=0;probeNum<3;){

            //set up ICMP structure
            memset(&buff, 0, sizeof(buff));
            sicmp = (struct icmphdr *) buff;
            sicmp->type   = ICMP_ECHO;  
            sicmp->id     = htons(pid);  
            sicmp->seqNum = htons(optval); 

            //setup select
            FD_CLR(skt, &rdfds);
            FD_SET(skt, &rdfds); 

            //set select timeout
            timeout.tv_sec=1;
            timeout.tv_usec=0;

            //sets the 'TTL' using the 'TTL option' at the 'IP level'
            if(setsockopt(skt, IPPROTO_IP, IP_TTL,
                        (void *)&optval, (socklen_t)sizeof(optval))){
                perror("setsockopt()");
            }




            //get sent time and store in icmp packet    
           gettimeofday (&sent, NULL);
           memcpy((struct timeval*)sicmp->data, &sent , sizeof(sent));///copy time to icmp data, then retreive the time on echo reply!
           //find length
           length=sizeof(struct icmphdr) + sizeof(struct timeval);
           //calculate checksum
           sicmp->checksum=~sum(0,buff,length);
            
            if(sendto(skt, buff, length, 0,
                        (struct sockaddr *) &dstaddr, sizeof(struct sockaddr_in)) <=0){
                perror("sendto()");
                exit(1);
            }


            //wait for packet
            rval = select(skt+1, &rdfds, NULL, NULL,&timeout);

            //if socket has received
            if(rval>0){
                gettimeofday(&end, NULL);
                peer_addrlen = (socklen_t) sizeof(struct sockaddr_storage);

                //prepare buffer and receive packet
                memset(&buff, 0, sizeof(buff));
                memset(&peer_addr, 0, peer_addrlen);
                if(recvfrom(skt, buff, sizeof(buff), 0,
                            (struct sockaddr *)&peer_addr, &peer_addrlen)<=0)
                    continue;


                //find ip header
                ip = (struct iphdr *)buff;

                //check IPv4, checksum, ip protocol is icmp
                if(ip->version!=4 || sum(0, ip, sizeof(buff))!=0xffff || ip->protocol!=1)
                    continue;
                memset(&hostbuff, 0, sizeof(hostbuff)); 
                //get host name 
                getnameinfo((struct sockaddr *) &peer_addr, peer_addrlen, 
                        hostbuff, sizeof(hostbuff),
                        NULL, 0, 
                        NI_NAMEREQD);

                //get IP as string
                getnameinfo((struct sockaddr *) &peer_addr, peer_addrlen, 
                        ipbuff, sizeof(ipbuff),
                        NULL, 0,
                        NI_NUMERICHOST);

                //IF first prober print hostname and ip.
                if(probeNum==0)
                    printf("%s (%s)",(hostbuff[0]==0?ipbuff:hostbuff), ipbuff);//TERNARY OPERATOR

                icmp = (struct icmphdr *)((uint32_t *)ip + ip->hdrlen);
               
                //length of packet - header length 
                rlength = ntohs(ip->length) - ip->hdrlen *4;
                //verify checksum
                if(sum(0, icmp, rlength)!=0xffff)
                    continue;

                if(icmp->type==11 && icmp->code==0){//TIMEOUT
                    //ret_ip = (struct iphdr *)((uint32_t *)ip + ip->hdrlen + 2);
                    returned_icmp = (struct icmphdr *)((uint32_t *)ip + ip->hdrlen + 7/*ret_ip->hdrlen*/); // 64 bits or returned dgram
                    //checks its what we sent
                    if(returned_icmp->type!=8||
                    returned_icmp->code!=0||
                    ntohs(returned_icmp->seqNum)!=optval||
                    ntohs(returned_icmp->id)!=pid)
                        continue;
                        
                    
                    printf("\t%1.3f ms", timeDifferance(&sent, &end));
                    probeNum++;
                    if(probeNum==3)
                        printf("\n");
                }else if(icmp->type==0 && icmp->code==0){//ECHO_REPLY
                    //get time from returned icmp message
                    printf("\t%1.3f ms", timeDifferance((struct timeval *) icmp->data, &end));
                    if(probeNum==2){
                        printf("\n");
                        exit(0);
                    }
                    probeNum++;
                }
            }else if(rval==0){
                printf("\t*");
                if(probeNum==2)
                    printf("\n");
                probeNum++;
            }

            //end selct
        }//end probe

    }//end ttl
    return;
}
/*creats and binds socket, returns skt*/
int createsocket (const char * servname) {
    int                  skt, errno;
    struct sockaddr_in   bindAddr;
    struct addrinfo      hints;  //prefered addr type(connection)
    struct addrinfo  *   list;   //list of addr structs
    struct addrinfo  *   addrptr;//the one i am gonna use

    if(servname == NULL){
        fprintf(stderr, "No servname!\n");
        exit(1);
    }

    /*
     * prefered connection type
     */

    bzero(&hints, sizeof(hints));
    hints.ai_flags                = 0;
    hints.ai_family               = AF_INET;
    hints.ai_socktype             = SOCK_RAW;
    hints.ai_protocol             = IPPROTO_ICMP;                 


    bzero(&bindAddr, sizeof(bindAddr));
    bindAddr.sin_family         = AF_INET;
    bindAddr.sin_port           = 0;
    bindAddr.sin_addr.s_addr    = INADDR_ANY;
    /*get IP*/
    if((errno = getaddrinfo(servname, 0, &hints, &list))<0){
        fprintf(stderr, "addrinfo error, lookup fail:  %s",
                gai_strerror(errno));
        exit(1);
    }

    addrptr=list;
    //start scanning 
    while(addrptr){
        //start
        if((skt = socket(addrptr->ai_family, addrptr->ai_socktype, addrptr->ai_protocol))<0){
            perror("socket()");
            exit(1);
        }
        if(skt > 0)
            if(bind(skt,(struct sockaddr *)&bindAddr, sizeof(struct sockaddr_in)) == 0)
                break;
        close (skt);
        addrptr=addrptr->ai_next;
    }
    //once IP has been found copy addr an set port=0
    if(addrptr != NULL){
        //copy address to destination struct
        memcpy(&dstaddr, addrptr->ai_addr, sizeof(dstaddr));
        dstaddr.sin_port = 0;
    }else{
        fprintf(stderr, "failed to bind()\n");
        exit(1);
    }

    freeaddrinfo(list);

    return skt;
}
/*calls create create socket, prints target name, call traceloop*/
void client (const char * servername) {
    skt = createsocket (servername);
    //find address of target.
    getnameinfo((struct sockaddr *) &dstaddr, sizeof(dstaddr), 
            ipbuff, sizeof(ipbuff),
            NULL, 0,
            NI_NUMERICHOST);
    printf("traceroute to %s (%s) 30 hops max, %ld byte packets \n", servername, ipbuff, (sizeof (struct iphdr) +
                sizeof(struct icmphdr) + sizeof(struct timeval)));
    traceloop();
    return; /*could not reach gateway*/ 
}
/*takes user argument, handles invalid execution, calls client*/
int main (int argc, char * argv[]) {
    printf("\nSNAKE, COME IN, DO YOU READ ME, SNAKE!!!\n\n");
    //stop invalid executions
    if(argc!=2){
        fprintf(stderr, "usage: %s <address>", argv[0]);
        exit(1);
    }

    client(argv[1]);
    fprintf(stderr, "\nGateway unreachable within 30 hops");
    return 0; 
}
