#include <iostream>
#include <thread>
#include <string>
#include <string.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "./user/User.pb.h"
#include "./proxy/Proxy.pb.h"
using namespace std;

bool CallMethod(const int clientfd , 
                google::protobuf::Message *proxyRequest ,
                google::protobuf::Message *proxyResponse) 
{
    std::string request = proxyRequest->SerializeAsString();
    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) ; 
    if (len == -1)
    {
        cerr << "send request msg error:" << request << endl;
    }

    char buffer[1024] = {0} ;
    len = recv(clientfd, buffer, 1024, 0);  // 阻塞了

    if(!proxyResponse->ParseFromString(buffer))
    {
        std::cout << "serialize fail" << std::endl ;
        return false;
    }
    
    return true ;
}

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }
    Proxy::ProxyRequest proxyRequest ; 
    proxyRequest.set_type("Login") ; 

    User::LoginRequest loginRequest ;
    loginRequest.set_name("zhangsan") ;
    loginRequest.set_password("123456") ; 
    std::string msg = loginRequest.SerializeAsString() ; 
    proxyRequest.set_request_msg(msg);

    User::LoginReponse loginResponse ; 
    if(CallMethod(clientfd , &proxyRequest , &loginResponse)) 
    {
        cout << proxyRequest.type() << " " << loginResponse.is_success() << " " << loginResponse.message() << std::endl ;
    }  
    ::close(clientfd);
    return 0 ;
}