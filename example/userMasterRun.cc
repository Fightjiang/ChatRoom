#include "./user/UserMaster.h"
#include "./rpc/RpcProvider.h"
#include <iostream>
using namespace std;

int main(int argc , char **argv)
{
    if (argc != 3) {
        cerr << "command invalid! example: ./userMasterRun 127.0.0.1 8004" << endl;
        exit(-1); 
    }

    const char* ip = argv[1];
    int port = std::atoi(argv[2]);  // 将字符串转换为整数
    
    // 设置日志级别
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);  

    // provider 是一个 rpc 网络服务对象, 把 UserService 对象发布到 rpc 节点上
    RpcProvider provider ; 
    provider.NotifyService(new UserMaster()) ; 
    
    // 启动服务进行监听
    provider.Run(ip , port) ; 
    return 0 ; 
}