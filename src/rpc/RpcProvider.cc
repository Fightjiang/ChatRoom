#include "./rpc/RpcProvider.h" 
#include "./rpc/rpcheader.pb.h"
#include "./base/ZookeeperUtil.h" 

/*
使用 m_serviceMap 保存对应 service_name 下的 service  和 service 下的 method 方法 

在 Server 端运行之前需要调用 NotifyService 注册对应的服务和方法
UserService Login Logout
FriendService AddFriend DelFriend
*/

void RpcProvider::NotifyService(google::protobuf::Service *service) 
{
    // 获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor * pserviceDesc = service->GetDescriptor() ; 
    // 获取服务的名字
    const std::string& service_name = pserviceDesc->name() ; 
    // 获取服务对象 service 的方法的数量
    int methodCnt = pserviceDesc->method_count() ;

    std::cout << "service name : " << service_name << std::endl ;

    ServiceInfo service_info ; 
    for (int i = 0 ; i < methodCnt ; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述(抽象描述)  UserService Login
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i) ; 
        const std::string &method_name = pmethodDesc->name() ; 
        service_info.m_methodMap.insert({method_name , pmethodDesc}) ;

        std::cout << "method name : " << method_name << std::endl ;
    }

    service_info.m_service = service ; 
    m_serviceMap.insert({service_name , service_info}) ; 
}

void RpcProvider::Run(const char *Ip , const uint16_t Port)
{
 
    // ConfigInfo configinfo_ ;   
    // std::string rpc_ip = configinfo_.rpc_server_ip  ; 
    // uint16_t rpc_port  = configinfo_.rpc_server_port ;

    std::string rpc_ip = Ip ; 
    uint16_t rpc_port  = Port;
    muduo::net::InetAddress address(rpc_ip , rpc_port) ; 

    // 创建 TcpServer 对象
    muduo::net::TcpServer server(&m_eventLoop , address , "RpcProvider") ; 
    
    // 绑定连接断开回调和消息读写回调方法 , 分离网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection , this , std::placeholders::_1)) ; 
    
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage , this, 
                              std::placeholders::_1 ,  
                              std::placeholders::_2 ,  
                              std::placeholders::_3)) ; 

    // 设置 muduo 库中线程池的线程数量
    server.setThreadNum(4) ; 
    
    // 把当前 rpc 节点上要发布的服务全部注册到 zk 上面，让 rpc client 可以从 zk 上发现服务
    ZkClient zkCli ; 
    zkCli.Start() ; 

    // service_name 为永久性节点   method_name 为临时性节点
    for(auto &sp : m_serviceMap)
    {
        // "/service_name" 
        std::string service_path = "/" + sp.first ;
        zkCli.Create(service_path.data() , nullptr , 0) ; 

        for (auto &mp : sp.second.m_methodMap) 
        {
            // "/service_name/method_name"
            std::string method_path = service_path + "/" + mp.first ; 
            std::string method_path_data = rpc_ip + ":" + std::to_string(rpc_port) ; 
            // 临时性节点，连接断开则删除节点
            zkCli.Create(method_path.data() , method_path_data.data() , method_path_data.size() , ZOO_EPHEMERAL) ; 
        }
    }

    // rpc 服务端启动准备，打印消息
    std::cout << "RpcProvider start service at ip : "<< rpc_ip << " port : "<< rpc_port << std::endl ; 

    // 启动网络服务
    server.start() ; 
    m_eventLoop.loop() ;  
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if(conn->connected() == false)
    {
        // 和 rpc client 的连接断开
        conn->shutdown() ; 
    }
}


