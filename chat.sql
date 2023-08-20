CREATE TABLE `user` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) DEFAULT NULL,
  `password` varchar(50) DEFAULT NULL, 
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=22 DEFAULT CHARSET=utf8;

INSERT INTO `user` VALUES (1,'zhang san','123456');
INSERT INTO `user` VALUES (2,'li si','123456');
INSERT INTO `user` VALUES (3,'wang wu','123456');

CREATE TABLE `offlinemessage` (
  `userid` int(11) NOT NULL,
  `message` varchar(500) NOT NULL,
  `datetime` DATETIME DEFAULT NOW()
) ENGINE=InnoDB DEFAULT CHARSET=utf8;