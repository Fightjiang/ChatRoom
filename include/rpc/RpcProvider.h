#ifndef RPC_PROVIDER_H
#define RPC_PROVIDER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h> 
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include <string>
#include <unordered_map>
#include <functional>

// 框架提供的专门服务发布 rpc 服务的网络对象类
class RpcProvider 
{
public: 
    // 这里是框架提供给外部使用的，可以发布 rpc 方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    // 启动 rpc 服务节点，开始提供 rpc 远程网络调用服务 , 进程进入阻塞状态，等待远程的 rpc 调用请求
    void Run(const char* , const uint16_t);

private: 

    // 组合 EventLoop ; 
    muduo::net::EventLoop m_eventLoop; 

    // service 服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service; // 保存服务对象
        std::unordered_map<std::string , const google::protobuf::MethodDescriptor*> m_methodMap; // 保存服务方法
    };

    // 存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string , ServiceInfo> m_serviceMap; 

    // 新的 socket 连接建立回调
    void OnConnection(const muduo::net::TcpConnectionPtr &conn); 

    // 已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp); 

    // Closure 的回调操作，用于序列化 rpc 的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr& , google::protobuf::Message *); 
} ; 

#endif 