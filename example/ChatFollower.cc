#include "./chat/ChatFollower.h"
using namespace std ;

int main(int argc , char **argv) 
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./UserFollowerRun 127.0.0.1 8006" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    const char *ip = argv[1] ;
    const uint16_t port = atoi(argv[2]) ;
    
    // 设置日志级别
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);  

    ChatFollower chatFollowerServer ; 
    chatFollowerServer.run(ip , port) ; 

    return 0 ; 
}