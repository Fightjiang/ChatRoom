## Chat 集群的设计
ChatServer 是 ProxyService 的从服务器，专门负责转发消息，有以下几种情况
1. 如果用户需要发送给对方的 ClientFd 与 ProxyService 直接相连，那么可以直接转发消息给客户端，不用再经过 ChatServer 集群。
2. 如果没有直接相连，那么需要借助 ChatServer 集群服务器帮助转发消息，ChatServer 收到 ProxyService 发送过来的请求之后开始判断
    * 用户下线，则直接把发送的消息存入用户的离线消息表中。
    * 用户在线，则从 Redis 中拿到用户对应的建立连接的 ProxyService 服务器所在的 IP:Port ，并建立连接并将消息转发给该 Proxy 服务器，然后该 Proxy 服务器会转发给客户端


客户端都是与 ProxyServer 建立连接，所以如果转发的消息就在这台服务器上，那么则直接转发给客户端即可，如果没在这台服务器上则需要通过 chatServer 集群帮忙转发消息。