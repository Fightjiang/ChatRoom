#include "./base/CommonConfig.h"

namespace ConfigInfo
{
    const char* rpc_server_ip = "127.0.0.1";
    const int rpc_server_port = 8080 ; 
    
    // zookeeper 配置
    const char* zookeeper_ip = "127.0.0.1" ; 
    const int zookeeper_port=2181 ; 
    
    // redis 配置
    const char* redis_ip = "127.0.0.1" ; 
    const int redis_port=6379 ; 
    
    // Mysql 配置
    const char * mysql_host = "localhost" ;                          // 数据库 IP 
    const int mysql_port = 3306 ;                                    // 数据库端口
    const char * mysql_user = "root" ;                               // 数据库账号
    const char * mysql_pwd = "root123" ;                             // 数据库密码
    const char * mysql_dbName = "chatroom" ;                         // 使用的 database
    const int mysql_maxConnCount = 16   ;                            // 数据库连接池的最大数量
    const int mysql_minConnCount = 8   ;                             // 数据库连接池的正常数量
    const int mysql_timeOut = 2 * 1000 ;                             // 想获得连接，阻塞 2s 则返回空不再等待
    const int mysql_maxIdleTime = 5 * 1000 ;                         // 空闲连接空闲了 5s

    const char *server_IP = "127.0.0.1" ;                            // 服务器 Ip  
    int server_Port = 8080 ;                                         // 服务器端口


}; 