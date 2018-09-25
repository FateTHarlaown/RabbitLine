//
// Created by NorSnow_ZJ on 2018/8/30.
//

#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include "rabbitline.h"

extern int enableHook();
extern int co_accept(int fd, struct sockaddr *addr, socklen_t *len);

static int listenFd;

static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(shPort);
    int nIP = 0;
    if( !pszIP || '\0' == *pszIP
        || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0")
        || 0 == strcmp(pszIP,"*")
            )
    {
        nIP = htonl(INADDR_ANY);
    }
    else
    {
        nIP = inet_addr(pszIP);
    }
    addr.sin_addr.s_addr = nIP;

}


static int CreateTcpSocket(const unsigned short shPort /* = 0 */,const char *pszIP /* = "*" */,bool bReuse /* = false */)
{
    int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if( fd >= 0 )
    {
        if(shPort != 0)
        {
            if(bReuse)
            {
                int nReuseAddr = 1;
                setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
            }
            struct sockaddr_in addr ;
            SetAddr(pszIP,shPort,addr);
            int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
            if( ret != 0)
            {
                close(fd);
                return -1;
            }
        }
    }
    return fd;
}



void io_routline(int cli)
{
    RabbitLine::enableHook();
    char * buf = new char[99999];
    std::cout << "into io line! start to ehco!" << std::endl;
    while (1) {
        ssize_t ret = read(cli, buf, 99999);
        if (ret == 0) {
            //perror("cli close connection!");
            break;
        }

        if (ret < 0) {
            //perror("some error happened!");
            continue;
        }

        ret = write(cli, buf, ret);
        if (ret < 0) {
            //perror("some error happened!");
        }

    }

    delete [] buf;
    close(cli);
}

void accept_routline()
{
    RabbitLine::enableHook();
    std::cout << "into accet line!" << std::endl;
    while (1) {
        std::cout << "begin to accept" << std::endl;
        int cli = co_accept(listenFd, NULL, NULL);
        if (cli < 0) {
            continue;
        }
        SetNonBlock(cli);
        //std::cout << "a cli " << cli << " connect! start create his io_roultline!" << std::endl;
        int io = RabbitLine::create(std::bind(io_routline, cli));
        RabbitLine::resume(io);
    }
}

int main()
{
    RabbitLine::enableHook();
    listenFd = CreateTcpSocket(54321, INADDR_ANY, true);
    listen(listenFd, 1024);

    if (listenFd == -1) {
        std::cout << "listen fd -1 !!" << std::endl;
        exit(1);
    }

    SetNonBlock(listenFd);
    std::cout << "start listen, begin ti create accept routline!" << std::endl;
    int ac = RabbitLine::create(accept_routline);
    RabbitLine::resume(ac);
    RabbitLine::eventLoop();
}
