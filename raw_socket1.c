/******************************************************************************
 *
 *       Filename:  raw_socket1.c
 *
 *    Description:  发送raw 网卡数据
 *
 *        Version:  1.0
 *        Created:  2021年07月07日 20时27分51秒
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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
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
	char szbuff[20] = "abcdefg123456";
	// 这样网卡会把sebuff中的内容发送出去
	int i32Len = sendto(SockFd, (char *)szbuff, sizeof(szbuff), 0, (const struct sockaddr *)&stTagAddr, sizeof(stTagAddr));
	if(-1 == i32Len)
	{
		printf("error to send data:errno=%d=>%s\n", errno, strerror(errno));
		return -1;
	}
}
