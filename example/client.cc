#include <iostream>
#include <thread>
#include <string>
#include <string.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <memory>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include "./user/User.pb.h"
#include "./proxy/Proxy.pb.h"
#include "./chat/ChatMessage.pb.h"
#include "./base/Socket.h"
using namespace std;

// 控制主菜单页面程序
bool isMainMenuRunning = false;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};
// 记录用户名字
std::string g_userName ; 
// 父线程只负责往 fd 写缓冲区中写入数据，子线程只负责向 fd 读缓冲区中读取数据，这样才不会冲突
sem_t rwsem;

// 记录系统的用户列表信息
vector<std::string> g_currentUserList;

// "help" command handler
void help(int fd = 0, const string &str = "") ;
// "chat" command handler
void chat(int, const string &) ; 
void getUserInfo(int , const string &) ; 
// "loginout" command handler
void logout(int, const string &) ;

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = { 
    {"help", "显示所有支持的命令,格式help"},
    {"getUserInfo" , "显示用户信息, 格式 getUserInfo"},
    {"chat", "一对一聊天, 格式 chat:name:message"},
    {"logOut", "注销, 格式 logOut"}
};

// 注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"getUserInfo",getUserInfo},
    {"chat", chat}, 
    {"logOut", logout}
};

// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

bool CallMethod(const int clientfd , 
                google::protobuf::Message *proxyRequest ,
                google::protobuf::Message *proxyResponse) 
{
    std::string request = proxyRequest->SerializeAsString();
    int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) ; 
    if (len == -1)
    {
        cerr << "send request msg error:" << request << endl;
        return false ;
    }

    // char buffer[1024] = {0} ;
    // len = recv(clientfd, buffer, 1024, 0);  // 阻塞了

    // if(!proxyResponse->ParseFromString(buffer))
    // {
    //     std::cout << "serialize fail" << std::endl ;
    //     return false;
    // }
    
    return true ;
}

