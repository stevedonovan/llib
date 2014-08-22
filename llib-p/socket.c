#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <llib/str.h>

// #define CHECK(x) if ((x) < 0) {perror(#x); return -1; }
// #define CHECK0(x) if((x) == NULL) {perror(#x); return -1;}

#define CHECK(x) if ((x) < 0) {return -1; }
#define CHECK0(x) if((x) == NULL) {return -1;}

typedef struct sockaddr sockaddr;

int socket_connect(const char *address)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    struct hostent *server;
    char **parts = str_split(address,":");
    char *addr = parts[0];
    int port = atoi(parts[1]);
    
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);    
    
    CHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0));
    CHECK0(server = gethostbyname(addr));
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr_list[0],server->h_length);
    CHECK(connect(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr)));
    unref(parts);
    return sockfd;        
}

int socket_server(const char *address)
{
    struct sockaddr_in serv_addr;
    int sockfd,on = 1;
    char **parts = str_split(address,":");
    int port = atoi(parts[1]);
    
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);    
    
    CHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0));
    CHECK(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    CHECK(bind(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr)));
    listen(sockfd,5);
    unref(parts);
    return sockfd;     
}

int socket_accept(int server_fd)
{
    struct sockaddr_in cli_addr;
    int fd;
    socklen_t clilen = sizeof(cli_addr);
    CHECK(fd = accept(server_fd,(sockaddr*)&cli_addr,&clilen));
    return fd;
}

int socket_server_once(const char *address)
{
    int fd, server_fd = socket_server(address);
    if (server_fd == -1)
        return -1;
    fd = socket_accept(server_fd);
    close(server_fd);
    return fd;
}

void socket_set_timeout(int s, int sec) {
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) == -1) {
        perror("setsockopt");
    }
}

