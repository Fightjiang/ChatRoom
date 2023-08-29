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

ZkClient::ZkClient(const std::string &path) : m_zhandle(nullptr) , path_(path)
{
    this->Start() ; 
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
    std::string host = ConfigInfo::zookeeper_ip ; 
    std::string port = std::to_string(ConfigInfo::zookeeper_port) ; 
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
    
    // std::cout << "zookeeper_init_success" << std::endl ; 
    if(path_.size() > 0 && !this->IsNodeExist(path_.data()))
    {
        this->CreateNode(path_.data() , nullptr , 0 , 0) ; 
    }
}

bool ZkClient::IsNodeExist(const char *path)
{
    int result = zoo_exists(m_zhandle , path , 0 , nullptr) ; 
    return result == ZOK ; 
}

// 执行这个删除的命令，不管 path 是否存在，如果没有存在，则说明本来就没有
void ZkClient::DeleteNode(const char *path)
{
    zoo_delete(m_zhandle , path , -1) ; 
}

// 在 zkserver 上根据指定的 path 创建 znode 节点
// state 对应 zoo_create 参数的 flags 
// 也就是对应的 
// extern ZOOAPI const int ZOO_EPHEMERAL;  
// 短暂的临时节点 If ZOO_EPHEMERAL flag is set, the node will automatically get removed if the client session goes away.

// extern ZOOAPI const int ZOO_SEQUENCE;  
// 永久节点，客户端断开，服务端并不会删除该节点

bool ZkClient::CreateNode(const char *path , const char *data , int datalen , int state) 
{
    char path_buffer[128] ; 
    int bufferlen = sizeof(path_buffer) ; 
    
    // 创建指定的 path 的 znode 节点
    // zh：ZooKeeper 客户端的句柄，通过 zookeeper_init 函数创建。
    // path：要创建的节点的路径，以 / 开头。例如："/mynode".
    // data：要存储在节点中的数据的指针。可以是 NULL，表示不存储数据。
    // datalen：要存储在节点中的数据的长度，如果 data 为 NULL，则此参数应为 0。
    // ZOO_OPEN_ACL_UNSAFE:  创建节点时指定节点的权限，
    // flags：创建节点的标志。可以是以下值之一：
        // 0：永久节点（持久节点）。
        // ZOO_EPHEMERAL：临时节点。
        // ZOO_SEQUENCE：顺序节点。

    // path_buffer：用于存储实际创建的节点路径的缓冲区。
    // path_buffer_len：path_buffer 的长度。
    
    int flag = zoo_create(m_zhandle , path , data , datalen ,
                        &ZOO_OPEN_ACL_UNSAFE , state , path_buffer , bufferlen) ; 
    if(flag != ZOK) 
    {
        std::cout << "flag : " << flag << std::endl ; 
        std::cout << "znode create error ... path: " << path << std::endl ;
        return false ;
    } 
    return true ;
}

// 根据参数指定的 znode 节点路径 path ，获得 znode 节点的值
std::string ZkClient::GetNodeData(const char *path)
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

bool ZkClient::SetNodeData(const char *path , const char *value)
{
    int result = zoo_set(m_zhandle, path, value, strlen(value), -1) ;
    return result == ZOK ; 
}


// 等到一个可用的从服务器节点，并建立连接，返回文件描述符
std::shared_ptr<Socket> ZkClient::GetFollowerFd()
{
    
    // 轮询的方式分配，因为可能存在其他节点下线的情况，所以每次都是重新 Flush 下子节点服务列表
    String_vector followers;
    zoo_get_children(m_zhandle, path_.data(), 0, &followers);
    total_services_ = followers.count;
    std::string host_data;
    for (int i = 0; i < followers.count && i < current_service_; i++)
    {
        std::string znode_name = followers.data[i];
        std::string znode_path = path_ + "/" + znode_name;
        host_data = GetNodeData(znode_path.data());
    }
    current_service_ = (current_service_ + 1) % (total_services_ + 1);

    int host_index = host_data.find(":");
    if (host_index == -1)  return nullptr ; 

    //取得ip 和 port
    std::string ip = host_data.substr(0, host_index);
    uint16_t port = atoi(host_data.substr(host_index + 1, host_data.size() - host_index).c_str());

    //建立套接字并连接
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        close(client_fd);
        return nullptr;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        close(client_fd);
        return nullptr;
    }
    std::shared_ptr<Socket> socketFd(new Socket(client_fd)) ; 
    return socketFd;
}