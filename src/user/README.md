## User 集群的设计
我们对外提供一个主节点 UserMaster 负责向 UserFollower 去分发请求，从而实现集群的负载均衡能力。 UserFollower 是真正负责处理请求响应的节点，它负责与 Redis、Mysql 进行交互，并且将自己的服务器信息注册到 Zookeeper 上以便供 UserMaster 获取和转发请求。

### 主节点：UserMaster
所有关于 登录、注册、注销、的业务请求都会在这里得到中转，按照轮询的方式每次都选取一个可用的 UserFollower 节点处理请求，然后接收 UserFollower 的请求转发给回去。
![主从结构图](image.png)

等待贴代码

### 从节点：UserFollower
UserFollower 是整个服务集群中真正提供请求响应处理的节点，提供了登录、注册、注销三个功能。当一个业务请求通过 UserMaster 分发到 UserFollower 上的时候，它会先去反序列化请求，根据具体的请求类型进行业务处理，并返回响应的结果。

![请求流程图](image-1.png)

服务注册到 Zookeeper 上供 UserMaster 发现的代码：

示例处理 Login 请求的代码：
```C++
// 好像没有需要特别用到分布式锁的场景，用户的在线和注销的状态都是直接放在 Redis 中的，注册因为唯一索引 name 的存在，故可以通过数据库索引来保持一致性

bool UserFollower::Login(int id , string password)
{
// 先验证密码是否正确
// 尝试写入 userId 登录信息到 Redis 中，默认在线时间是 10 小时
// 更加正确的做法应该还是要与客户端建立一个心跳包，定时检查客户端是否还在线，如果不在线就把 Redis 中的信息删掉，避免由于网络的请求，客户端早就不在线了而服务器却没有发现，导致后续客户端的登录请求无法处理。

redisLock(userId , IP , 10*60*60*1000);
if false 则说明已经在线，则返回客户端已经在线登录。
}

// 注销
void UserFollower::LoginOut(int id) 
{
    redisUnlock(userId , IP) ; 
}

// 注册
int UserFollower::Register(string name , string password) 
{
    // 正常注册就可以了，因为 name 上加了唯一的索引，所以也不用担心出现冲突的情况
}
```
