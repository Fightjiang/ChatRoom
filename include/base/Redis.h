#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <string>
#include <string.h>
#include <functional>

 
class RedisCli
{
public:
    using redis_handler = std::function<void(int,std::string)>;

    // 初始化redis连接信息
    RedisCli();

    // 释放连接
    ~RedisCli();

    // 初始化get_channel、set_channel连接
    bool connect();

    // 判断某个键是否存在
    bool key_exist(const std::string& key);

    // 获得 key 对应的键值
    std::string get_key(const std::string& key);

    // 设置 key 对应的 value 值
    bool set_key(const std::string& key, const std::string& host);

    //删除 key 
    bool del_key(const std::string& key);


    // 分布式锁 加锁
    bool redisLock(const std::string& lock_key , const std::string& unique_value , const std::string& timeout = "100000") ;
    
    // 尝试解锁
    bool redisUnLock(const std::string& lock_key , const std::string& unique_value) ;
 

private:
    // hiredis同步上下文对象，负责获得 get 消息
    redisContext *get_channel_;

    // 负责 set 消息
    redisContext *set_channel_;

    // // 回调操作，收到消息给service上报
    // redis_handler notify_message_handler_;

    // redis 的 IP
    const char* ip_;

    // redis 的端口 Port
    uint16_t port_;
};

#endif