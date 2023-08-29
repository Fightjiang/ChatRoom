#ifndef SOCKET_H
#define SOCKET_H

#include <unistd.h>

// 封装socket fd
class Socket 
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    ~Socket() { 
        ::close(sockfd_);
    }

    // 获取sockfd
    int fd() const { return sockfd_; }
private:
    const int sockfd_;
};


#endif