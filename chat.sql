CREATE TABLE `user` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) DEFAULT NULL,
  `password` varchar(50) DEFAULT NULL, 
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=22 DEFAULT CHARSET=utf8;

INSERT INTO `user` VALUES (1,'zhangsan','123456');
INSERT INTO `user` VALUES (2,'lisi','123456');
INSERT INTO `user` VALUES (3,'wangwu','123456');

CREATE TABLE `offlinemessage` (
  `id` int NOT NULL AUTO_INCREMENT, 
  `recvid` int(11) NOT NULL,
  `sendid` int(11) NOT NULL,
  `message` varchar(500) NOT NULL, 
  `status` ENUM('READ' , 'UNREAD') NOT NULL DEFAULT 'UNREAD',
  `datetime` DATETIME DEFAULT NOW(),
  PRIMARY KEY (`id`),
  INDEX idx_recvId (`recvid`) ,
  INDEX idx_status (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO `offlinemessage`(recvid , sendid , message) VALUE (1,2,"你好吖！！！") ;
INSERT INTO `offlinemessage`(recvid , sendid , message) VALUE (2,1,"吃饭了嘛？？") ;
INSERT INTO `offlinemessage`(recvid , sendid , message) VALUE (3,1,"你好，我是 王五") ;
INSERT INTO `offlinemessage`(recvid , sendid , message) VALUE (3,2,"你好，我是 李四") ;

/*
// 为什么后面又在 offlinemessage 表中加上了 id 主键？
1. 考虑以 recvid 或 (recvid , sendid) 联合索引做不了主键，因为可以存在重复的情况 ， Mysql 还是会有一个隐藏字段来建立主键
2. 如果读取离线消息成功，但是发送失败，还是要将 status 修改为 UNREAD ，这个时候最好的方式还是可以主键 ID 

// 为什么对 recvid 建立索引
1. 因为读取离线消息的时候，只需要看接收者就好了。

// 为什么对区分度低的 status 字段也建立索引
1. 因为 UNREAD 字段所在的比例应该很少，如果要查询系统里面又多少 UNREAD 的消息，这样也能优化查询效率，可加可不加，纯粹看业务需求

*/