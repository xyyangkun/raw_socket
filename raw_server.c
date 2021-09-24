/******************************************************************************
 *
 *       Filename:  raw_server.c
 *
 *    Description:  接收raw socket 数据包
 *
 *        Version:  1.0
 *        Created:  2021年07月26日 19时24分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <net/ethernet.h> /* the L2 protocols */// ETH_P_ALL
#include <linux/if_packet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/ip.h> // ip head
#include <netinet/udp.h> // udp head
#include <signal.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int running = 1;

void sigint(int signum)
{
	if (!running){
		exit(EXIT_FAILURE);
	}
	running = 0;
}

void sigalarm(int signum)
{
	if (!running){
		exit(EXIT_FAILURE);
	}
	running = 0;
}
#define PACKET_SIZE 1000
#ifndef ETH_P_WSMP
	#define ETH_P_WSMP	0x88DC
#endif
void print_string_hex(char *buf, unsigned short length)
{
	int i =0;
	printf("\n\t\t");
	for( i = 0; i < length; i++){
		printf(" %02X", (buf[i]&0xFF));
		if( (i + 1) % 16 == 0 )
		{
			printf("\n\t\t");
		}
	}
	printf("\n\n");
}

// 校验和函数
unsigned short csum(unsigned short *buf, int nwords)
{
	unsigned long sum;
	for(sum=0; nwords>0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned short)(~sum);
}

// 发送mac层数据
int main()
{
	struct sockaddr_ll stTagAddr;
	memset(&stTagAddr, 0 , sizeof(stTagAddr));
	stTagAddr.sll_family    = AF_PACKET;//填写AF_PACKET,不再经协议层处理
	stTagAddr.sll_protocol  = htons(ETH_P_ALL);

	int ret;
	struct ifreq req;
	int sd;
	sd = socket(PF_INET, SOCK_DGRAM, 0);//这个sd就是用来获取eth0的index，完了就关闭
	if(sd == -1)
	{
		printf("error to create socket pf_inet:%s\n", strerror(errno));
		return -1;
	}

	// todo 此处需要更改为你使用的网卡名称
	char *eth_name = "enp3s0";
	strncpy(req.ifr_name, eth_name, strlen(eth_name));//通过设备名称获取index
	ret=ioctl(sd, SIOCGIFINDEX, &req);
	if(ret <0)
	{
		perror("SIOCGIFHWADDR");
		return -1;
	}
	
	// 获取mac地址
	struct ifreq if_mac;
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, eth_name, strlen(eth_name));
	if (ioctl(sd, SIOCGIFHWADDR, &if_mac) < 0)
	{
		perror("SIOCGIFHWADDR");
		return -1;
	}


	close(sd);
	if (ret==-1)
	{
		   printf("Get eth0 index err:%d=>%s \n", errno, strerror(errno));
		   return -1;
	}

	// int SockFd = socket(PF_PACKET, SOCK_RAW, htons(VSTRONG_PROTOCOL));
	int SockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (-1 == SockFd)
	{
		  printf("create socket error:%d.\n",errno); 
		  return -11;
	}

	struct sockaddr_ll s_addr;
	/* prepare sockaddr_ll */
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sll_family   = AF_PACKET;
	s_addr.sll_protocol = htons(ETH_P_WSMP);
	s_addr.sll_ifindex  = req.ifr_ifindex;

	/* bind to interface */
	if (bind(SockFd, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1)
	{
		perror("error to bind:");
		exit(EXIT_FAILURE);
	}

	//处理信号
	/* enable signal */
	signal(SIGINT, sigint);
	signal(SIGALRM, sigalarm);

	stTagAddr.sll_ifindex   = req.ifr_ifindex;//网卡eth0的index，非常重要，系统把数据往哪张网卡上发，就靠这个标识
	stTagAddr.sll_pkttype   = PACKET_OUTGOING;//标识包的类型为发出去的包
	stTagAddr.sll_halen     = 6;    //目标MAC地址长度为6
	//填写目标MAC地址
	stTagAddr.sll_addr[0]   = 0x2c;
	stTagAddr.sll_addr[1]   = 0xf0;
	stTagAddr.sll_addr[2]   = 0x5d;
	stTagAddr.sll_addr[3]   = 0x6e;
	stTagAddr.sll_addr[4]   = 0x3e;
	stTagAddr.sll_addr[5]   = 0xa6;
	//填充帧头和内容
	// 构造ethernet header
	int tx_len = 0;
	char sendbuf[1024];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	memset(sendbuf, 0, 1024);
	/* Ethernet header */
	// 此处应该根据源mac 和 目的mac做更改
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = 0x2c;
	eh->ether_dhost[1] = 0xf0;
	eh->ether_dhost[2] = 0x5d;
	eh->ether_dhost[3] = 0x6e;
	eh->ether_dhost[4] = 0x3e;
	eh->ether_dhost[5] = 0xa6;
	eh->ether_type = htons(ETH_P_IP);
	tx_len += sizeof(struct ether_header);

	// 构造ip 头
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	int ttl = 20;
	/* IP Header */
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 16; // Low delay
	iph->id = htons(54321);
	iph->ttl = ttl; // hops
	iph->protocol = 17; // UDP
	/* Source IP address, can be spoofed */
	//iph->saddr = inet_addr(inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));
	// 此处要更改源ip和目的ip
	iph->saddr = inet_addr("172.20.2.7");
	/* Destination IP address */
	iph->daddr = inet_addr("172.20.2.76");
	tx_len += sizeof(struct iphdr);
	
	// 构建udp 数据头
	struct udphdr *udph = (struct udphdr *) (sendbuf + sizeof(struct iphdr) + sizeof(struct ether_header));
	/* UDP Header */
	// 源端口, 目的端口
	udph->source = htons(3423);
	udph->dest = htons(5342);
	udph->check = 0; // skip
	tx_len += sizeof(struct udphdr);

	//udp 数据
	/* Packet data */
	sendbuf[tx_len++] = 0xde;
	sendbuf[tx_len++] = 0xad;
	sendbuf[tx_len++] = 0xbe;
	sendbuf[tx_len++] = 0xef;

	// 填充头校验信息
	/* Length of UDP payload and header */
	udph->len = htons(tx_len - sizeof(struct ether_header) - sizeof(struct iphdr));
	/* Length of IP payload and header */
	iph->tot_len = htons(tx_len - sizeof(struct ether_header));
	/* Calculate IP checksum on completed header */
	iph->check = csum((unsigned short *)(sendbuf+sizeof(struct ether_header)), sizeof(struct iphdr)/2);

#if 0
	// 这样网卡会把sebuff中的内容发送出去
	int i32Len = sendto(SockFd, (char *)sendbuf, tx_len, 0, (const struct sockaddr *)&stTagAddr, sizeof(stTagAddr));
	if(-1 == i32Len)
	{
		printf("error to send data:errno=%d=>%s\n", errno, strerror(errno));
		return -1;
	}
#endif


#if 1
	int  recv = 0;
	char buff[PACKET_SIZE];
	printf("wait recv data\n");
	while(1)
	{
		recv = recvfrom(SockFd, buff, PACKET_SIZE, 0, NULL, NULL);
		printf("recv :%d \n", recv);
		if (recv <= 0){
			printf("recv recv:%d %s\n", recv, strerror(errno));
			continue;
		}else{
			print_string_hex(buff, recv);       
		}
	}

#endif

	close(SockFd);

}
