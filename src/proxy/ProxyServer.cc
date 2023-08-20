#include "./proxy/ProxyServer.h"

//初始化服务器信息
ProxyServer::ProxyServer() {}

void ProxyServer::start(const char* ip, const uint16_t port) 
{
    ip_ = ip ; port_ = port ;
    muduo::net::InetAddress address(ip_ , port_) ; 
    // 创建 TcpServer 对象
    muduo::net::TcpServer server(&mainLoop_ , address , "ProxyServer") ; 
    //注册连接事件回调函数
    server.setMessageCallback(std::bind(&ProxyServer::on_message, this, 
                               std::placeholders::_1, 
                               std::placeholders::_2, 
                               std::placeholders::_3));

    //注册连接事件回调函数
    server.setConnectionCallback(std::bind(&ProxyServer::on_connetion, this, 
                                 std::placeholders::_1));

    //设置工作线程数量
    //最佳线程数目 = （线程等待时间与线程CPU时间之比 + 1）* CPU数目 ==>高网络I/O设计
    server.setThreadNum(4);

    // 启动网络服务
    std::cout << "ProxyServer start IP : " << ip_ <<" Port = " << port_ << std::endl ;
    proxyService_ = std::make_shared<ProxyService>(ip_.data() , port_) ;
    server.start() ; 
    mainLoop_.loop() ;  
}

//读写事件回调函数
void ProxyServer::on_message(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp stamp)
{
    //反序列化
    std::string recv_str = buffer->retrieveAllAsString();
    Proxy::ProxyRequest request;
    request.ParseFromString(recv_str);

    //获取对应的处理器并执行
    auto msg_handler = proxyService_->get_handler(request.type());
    std::string strMsg = request.request_msg();
    msg_handler(conn, strMsg, stamp);
}

//连接事件的回调函数
void ProxyServer::on_connetion(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        //当客户端异常退出，执行此命令
        proxyService_->client_close_exception(conn);
        conn->shutdown();
    }
}

// 清理客户端连接资源
void ProxyServer::on_clear() 
{
    proxyService_->reset() ; 
}