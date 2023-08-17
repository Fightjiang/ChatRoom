#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include "../proxy/ProxyService.h"
#include "../proxy/Proxy.pb.h"

#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Timestamp.h>

#include <string> 
#include <memory>
#include <functional>

class ProxyServer
{
private :
    // 初始化服务器信息
    ProxyServer(const char* , const uint16_t) ; 

    // 代理服务器开始工作
    void start() ; 

     //连接事件的回调函数
    void on_connetion(const muduo::net::TcpConnectionPtr &conn);

    // 读写事件回调函数
    void on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp stamp);

    // 清理服务器资源
    void on_clear() ; 
private :
    muduo::net::EventLoop mainLoop_;
    const char* ip_ ; 
    const uint16_t port_ ;  
    std::shared_ptr<ProxyService> proxyService_ ; 
} ; 


#endif