void mainMenu(const shared_ptr<Socket> clientFd)
{ 
    help() ;  
    char buffer[1024] = {0};
    while (g_isLoginSuccess)
    {
        cin.getline(buffer, 1024);  
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientFd->fd(), commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}

// "help" command handler
void help(int fd, const string &str)
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// "chat" command handler
void chat(int clientfd, const string &str)
{
    int idx = str.find(":"); // Name:message
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    std::string recvname = str.substr(0, idx) ;
    std::string msg = str.substr(idx + 1, str.size() - idx);

    Proxy::ProxyRequest proxyRequest ; 
    proxyRequest.set_type("chatMessage") ; 

    ChatMessage::Message message ;
    message.set_sendname(g_userName) ;
    message.set_recvname(recvname) ; 
    message.set_msg(msg) ; 
    message.set_time(getCurrentTime());

    std::string proxyMsg = message.SerializeAsString() ; 
    proxyRequest.set_request_msg(proxyMsg);

    ChatMessage::ChatResponse chatResponse ; 
    // 发送消息给 recvname 
    if( CallMethod(clientfd , &proxyRequest , &chatResponse) == false) {
        std::cout << "send Chat Message Error , Maybe Reason: " << chatResponse.message() << std::endl ;
    } 
}
 
void logout(int clientfd, const string &str)
{
    Proxy::ProxyRequest proxyRequest ; 
    proxyRequest.set_type("Logout") ; 
    
    User::LogOutRequest LogoutRequest;
    LogoutRequest.set_name(g_userName) ;
    
    std::string proxyMsg = LogoutRequest.SerializeAsString() ; 
    proxyRequest.set_request_msg(proxyMsg);

    // 发送消息给 recvname 
    User::LogOutResponse LogoutResponse ;
    CallMethod(clientfd , &proxyRequest , &LogoutResponse) ; 
    // if(LogoutResponse.is_success()) {
    //     isMainMenuRunning = false;
    //     std::cout << "LogOut Success" << std::endl ;
    // } else {
    //     std::cout << LogoutResponse.message() << std::endl ;
    // }
}

void getUserInfo(int clientfd, const string &str)
{
    Proxy::ProxyRequest proxyRequest ; 
    proxyRequest.set_type("getUserInfo") ; 
    
    User::LogOutRequest LogoutRequest;
    LogoutRequest.set_name(g_userName) ;
    
    std::string proxyMsg = LogoutRequest.SerializeAsString() ; 
    proxyRequest.set_request_msg(proxyMsg);

    // 发送消息给 recvname 
    User::LogOutResponse LogoutResponse ;
    CallMethod(clientfd , &proxyRequest , &LogoutResponse) ; 
    // if(LogoutResponse.is_success()) {
    //     isMainMenuRunning = false;
    //     std::cout << "LogOut Success" << std::endl ;
    // } else {
    //     std::cout << LogoutResponse.message() << std::endl ;
    // }
} 


// 子线程 - 接收线程
void readTaskHandler(shared_ptr<Socket> socketFd)
{
    while(true)
    {
        char buffer[1024] = {0};
        int len = recv(socketFd->fd(), buffer, 1024, 0);  // 阻塞了
        if (-1 == len || 0 == len)
        { 
            exit(-1);
        }
        Proxy::ProxyResponse proxyResponse ; 
        if(!proxyResponse.ParseFromString(buffer))
        {
            std::cout << "serialize fail" << std::endl ;
            exit(-1);
        }
        std::string methodType = proxyResponse.type() ;
        if(methodType == "Login") {
        
            User::LoginReponse response;
            response.ParseFromString(proxyResponse.response_msg()) ; 
            if(response.is_success()) {
                g_isLoginSuccess = true ;    
            }
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成 
            std::cout << response.message() << std::endl ;
        
        } else if(methodType == "Logout") {

            User::LogOutResponse response;
            response.ParseFromString(proxyResponse.response_msg()) ; 
            g_isLoginSuccess = false ; 
            std::cout << response.message() << std::endl ;

        } else if(methodType == "Register") {
        
            User::RegisterResponse response ; 
            response.ParseFromString(proxyResponse.response_msg()) ; 
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成 
            std::cout << response.message() << std::endl ;
        
        }else if(methodType == "GetUserInfo") {
            User::GetUserInfoResponse response ; 
            response.ParseFromString(proxyResponse.response_msg()) ; 
            if(response.is_success()){
                int idx = 1 ; 
                for(const std::string &name : response.usernames()) {
                    g_currentUserList.push_back(name) ;
                    std::cout << idx << " . " << name << std::endl ;
                }
            } else {
                std::cout << "GetUserInfo Fail" << std::endl ;
            }
        }else if(methodType == "SendChatMessage") {
            
            ChatMessage::ChatResponse response ; 
            response.ParseFromString(proxyResponse.response_msg()) ;
            if (!response.is_success()){
                std::cout << response.message() << std::endl ;
            } 
            std::cout << response.message() << std::endl ;

        }else if(methodType == "RecvChatMessage") {
        
            ChatMessage::Message message ; 
            message.ParseFromString(proxyResponse.response_msg()) ;
            std::cout << message.time() << "[" << message.sendname() << "] : " << message.msg() << std::endl ; 
        
        }else if(methodType == "ReadOffline") {
            ChatMessage::ReadOfflineResponse readOfflineMessage ; 
            readOfflineMessage.ParseFromString(proxyResponse.response_msg()) ;
            if(readOfflineMessage.is_success()) {
                for(const ChatMessage::Message &message : readOfflineMessage.messages()) {
                    std::cout << message.time() << "[" << message.sendname() << "] : " << message.msg() << std::endl ; 
                }
            }
        }
    }
    
}

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 8000" << endl;
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

    shared_ptr<Socket> clientSocketFd(new Socket(clientfd)) ; 

    // 初始化读写线程同步通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 启动接收消息的子线程
    std::thread readTask(readTaskHandler , clientSocketFd) ;
    readTask.detach() ; 
    bool quitMain = true ;
    // 主线程用于接收用户输入，负责发送数据给 proxyServer 服务器
    while(quitMain) 
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:" ; 
        int choice = 0 ; cin>>choice ; 
        switch (choice)
        {
        case 1:
            {
                cout << "username:" ; cin>>g_userName ;
                cout << "userpassword:" ; 
                std::string userPassword ; cin>>userPassword ; getchar() ; 

                Proxy::ProxyRequest proxyRequest ; 
                proxyRequest.set_type("Login") ; 

                User::LoginRequest loginRequest ;
                loginRequest.set_name(g_userName) ;
                loginRequest.set_password(userPassword) ; 
                std::string msg = loginRequest.SerializeAsString() ; 
                proxyRequest.set_request_msg(msg);

                User::LoginReponse loginResponse ; 
                if(CallMethod(clientSocketFd->fd() , &proxyRequest , &loginResponse)) 
                {
                    //cout << proxyRequest.type() << " " << loginResponse.is_success() << " " << loginResponse.message() << std::endl ;
                    sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
                    if(g_isLoginSuccess) {
                        // 进入聊天主菜单页面 
                        mainMenu(clientSocketFd);
                    } 
                } 
            }
            break;
        case 2:
            {
                cout << "username:" ;
                cin>>g_userName ;
                cout << "userpassword:" ; 
                std::string userPassword ; cin>>userPassword ;

                Proxy::ProxyRequest proxyRequest ; 
                proxyRequest.set_type("Register") ; 

                User::RegisterRequest registerRequest ;
                registerRequest.set_name(g_userName) ;
                registerRequest.set_password(userPassword) ; 
                std::string msg = registerRequest.SerializeAsString() ; 
                proxyRequest.set_request_msg(msg);

                User::RegisterResponse registerResponse ; 
                if(CallMethod(clientSocketFd->fd() , &proxyRequest , &registerResponse)) 
                {
                    sem_wait(&rwsem);
                    // std::cout << registerResponse.message() << std::endl ;
                    // if(registerResponse.is_success()) {
                    //     std::cout << "Login Again Please" << std::endl ;
                    // }
                }  
            }
            break ;
        case 3 : 
            quitMain = false ; 
            cout << "quit success!" << endl ;
            break ; 
        default:
            quitMain = false ; 
            cerr << "invalid input!" << endl;
            break ; 
        }
    }
    return 0 ;
}
