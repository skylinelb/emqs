#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socketvar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "emq_log.h"
#include "emq_global.h"
#include "emq_nio.h"
#include "emq_socket.h"

int socketBind(int sock, const char *bind_ipaddr, const int port)
{
    struct sockaddr_in bindaddr;

    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(port);
    if (bind_ipaddr == NULL || *bind_ipaddr == '\0')
    {
        bindaddr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_aton(bind_ipaddr, &bindaddr.sin_addr) == 0)
        {
            applog(EMERG, "invalid ip addr %s", bind_ipaddr);
            return EINVAL;
        }
    }

    if (bind(sock, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) < 0)
    {
/*
        applog(EMERG, "bind port %d failed errno: %d, error info: %s.", \
            port, errno, STRERROR(errno));
*/
        return errno != 0 ? errno : ENOMEM;
    }

    return 0;
}

int socketListenByAddrAndPort(const char *bind_ipaddr, const int port, int *err_no)
{
    int sock;
    int result;
	
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
	*err_no = errno != 0 ? errno : EMFILE;
/*
	applog(EMERG, "socket create failed, errno: %d, error info: %s", \
		errno, STRERROR(errno));
*/
        return -1;
    }

    result = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &result, sizeof(int))<0)
    {
        *err_no = errno != 0 ? errno : ENOMEM;
/*
	applog(EMERG, "setsockopt failed, errno: %d, error info: %s", \
		errno, STRERROR(errno));
*/
         close(sock);
         return -2;
    }

    if ((*err_no=socketBind(sock, bind_ipaddr, port)) != 0)
    {
        close(sock);
        return -3;
    }
	
    if (listen(sock, 1024) < 0)
    {
	*err_no = errno != 0 ? errno : EINVAL;

/*
	applog(EMERG, "listen port %d failed, errno: %d, error info: %s", \
        	port, errno, STRERROR(errno));
*/
         close(sock);
         return -4;
     }

    *err_no = 0;
    return sock;
}

/*
 * connect to other
 */
int socketConnectToOther(char *ip_addr, int port)
{
    int                sockfd;
    struct sockaddr_in svr_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        applog(EMERG,"tcp create socket error.");
        return -1;
    }

    memset((char *)&svr_addr, 0, sizeof(svr_addr));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = inet_addr(ip_addr);
    if(svr_addr.sin_addr.s_addr < 0) {
        applog(EMERG, "ip addr %s is invalid", ip_addr);
        return -1;
    }

    svr_addr.sin_port = htons(port);

    if ((connect(sockfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr))) < 0) {
        applog(EMERG, "tcp connect to [%s][%d] error.", ip_addr , port);
        close(sockfd);
        return -1;
    }

    return(sockfd);
}

int socketReadN(int fd, char * ptr, int nbytes)
{
    int nleft, nread;
    int tselect;

    nread = 0;
    nleft = nbytes;

    while (nleft > 0) {
        nread = recv(fd, ptr, nleft, 0);
/*
applog(INFO, "skyline test nread %d", nread);
*/
        if (nread < 0) {
            applog( EMERG,"tcp recv data error" );
            return (nread);
        }
        else if (nread == 0)
            return 0;                 /* connection close */
        else if( nread==nleft )
            break;

/* 暂时不使用
       if (( tselect=TcpSelectSocket(30,fd)) < 0){
           ERRLOG( 100502,"tcp select error when read data." );
           return -1;
       }
       if( tselect == 0 ){
           ERRLOG( 100502,"tcp read data timeout." );
           return (nbytes-nleft);
       }
*/

        nleft -= nread;
        ptr   += nread;
    }
    nleft -= nread;
    return (nbytes - nleft);
}

int socketWriteN(int fd, char * ptr, int nbytes)
{
    int    nleft, nwritten;

    nleft = nbytes;

applog(INFO, "skyline test wriete fd %d", fd);
    while (nleft > 0) {
        nwritten = send (fd, ptr, nleft, 0);
        if (nwritten < 0)
            return (nwritten);             /* error, return < 0 */
        else if (nwritten == 0)
            break;                         /* EOF */

        nleft -= nwritten;
        ptr   += nwritten;
    }

    return (nbytes - nleft);
}

int socketGetIpAddr(int socks,char *ipstr, int *port)
{
        struct sockaddr_in peer;
        int ret;
        char *ip_str,*ch,tmpstr[30];
        int len;

        len=sizeof(struct sockaddr);
        ret=getpeername(socks,(struct sockaddr *)&peer,(unsigned int *)&len);
        if (ret<0) return(-1);
        ip_str=inet_ntoa(peer.sin_addr);
        *port = ntohs(peer.sin_port);

        ch=memccpy(ipstr,ip_str,'\0',20);
        if (ch==NULL) ipstr[20]='\0';

        return(0);
}