/*
RpcProvider 和 RpcConsumer 必须协商好之间通信的 protobuf 数据类型 
定义 proto 的 message 类型，进行数据头的序列化和反序列化

service_name method_name args 
16UserServiceLoginzhang san123456

header_size(4个字节) + header_str + args_str 
 
*/
// 如果远程有一个 rpc 服务的调用请求，那么 muduo 在读完发送过来的消息之后，就会执行 OnMessage 这个回调函数
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer* buffer, muduo::Timestamp)
{
    // 网络上接受远程 rpc 调用的请求数据流 
    std::string recv_buf = buffer->retrieveAllAsString() ; 

    // 从字符流中读取前 4 个字节的内容 
    uint32_t header_size = 0 ; 
    recv_buf.copy((char *)&header_size , 4 , 0) ; 

    // 根据 header_size 读取数据头的原始字符流
    std::string rpc_header_str = recv_buf.substr(4 , header_size) ; 

    // 发序列化数据，得到 rpc 请求的详细信息
    mprpc::RpcHeader rpcHeader ; 

    std::string service_name ; 
    std::string method_name ; 
    uint32_t args_size ; 

    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name() ; 
        method_name = rpcHeader.method_name() ; 
        args_size = rpcHeader.args_size() ; 
    }
    else 
    {
        // 数据头发序列化失败
        std::cout << "rpc_header_str: " << rpc_header_str <<" parse error " << std::endl ; 
        return ; 
    }

    std::string args_str = recv_buf.substr(4 + header_size , args_size) ; 
    
    // 打印调试信息
    std::cout << "=================================" << std::endl ; 
    std::cout << "header_size: "    << header_size << std::endl ; 
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl ; 

    std::cout << "service_name: "   << service_name << std::endl ; 
    std::cout << "method_name: "    << method_name << std::endl ; 
    std::cout << "args_size: "      << args_size << std::endl ; 
    std::cout << "args_str: "      << args_str << std::endl ; 
    std::cout << "=================================" << std::endl ; 

    // 获取 service 对象 和 method 对象
    auto serviceIt = m_serviceMap.find(service_name) ; 
    if(serviceIt == m_serviceMap.end())
    {
        std::cout << service_name << "is not exists" << std::endl ; 
        return ; 
    }

    auto methodIt = serviceIt->second.m_methodMap.find(method_name) ; 
    if(methodIt == serviceIt->second.m_methodMap.end())
    {
        std::cout << service_name << ":" << method_name << " is not exist!" << std::endl ; 
        return ; 
    }

    // 获取对应的 service 对象，如 UserService ; 
    google::protobuf::Service *service = serviceIt->second.m_service ; 
    // 获取对应的 method 对象，如 Login  
    const google::protobuf::MethodDescriptor *method = methodIt->second ; 
    
    // 生成 rpc 方法调用的请求 request 和 响应 response 参数
    google::protobuf::Message* request = service->GetRequestPrototype(method).New() ; 
    if(!request->ParseFromString(args_str))
    {
        std::cout << "requests parse error , content: " << args_str << std::endl ; 
        return  ;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New() ; 

    // 给下面的 method 方法的调用，绑定一个 Closure 的回调函数 , bind 等方法也是可以的
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider , 
                                                                    const muduo::net::TcpConnectionPtr& , 
                                                                    google::protobuf::Message*>
                                                                    (this , 
                                                                    &RpcProvider::SendRpcResponse , 
                                                                    conn , response) ; 

    // 在框架上根据远端 rpc 请求 ，调用当前 rpc 节点发布的方法
    // 统一的调用方法，调用完再执行 done 将 response 发送给客户端
    // new UserService().Login(controller , request , response , )
    service->CallMethod(method , nullptr , request , response , done);
}

// Closure 的回调操作，用于序列化 rpc 的响应和网络发送给请求端
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message *response) 
{
     
    std::string response_str ;
    if (response->SerializeToString(&response_str)) // response 进行序列化
    {
        // 序列化成功之后，通过网络把 rpc 方法执行的结果发送给 rpc 的调用方
        conn->send(response_str) ; 
    }     
    else 
    {
        std::cout << "serialize response_str error !" << std::endl ; 
    }
    conn->shutdown() ; // 模拟 http 的短链接服务，由 rpcprovider 主动断开连接
}