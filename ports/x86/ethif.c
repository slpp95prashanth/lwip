#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <netif/etharp.h>

#include <sys/time.h>
#include <sys/sysinfo.h>

//#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <net/if.h>           // struct ifreq
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.

#include <lwip/debug.h>

#if 0
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <netinet/ip_icmp.h>
#endif

#include <errno.h>            // errno, perror()
#include <stdio.h>
#include <stdarg.h>

#define RX_BUF_SIZE (1024 * 2)
#define TX_BUF_SIZE (1024 * 2)

/* Timer resolution in ms */

#define ARP_TIMER (5000000)
#define TCP_TIMER (250000)
#define IP_REASMY_TIMER (250000)

#define SRC_MAC "c0:3f:d5:4d:46:e7"

#define PRINT_IPADDR(str, ipaddr) printf(#str": %"U16_F".%"U16_F".%"U16_F".%"U16_F" ""\n",	    \
                    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr));

struct _sock {
    int sock;
    struct sockaddr_ll sock_ll;
    struct pbuf *p_tx, *p_rx;
};

static unsigned long long int uptime;

unsigned long long int sys_now()
{
    struct sysinfo s_info;

    int error;

    error = sysinfo(&s_info);

    if(error != 0) {
        LWIP_DEBUGF(DUMP, ("code error = %d\n", error));
	exit(-1);
    }

    return s_info.uptime - uptime;
}

int send_pac(struct netif *netif, char *pac, size_t len)
{
    int ret = 0;
    int i;
    struct _sock *sock = netif->state;

    for (i = 0; i < len; i++) {
	LWIP_DEBUGF(DUMP, ("%02x ", pac[(i)]));

	if (i % 20 == 0) {
	    LWIP_DEBUGF(DUMP, ("\n"));
	}
    }

    ret = sendto(sock->sock, pac, len, 0, (struct sockaddr *)&sock->sock_ll,
			 sizeof(struct sockaddr_ll));
    if (ret <= 0) {
        perror ("sendto() failed");
        return ret;
    }

    LWIP_DEBUGF(DUMP, ("Sending packet Packet size = %p\n", ret));

    return 0;
}

void *timer(void *data)
{
    struct timeval time;

    unsigned int arp_us, tcp_us, ip_reass_us, new_us;

    LWIP_DEBUGF(DUMP, ("%s \n", __func__));

    gettimeofday(&time, NULL);

    arp_us = tcp_us = ip_reass_us = time.tv_sec * 1000000 + time.tv_usec;

    while (1) {
	gettimeofday(&time, NULL);

	new_us = time.tv_sec * 1000000 + time.tv_usec;

	if ((new_us - arp_us) >= (ARP_TIMER)) {
	    etharp_tmr();
	    arp_us = new_us;
	}

	if ((new_us - tcp_us) >= (TCP_TIMER)) {
	    tcp_tmr();
	    tcp_us = new_us;
	}

	if ((new_us - ip_reass_us) >= (IP_REASMY_TIMER)) {
	    ip_reass_tmr();
	    ip_reass_us = new_us;
	}
    }

    return NULL;
}

int eth_output(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
    LWIP_DEBUGF(DUMP, ("%s if=%s\n", __func__, netif->name));

    send_pac(netif, p->payload, p->tot_len);

    return 0;
}

int eth_linkoutput(struct netif *netif, struct pbuf *p)
{
    int i;

    LWIP_DEBUGF(DUMP, ("%s if=%s\n", __func__, netif->name));

    LWIP_DEBUGF(DUMP, ("tot_len = %d, len = %d\n", p->tot_len, p->len));

    for (i = 0; i < p->tot_len; i++) {
	LWIP_DEBUGF(DUMP, ("%02x ", ((char *)(p->payload))[i]));

	if (i % 10 == 0) {
	    LWIP_DEBUGF(DUMP, ("\n"));
	}
    }

    send_pac(netif, p->payload, p->tot_len);

    return 0;
}

int eth_input(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
    LWIP_DEBUGF(DUMP, ("%s if=%s\n", __func__, netif->name));

    return 0;
}

#undef recv
#undef sock
#undef setsockopt
#undef ioctl

int etharp_output(struct netif *, struct pbuf *, ip_addr_t *);

