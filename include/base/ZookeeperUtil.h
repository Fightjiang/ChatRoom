#ifndef ZOOKEEPER_UTIL_H
#define ZOOKEEPER_UTIL_H

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../base/Socket.h" 

// 封装的 zk 客户端类
class ZkClient
{
public:
    ZkClient(const std::string &path = "")   ;
    ~ZkClient()  ;
    // zkClient 启动连接 zkserver
    void Start() ;
    // 在 zkserver 上根据指定的 path 创建 znode 节点
    bool CreateNode(const char *path , const char *data , int datalen , int state = 0) ; 
    // 删除节点
    void DeleteNode(const char *path) ;
    // 判断该路径是否存在 
    bool IsNodeExist(const char *path) ; 
    // 根据参数指定的 znode 节点路径，获得 znode 节点的值
    std::string GetNodeData(const char *path) ; 
    // 修改对应节点上的值
    bool SetNodeData(const char *path , const char *value);
    // // 更新从服务器对应的 IP 列表
    // void FlushFollower();
    // 等到一个可用的从服务器节点，并建立连接，返回文件描述符
    std::shared_ptr<Socket> GetFollowerFd() ;

private : 
    // zk 的客户端句柄
    zhandle_t *m_zhandle ; 
    std::string path_ ; // 当前服务下的路径
    // std::unordered_map<std::string, std::string> followers_; //存储从节点
    int current_service_; //当前该是哪个节点服务
    int total_services_;  //总共有多少节点服务 
} ; 

#endif