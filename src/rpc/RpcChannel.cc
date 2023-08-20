
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <error.h>

#include "./rpc/rpcheader.pb.h"
#include "./rpc/RpcChannel.h" 
#include "./base/ZookeeperUtil.h"

// header_size + service_name method_name args_size + args 
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller, 
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response, 
                            google::protobuf::Closure* done)
{
    
    const google::protobuf::ServiceDescriptor* sd = method->service() ; 
    std::string service_name = sd->name() ; 
    std::string method_name = method->name() ; 

    // 获取参数的序列化字符串长度 args_size 
    uint32_t args_size = 0 ; 
    std::string args_str ; 
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size() ; 
    }
    else 
    {
        controller->SetFailed("serialise requests error !") ; 
        return ;
    }

    // 定义 rpc 的请求头
    mprpc::RpcHeader rpcHeader ;
    rpcHeader.set_service_name(service_name) ; 
    rpcHeader.set_method_name(method_name) ; 
    rpcHeader.set_args_size(args_size) ; 

    uint32_t header_size = 0 ;  
    std::string rpc_header_str ; 
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size() ;
    }
    else 
    { 
        controller->SetFailed("serialise rpc header error !") ; 
        return ;
    }

    // 组织待发送的 rpc 请求字符串
    std::string send_rpc_str ; 
    send_rpc_str.insert(0 , std::string((char *)&header_size , 4)) ; 
    send_rpc_str += rpc_header_str ; // hearder  
    send_rpc_str += args_str       ; // args 

    // 打印调试信息
    std::cout << "=================================" << std::endl ; 
    std::cout << "header_size: "    << header_size << std::endl ; 
    // 打印的时候，为什么会突兀的换行呢？因为 rpc_header_str 是调用 protobuf 提供的 SerializeToString 其中还包括 args_size 等信息,不一定完全
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl ; 

    std::cout << "service_name: "   << service_name << std::endl ; 
    std::cout << "method_name: "    << method_name << std::endl ; 
    std::cout << "args_size: "      << args_size << std::endl ; 
    std::cout << "args_str: "      << args_str << std::endl ; 
    std::cout << "=================================" << std::endl ; 

    // 使用 Tcp 编程，完成 rpc 方法的远程调用发送
    int clientfd = socket(AF_INET , SOCK_STREAM , 0) ; 
    if(-1 == clientfd) 
    {
        controller->SetFailed("create socket error ! errno : " + std::to_string(errno)) ; 
        return ; 
    }

    // // 读取配置文件 rpcserver 的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip") ; 
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").data()) ;
    
    // rpc 调用方想调用 service_name 的 method_name 服务，需要查询 zk 上该服务所在的 host 信息
    ZkClient zkCli ; 
    zkCli.Start() ; 
    std::string method_path = "/" + service_name + "/" + method_name ; 
    std::string host_data = zkCli.GetNodeData(method_path.data()) ; 
    if(host_data == "")
    {
        controller->SetFailed(method_path + " is not exist !!" ) ; 
        return ; 
    }
    size_t idx = host_data.find(":") ; 
    if(idx == std::string::npos) 
    {
        controller->SetFailed(method_path + " address is invalid !!" ) ; 
        return ; 
    }

    std::string ip = host_data.substr(0 , idx)  ;
    uint32_t port = atoi(host_data.substr(idx + 1 , host_data.size() - 1).data()) ; 
    struct sockaddr_in server_addr ;  
    server_addr.sin_family = AF_INET  ;
    server_addr.sin_addr.s_addr   = inet_addr(ip.data()) ; 
    server_addr.sin_port   = htons(port) ; 
    // 连接 rpc 服务节点
    if( -1 == connect(clientfd , (struct sockaddr *) &server_addr , sizeof(server_addr)))
    {
        ::close(clientfd) ; 
        controller->SetFailed("connnect error ; errno: " + std::to_string(errno)) ; 
        return ; 
    }

    if(-1 == send(clientfd , send_rpc_str.data() , send_rpc_str.size() , 0))
    {
        ::close(clientfd) ; 
        controller->SetFailed("send error ! errno :" + std::to_string(errno)) ; 
        return ; 
    }

    // 接受 rpc 请求的响应值
    char recv_buf[1024] = {0} ; 
    int recv_size = 0 ; 
    if(-1 == (recv_size = recv(clientfd , recv_buf , 1024 , 0)))
    {
        ::close(clientfd) ; 
        controller->SetFailed("recv error ! errno : " + std::to_string(errno)) ; 
        return ; 
    }
    // 发序列化 rpc 调用的响应数据
    // 将 recv_buf 转化成 string 的时候会出现问题，因为 string 构造的时候遇到 \0 后面的数据就不会转化了，存在丢失
    // std::string response_str(recv_buf , 0 , recv_size) ; 
    // if(!response->ParseFromString(response_str))
    // 故我们直接用 recv_buf 来实现数据转化功能
    if(!response->ParseFromArray(recv_buf , recv_size))
    {
        ::close(clientfd) ; 
        controller->SetFailed("parse error ! response_str : " + std::to_string(errno)) ; 
        return ;
        
    }
    ::close(clientfd) ;  
}