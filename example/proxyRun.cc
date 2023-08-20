#include "./proxy/ProxyServer.h"
#include <semaphore.h>
#include <signal.h>
#include <functional>
using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void reset_handler(int)
{
    ProxyServer::getInstance().on_clear(); 
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ProxyServer 127.0.0.1 8003" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    const char *ip = argv[1] ;
    const uint16_t port = atoi(argv[2]) ;
    
    ProxyServer &proxyServer = ProxyServer::getInstance() ;
    proxyServer.start(ip , port) ;

    // auto func = [&](int signum) { proxyServer_.on_clear() ; } ; 
    // signal(SIGINT, func) ; 

    return 0 ;
}