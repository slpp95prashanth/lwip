#include <stdio.h>
#include <error.h>
#include <lwipopts.h>
#include "lwip/tcp.h"

/* This is the data for the actual web page.
Most compilers would place this in ROM. */
const static char indexdata[] =
"HTTP/1.1 200 OK\r\n\
Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n\
Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\n\
Content-Length: 105\r\n\
Content-Type: text/html\r\n\
Connection: Closed\r\n\
\r\n\
<html>\r\n\
<head><title>A test page</title></head>\r\n\
<body>\r\n\
This is a small test page.\r\n \
</body>\r\n \
</html>\r\n";

/**
 * Data has been received on this pcb.
 */
err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  err_t err_send;

  int i;

  char *data;

  if (p == NULL) {
    return ERR_OK;
  }

  data = p->payload;

  printf("%s: Packet received\n", __func__);

  if (p != NULL){
    /* Inform TCP that we have taken the data. */
    tcp_recved(pcb, p->tot_len);
   }
  
  if ((err != ERR_OK) || (p == NULL)){
    /* error or closed by other side */
    if (p != NULL) {
      pbuf_free(p);
   }
   echo_close_conn(pcb);
   return ERR_OK;
  }

  for (i = 0; i < p->len; i++) {
    diag_printf("%c ", data[(i)]);
  }

  tcp_write(pcb, indexdata, sizeof(indexdata), (TCP_WRITE_FLAG_COPY));

//  err_send = echo_send_data(pcb, p);

  return err_send;
}

/* This is the callback function that is called when
a connection has been accepted. */
static int
http_accept(void *arg, struct tcp_pcb *pcb)
{
    /* Set up the function http_recv() to be called when data
       arrives. */
//    tcp_accepted(((struct tcp_pcb_listen*)arg));

    tcp_recv(pcb, http_recv);

    return ERR_OK;
}

/* The initialization function. */
void http_init(void)
{
    struct tcp_pcb *pcb;
    /* Create a new TCP PCB. */
    pcb = tcp_new();
    /* Bind the PCB to TCP port 80. */
    tcp_bind(pcb, NULL, 80);
    /* Change TCP state to LISTEN. */
    pcb = tcp_listen(pcb);
    /* Set up http_accet() function to be called
       when a new connection arrives. */
    tcp_accept(pcb, http_accept);
}

