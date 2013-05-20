#ifndef _EMQ_SOCKET_INCLUDE
#define _EMQ_SOCKET_INCLUDE

#include "emq_nio.h"

static int socketBind(int sock, const char *bind_ipaddr, const int port);

extern int socketListenByAddrAndPort(const char *bind_ipaddr, const int port, int *err_no);
extern int socketConnectToOther(char *ip_addr, int port);
extern int socketReadN(int fd, char * ptr, int nbytes);
extern int socketWriteN(int fd, char * ptr, int nbytes);
extern int socketRecvDataFromConn(EmqNioConn *the_conn, char *buff, int nbytes);
extern int socketSendDataToConn(EmqNioConn *the_conn, char *buff, int nbytes);
extern int socketServer(const char *bind_ipaddr, const int port, int *err_no);
extern void socketAcceptLoop(int sock);
extern int socketGetIpAddr(int socks,char *ipstr, int *port);

#endif
