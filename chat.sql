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
