#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

namespace ConfigInfo
{
    extern const char* rpc_server_ip ;
    extern const int rpc_server_port ; 
    
    // zookeeper 配置
    extern const char* zookeeper_ip ; 
    extern const int zookeeper_port ; 
    
    // redis 配置
    extern const char* redis_ip ; 
    extern const int redis_port ; 
    
    // Mysql 配置
    extern const char * mysql_host ;                                 // 数据库 IP 
    extern const int mysql_port ;                                    // 数据库端口
    extern const char * mysql_user ;                                 // 数据库账号
    extern const char * mysql_pwd ;                                  // 数据库密码
    extern const char * mysql_dbName ;                               // 使用的 database
    extern const int mysql_maxConnCount ;                            // 数据库连接池的最大数量
    extern const int mysql_minConnCount ;                            // 数据库连接池的正常数量
    extern const int mysql_timeOut ;                                 // 想获得连接，阻塞 2s 则返回空不再等待
    extern const int mysql_maxIdleTime ;                             // 空闲连接空闲了 5s

    extern const char *server_IP ;                                   // 服务器 Ip  
    extern int server_Port;                                          // 服务器端口


}; 


#endif