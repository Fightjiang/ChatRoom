#include "./base/ZookeeperUtil.h"
#include "./base/CommonConfig.h"
#include <iostream>

// 全局的 watcher 观察器 zkserver 给 zkclient 发通知
void global_watcher(zhandle_t *zh , int type ,
                    int state , const char *path , void *watcherCtx)
{
    if(type == ZOO_SESSION_EVENT) // 回调的消息类型是和会话相关的消息类型
    {
        if(state == ZOO_CONNECTED_STATE) // zkclient 和 zkServer 连接成功
        {
            sem_t *sem = (sem_t*) zoo_get_context(zh) ; 
            sem_post(sem) ; // 唤醒
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr) 
{
}

ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle) ; // 关闭句柄，释放资源 
    }
}

// 启动
void ZkClient::Start() 
{
    ConfigInfo configinfo_ ;    

    std::string host = configinfo_.zookeeper_ip ; 
    std::string port = std::to_string(configinfo_.zookeeper_port) ; 
    std::string IPportStr = host + ":" + port ; 

    /*
    zookeeper_mt: 多线程版本
    zookeeper 的 API 客户端程序提供了三个线程
    1. API  调用线程
    2. 网络 I/O 线程 pthread_create poll
    3. watcher 回调线程
    */
    m_zhandle = zookeeper_init(IPportStr.data() , global_watcher , 30000 , nullptr , nullptr , 0) ;
    // 返回来的时候，并不意味这 zookeeper 连接成功了，因为上面的函数，是一个异步操作
    // 注意这里 判断nullptr == m_zhandle 是否为空，只是表示 zookeeper_init 初始化的参数和一些内存栈开辟是否出错
    if(nullptr == m_zhandle) {
        std::cout << "zookeeper_init error" << std::endl ; 
        exit(EXIT_FAILURE) ; 
    }

    sem_t sem ; 
    sem_init(&sem , 0 , 0) ; 
    zoo_set_context(m_zhandle , &sem) ; // 向 m_zhangle 添加信息

    sem_wait(&sem) ; // 阻塞，等待唤醒
    std::cout << "zookeeper_init_success" << std::endl ; 
}

// 在 zkserver 上根据指定的 path 创建 znode 节点
// state 对应 zoo_create 参数的 flags 
// 也就是对应的 
// extern ZOOAPI const int ZOO_EPHEMERAL;  // 短暂的临时节点 If ZOO_EPHEMERAL flag is set, the node will automatically get removed if the client session goes away.
// extern ZOOAPI const int ZOO_SEQUENCE;  //  永久节点，客户端断开，服务端并不会删除该节点
void ZkClient::Create(const char *path , const char *data , int datalen , int state) 
{
    char path_buffer[128] ; 
    int bufferlen = sizeof(path_buffer) ; 
    // 判断 path 表示的 znode 节点是否存在 ， 如果存在， 就不再重复创建了
    int flag = zoo_exists(m_zhandle , path , 0 , nullptr) ; 
    if(ZNONODE == flag) // 表示 path 的 znode 节点不存在
    {
        // 创建指定的 path 的 znode 节点
        flag = zoo_create(m_zhandle , path , data , datalen ,
                        &ZOO_OPEN_ACL_UNSAFE , state , path_buffer , bufferlen) ; 
        if(flag == ZOK) 
        {
            std::cout << "znode create success ... path: " << path << std::endl ; 
        }
        else 
        {
            std::cout << "flag : " << flag << std::endl ; 
            std::cout << "znode create error ... path: " << path << std::endl ;
            exit(EXIT_FAILURE) ; 
        }
    }
}

// 根据参数指定的 znode 节点路径 path ，获得 znode 节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64] ; 
    int bufferlen = sizeof(buffer) ; 
    // 同步
    int flag = zoo_get(m_zhandle , path , 0 , buffer , &bufferlen , nullptr) ; 
    if(flag != ZOK) 
    {
        std::cout << "get znode error ... path : " << path << std::endl ;
        return "" ; 
    }
    else 
    {
        return buffer ; 
    }
}