int eth_init(struct netif *netif)
{
    int error;

    struct sysinfo s_info;
    struct ether_addr *mac;

    static struct _sock _sock;

    struct sockaddr_ll *bind_eth = &_sock.sock_ll;

    struct ifreq ifr;

    char buf[1024];

    error = sysinfo(&s_info);

    if(error != 0) {
        LWIP_DEBUGF(DUMP, (("code error = %d\n", error)));
	exit(-1);
    }

    uptime = s_info.uptime;

    netif->output = etharp_output;

    LWIP_DEBUGF(DUMP, ("netif->output = %p\n", netif->output));

    netif->linkoutput = eth_linkoutput;

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->hwaddr_len = 6;

    netif->hwaddr[0] = 0xc0;
    netif->hwaddr[1] = 0x3f;
    netif->hwaddr[2] = 0xd5;
    netif->hwaddr[3] = 0x4d;
    netif->hwaddr[4] = 0x46;
    netif->hwaddr[5] = 0xe7;

    netif->mtu = 1500;

    _sock.p_tx = pbuf_alloc(PBUF_RAW, TX_BUF_SIZE, PBUF_RAM);
    if (_sock.p_tx == NULL) {
	LWIP_DEBUGF(DUMP, ("pbuf_alloc error\n"));
	exit(-1);
    }

    _sock.p_rx = pbuf_alloc(PBUF_RAW, RX_BUF_SIZE, PBUF_RAM);
    if (_sock.p_rx == NULL) {
	LWIP_DEBUGF(DUMP, ("pbuf_alloc error\n"));
	exit(-1);
    }

    _sock.sock = socket( AF_PACKET , SOCK_RAW , 768);

    if (_sock.sock < 0) {
        //Print the error with proper message
        perror("Socket Error");
        return _sock.sock;
    }

    bind_eth->sll_ifindex = if_nametoindex("eth0");
    bind_eth->sll_family = AF_PACKET;

    mac = ether_aton(SRC_MAC);

    memcpy(bind_eth->sll_addr, mac, 6);
    bind_eth->sll_halen = 6;
/*
    error = bind(_sock.sock, (struct sockaddr *)&bind_eth, sizeof(struct sockaddr_ll));
    if (error < 0) {
	perror("error bind");
	return error;
    }
*/
    memset (&ifr, 0, sizeof (ifr));
    snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", "eth0");

    if (ioctl (_sock.sock, SIOCGIFINDEX, &ifr) < 0) {
	perror ("ioctl() failed to find interface ");
    }

    LWIP_DEBUGF(DUMP, ("Index for interface %s is %i\n", "eth0", ifr.ifr_ifindex));

    if (setsockopt (_sock.sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr)) < 0) {
	perror ("setsockopt() failed to bind to interface ");
    }

    netif->state = &_sock;

    return 0;
}

extern err_t ethernet_input(struct pbuf *, struct netif *);

int _main(void)
{
    static struct netif netif;
    struct _sock *_sock;

    unsigned int gw_addr;
    unsigned int net_mask;
    unsigned int ip_addr;
    int i, ret;

    static pthread_t threadid;

    ip_addr = htonl(0xac100915); /* 172.16.9.21 */
    net_mask = htonl(0xffffff00); /* 255.255.255.0 */
    gw_addr = htonl(0xac100901); /* 172.16.9.1 */

//    memset(&gw_addr, '\0', (sizeof(struct ip_addr)));
//    memset(&net_mask, '\0', (sizeof(struct ip_addr)));

#if 0
    memset(&ip_addr, 0x10, (sizeof(struct ip_addr)));

    netif.ip_addr = 0x10101010;
    netif.netmask = 0;
    netif.gw = 0;
    netif.input = eth_input;
    netif.output = etharp_output;
    netif.linkoutput = eth_linkoutput;
#endif

    netif.name[0] = 'e';
    netif.name[1] = 't';
    netif.name[2] = 'h';
    netif.name[3] = '0';
    netif.name[4] = '\0';

    lwip_init();

    netif_add(&netif, &ip_addr, &net_mask, &gw_addr, NULL, eth_init, ethernet_input);

    netif_set_default(&netif);

    netif_set_up(&netif);

    ret = pthread_create(&threadid, NULL, timer, NULL);

    if (ret != 0) {
	perror("pthread_create: ");
	exit(-1);
    }

    _sock = netif.state;

    while (1) {
	_sock->p_rx->len = recv(_sock->sock, _sock->p_rx->payload, RX_BUF_SIZE, 0);

	LWIP_DEBUGF(DUMP, ("_sock->p_rx->len = %d\n", _sock->p_rx->len));

	if (_sock->p_rx->len > 0) {
	    _sock->p_rx->tot_len = _sock->p_rx->len;

	    for (i = 0;i < _sock->p_rx->len; i++) {
		LWIP_DEBUGF(DUMP, ("%02x ", ((char *)(_sock->p_rx->payload))[i]));

		if (i % 10 == 0) {
		    LWIP_DEBUGF(DUMP, ("\n"));
		}
	    }
	}

	_sock->p_rx->ref = 1;

	(*netif.input)(_sock->p_rx, &netif);

	_sock->p_rx = pbuf_alloc(PBUF_RAW, RX_BUF_SIZE, PBUF_RAM);
	if (_sock->p_rx == NULL) {
	    LWIP_DEBUGF(DUMP, ("pbuf_alloc error\n"));
	    exit(-1);
	}
    }

    return 0;
}

int dump_ifstats(char *name)
{
    struct netif *netif = netif_list;

    ip_addr_t *ipaddr;
    struct eth_addr *ethaddr;

    while (netif) {
	ipaddr = &netif->ip_addr;
	ethaddr = netif->hwaddr;

	printf("if: %s\n", netif->name);
	printf("ipaddr: %"U16_F".%"U16_F".%"U16_F".%"U16_F" ""ethaddr:"" %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
		    ip4_addr1_16(ipaddr), ip4_addr2_16(ipaddr), ip4_addr3_16(ipaddr), ip4_addr4_16(ipaddr),
		    ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
		    ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]);

	PRINT_IPADDR(netmask, &netif->netmask);
	PRINT_IPADDR(gw, &netif->gw);

	printf("MTU: %u\n", netif->mtu);
	netif = netif->next;
    }

    return 0;
}

int dump_netstat(void)
{
    struct netif *netif = netif_list;

    while (netif) {
	printf("if: %s\n", netif->name);

	printf("TCP: %u, UDP: %u, ICMP: %u\n", netif->ifintcp, netif->ifinudp, netif->ifinicmp);

	netif = netif->next;

	tcp_debug_print_pcbs();

	udp_pcb_dump();

    }

    return 0;
}