int socketRecvDataFromConn(EmqNioConn *the_conn, char *buff, int nbytes) 
{
    int nread;    
    int datalen = 0;
    char *rebuff;
    int buffsize = 0;

    /*pthread_mutex_lock(&the_conn->recvBuff_mutex); */
    buffsize = the_conn->recvbuffsize;

    if(nbytes > (&the_conn->recvBuff[buffsize] - the_conn->recviptr)) {
        memcpy(the_conn->recvBuff, the_conn->recviptr, the_conn->recvoptr - the_conn->recviptr);
        the_conn->recvoptr = the_conn->recvBuff + (the_conn->recvoptr - the_conn->recviptr);
        the_conn->recviptr = the_conn->recvBuff;
    }

    if(nbytes > (&the_conn->recvBuff[buffsize] - the_conn->recvBuff)) {
       rebuff = (char *)malloc(nbytes+1);
       if(rebuff == NULL)  {
           applog(EMERG, "malloc memory error");
      /*     pthread_mutex_unlock(&the_conn->recvBuff_mutex); */
           return errno != 0 ? errno : ENOMEM; 
       }
       memcpy(rebuff, the_conn->recviptr, the_conn->recvoptr - the_conn->recviptr);
       the_conn->recvoptr = rebuff + (the_conn->recvoptr - the_conn->recviptr);
       the_conn->recviptr = rebuff; 
       free(the_conn->recvBuff);
       the_conn->recvBuff = rebuff;
       buffsize = the_conn->recvbuffsize = nbytes+1;
    }

    datalen = the_conn->recvoptr - the_conn->recviptr;
    while( datalen < nbytes ) {
       nread = recv(the_conn->fd, the_conn->recvoptr, &the_conn->recvBuff[buffsize] - the_conn->recvoptr, 0); 
       if(nread < 0 ) {
           if (errno != EWOULDBLOCK && errno != 2) {
               applog(INFO, "recv data on socket error %d,str:%s", errno,strerror(errno)); 
               emqNioDisconnectionConn(the_conn);
               return -1;
           }
       }else if(nread == 0) {
           applog(INFO, "the socket is closed by peer");
           emqNioDisconnectionConn(the_conn);
           return -1;
       }else {
           the_conn->recvoptr += nread;
           datalen += nread;
       }
    }

    memcpy(buff, the_conn->recviptr, nbytes);
    the_conn->recviptr += nbytes; 
    if(the_conn->recviptr == the_conn->recvoptr)
       the_conn->recviptr = the_conn->recvoptr = the_conn->recvBuff;

    /*pthread_mutex_lock(&the_conn->recvBuff_mutex); */

    return nbytes;
}

int socketSendDataToConn(EmqNioConn *the_conn, char *buff, int nbytes) 
{
    int nwrite;    
    int datalen = 0;
    char *rebuff;
    int buffsize = 0;

    /* pthread_mutex_lock(&the_conn->sendBuff_mutex); */
    buffsize = the_conn->sendbuffsize;

    if(nbytes > (&the_conn->sendBuff[buffsize] - the_conn->sendiptr)) {
        memcpy(the_conn->sendBuff, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr);
        the_conn->sendoptr = the_conn->sendBuff + (the_conn->sendoptr - the_conn->sendiptr);
        the_conn->sendiptr = the_conn->sendBuff;
    }

    if(nbytes > buffsize) {
       rebuff = (char *)malloc(nbytes+1);
       if(rebuff == NULL)  {
           applog(EMERG, "malloc memory error");
      /*    pthread_mutex_unlock(&the_conn->sendBuff_mutex); */
           return errno != 0 ? errno : ENOMEM; 
       }
       memcpy(rebuff, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr);
       the_conn->sendoptr = rebuff + (the_conn->sendoptr - the_conn->sendiptr);
       the_conn->sendiptr = rebuff; 
       free(the_conn->sendBuff);
       the_conn->sendBuff = rebuff;
       buffsize = the_conn->sendbuffsize = nbytes+1;
    }

    datalen = &the_conn->sendBuff[buffsize] - the_conn->sendoptr;
    while( datalen < nbytes ) {
       nwrite = send(the_conn->fd, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr, 0); 
       
/*
       nwrite = write(the_conn->fd, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr); 
*/
       if(nwrite < 0 ) {
           if (errno != EWOULDBLOCK && errno != 2) {
               applog(INFO, "send data on socket error %d", errno); 
               emqNioDisconnectionConn(the_conn);
               return -1;
           }
       }else {
           the_conn->sendiptr += nwrite;
           if(the_conn->sendiptr == the_conn->sendoptr)
               the_conn->sendiptr = the_conn->sendoptr = the_conn->sendBuff;
           else {
               memcpy(the_conn->sendBuff, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr);
               the_conn->sendoptr = the_conn->sendBuff + (the_conn->sendoptr - the_conn->sendiptr);
               the_conn->sendiptr = the_conn->sendBuff;
           }
           datalen = (&the_conn->sendBuff[buffsize] - the_conn->sendoptr);
       }
    }

    memcpy(the_conn->sendoptr, buff, nbytes);
    the_conn->sendoptr += nbytes; 
    nwrite = send(the_conn->fd, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr, 0);          
/*
applog(INFO, "skyline test fd:%d,iptr:%x,opter:%x",the_conn->fd, the_conn->sendiptr, the_conn->sendoptr);
    nwrite = write(the_conn->fd, the_conn->sendiptr, the_conn->sendoptr - the_conn->sendiptr); 
*/
    if(nwrite < 0 ) {
        if (errno != EWOULDBLOCK && errno != 2) {
           applog(INFO, "send data on socket error %d", errno); 
           emqNioDisconnectionConn(the_conn);
           return -1; 
        }
    }else {
        the_conn->sendiptr += nwrite;
        if(the_conn->sendiptr == the_conn->sendoptr)
            the_conn->sendiptr = the_conn->sendoptr = the_conn->sendBuff;
    }

    /*pthread_mutex_lock(&the_conn->sendBuff_mutex); */ 

    return nbytes;
}
