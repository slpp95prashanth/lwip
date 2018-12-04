CC=gcc
CCDEP=$(CC)

CFLAGS += -DLWIP_DEBUG -g
ARFLAGS=rs

TOPDIR=./
TARGETDIR=$(TOPDIR)/tmplib
CONTRIBDIR=$(TOPDIR)/lwip/contrib   
LWIPARCH=$(TOPDIR)/ports/x86

#Set this to where you have the lwip core module checked out from CVS
#default assumes it's a dir named lwip at the same level as the contrib module
LWIPDIR=$(TOPDIR)/src
MCHINCDIR=$(TOPDIR)/include

CFLAGS += $(CPPFLAGS) -I$(LWIPDIR)/include -I.              \
    -I$(LWIPDIR)/include/ipv4 -I $(LWIPARCH)/include

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES=$(LWIPDIR)/core/mem.c             \
    $(LWIPDIR)/core/memp.c              \
    $(LWIPDIR)/core/netif.c             \
    $(LWIPDIR)/core/pbuf.c              \
    $(LWIPDIR)/core/raw.c               \
    $(LWIPDIR)/core/stats.c             \
    $(LWIPDIR)/core/sys.c               \
    $(LWIPDIR)/core/tcp.c               \
    $(LWIPDIR)/core/tcp_in.c            \
    $(LWIPDIR)/core/tcp_out.c           \
    $(LWIPDIR)/core/udp.c               \
    $(LWIPDIR)/core/dhcp.c              \
    $(LWIPDIR)/core/init.c		\
    $(LWIPDIR)/core/timers.c		\
    $(LWIPDIR)/core/def.c

CORE4FILES=$(LWIPDIR)/core/ipv4/icmp.c          \
    $(LWIPDIR)/core/ipv4/ip.c           \
    $(LWIPDIR)/core/ipv4/inet.c         \
    $(LWIPDIR)/core/ipv4/ip_addr.c          \
    $(LWIPDIR)/core/ipv4/ip_frag.c          \
    $(LWIPDIR)/core/ipv4/inet_chksum.c	    \
    $(LWIPDIR)/netif/etharp.c		    \
    $(LWIPDIR)/netif/ethernetif.c

# NETIFFILES: Files implementing various generic network interface functions.'
NETIFFILES=$(LWIPARCH)/ethif.c

# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(CORE4FILES) $(NETIFFILES)
#OBJS=$(notdir $(LWIPFILES:.c=.o))
OBJS=$(LWIPFILES:.c=.o)

LWIPLIB=liblwip4.a

all compile: $(LWIPLIB)
	mkdir -p $(TARGETDIR)
	install $(LWIPLIB) $(TARGETDIR)

.PHONY: all depend compile clean

%.o:
	$(CC) $(CFLAGS) -c $(@:.o=.c) -o $@

clean:
	rm -f *.o $(LWIPLIB) $(OBJS) .depend*

$(LWIPLIB): $(OBJS)
	$(AR) $(ARFLAGS) $(LWIPLIB) $?

depend dep: .depend

include .depend

.depend: $(LWIPFILES)
	$(CCDEP) $(CFLAGS) -MM $^ > .depend || rm -f .depend
