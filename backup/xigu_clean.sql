/*
 Navicat Premium Dump SQL

 Source Server         : mysql
 Source Server Type    : MySQL
 Source Server Version : 90704 (9.7.4)
 Source Host           : localhost:3306
 Source Schema         : xigu

 Target Server Type    : MySQL
 Target Server Version : 90704 (9.7.4)
 File Encoding         : 65001

 Date: 04/11/2026 22:08:21
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for chat_message
-- ----------------------------
DROP TABLE IF EXISTS `chat_message`;
CREATE TABLE `chat_message` (
  `message_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `thread_id` bigint unsigned NOT NULL,
  `sender_id` int NOT NULL,
  `recv_id` int NOT NULL,
  `client_msg_id` varchar(64) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `content` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` tinyint unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`message_id`),
  UNIQUE KEY `uk_chat_client_msg` (`thread_id`,`sender_id`,`client_msg_id`),
  KEY `idx_chat_message_thread_id` (`thread_id`,`message_id`),
  KEY `idx_chat_message_thread_time` (`thread_id`,`created_at`,`message_id`),
  CONSTRAINT `fk_chat_message_thread` FOREIGN KEY (`thread_id`) REFERENCES `chat_thread` (`thread_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ----------------------------
-- Records of chat_message
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for chat_thread
-- ----------------------------
DROP TABLE IF EXISTS `chat_thread`;
CREATE TABLE `chat_thread` (
  `thread_id` bigint unsigned NOT NULL AUTO_INCREMENT,
  `thread_type` tinyint unsigned NOT NULL DEFAULT '1',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`),
  KEY `idx_chat_thread_updated` (`updated_at`,`thread_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ----------------------------
-- Records of chat_thread
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for friend
-- ----------------------------
DROP TABLE IF EXISTS `friend`;
CREATE TABLE `friend` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `self_id` int NOT NULL,
  `friend_id` int NOT NULL,
  `back` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `self_friend` (`self_id`,`friend_id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of friend
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for friend_apply
-- ----------------------------
DROP TABLE IF EXISTS `friend_apply`;
CREATE TABLE `friend_apply` (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `from_uid` int NOT NULL,
  `to_uid` int NOT NULL,
  `status` smallint NOT NULL DEFAULT '0',
  `descs` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `back_name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `from_to_uid` (`from_uid`,`to_uid`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of friend_apply
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for group_chat
-- ----------------------------
DROP TABLE IF EXISTS `group_chat`;
CREATE TABLE `group_chat` (
  `thread_id` bigint unsigned NOT NULL,
  `group_name` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `owner_id` int NOT NULL DEFAULT '0',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`),
  CONSTRAINT `fk_group_thread` FOREIGN KEY (`thread_id`) REFERENCES `chat_thread` (`thread_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ----------------------------
-- Records of group_chat
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for group_chat_member
-- ----------------------------
DROP TABLE IF EXISTS `group_chat_member`;
CREATE TABLE `group_chat_member` (
  `thread_id` bigint unsigned NOT NULL,
  `user_id` int NOT NULL,
  `role` tinyint unsigned NOT NULL DEFAULT '0',
  `joined_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`,`user_id`),
  KEY `idx_group_member_user` (`user_id`,`thread_id`),
  CONSTRAINT `fk_group_member_thread` FOREIGN KEY (`thread_id`) REFERENCES `group_chat` (`thread_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ----------------------------
-- Records of group_chat_member
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for private_chat
-- ----------------------------
DROP TABLE IF EXISTS `private_chat`;
CREATE TABLE `private_chat` (
  `thread_id` bigint unsigned NOT NULL,
  `user1_id` int NOT NULL,
  `user2_id` int NOT NULL,
  PRIMARY KEY (`thread_id`),
  UNIQUE KEY `uk_private_users` (`user1_id`,`user2_id`),
  KEY `idx_private_user1` (`user1_id`,`thread_id`),
  KEY `idx_private_user2` (`user2_id`,`thread_id`),
  CONSTRAINT `fk_private_thread` FOREIGN KEY (`thread_id`) REFERENCES `chat_thread` (`thread_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ----------------------------
-- Records of private_chat
-- ----------------------------
BEGIN;
COMMIT;

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
  `id` int NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL DEFAULT '0',
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `pwd` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nick` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `desc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `sex` int NOT NULL DEFAULT '0',
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `uid` (`uid`) USING BTREE,
  UNIQUE KEY `email` (`email`) USING BTREE,
  KEY `name` (`name`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of user
-- ----------------------------
BEGIN;
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (1, 1001, '西故', '3099154616@qq.com', '$argon2id$v=19$m=262144,t=3,p=1$bD899q54K8vMwy+5fc7JKA$/jVJP8RyQj0mFzJqY/56T2F9f3SB74NSBm9Lxc0XoZ8', 'xigu', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (2, 1002, '霄宫', 'xigu920@163.com', '$argon2id$v=19$m=262144,t=3,p=1$o/CzQVrUYHFP1kQbYhSztw$7KdjCxJlu0I7uDrmqqIivmuBUfII15hYy66h3Q6H870', '烟花', '', 1, ':/res/head_1.jpg');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (3, 1003, 'test1', '123@123.com', '$argon2id$v=19$m=262144,t=3,p=1$UhGmcWrXNJq/tMMW6g0gAQ$3SzQ/AyU/+Y7Vb+byrI9At5yXz9imXbNcrmZ7W4HNCU', '', '', 0, '');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (4, 1004, 'test2', '456@456.com', '$argon2id$v=19$m=262144,t=3,p=1$9plVfbgTD5bk1C99b6hlXA$jCdqshgcsz52get45glPtwwXX5zPjxEgbRs8rwcRAwk', '', '', 0, '');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (5, 1005, 'test3', '789@789.com', '$argon2id$v=19$m=262144,t=3,p=1$a+JGv9GlxLHO9ZOZbK0Nmw$kU0hSeyL/bAy8YielfzDMq3nYs+pKfeeCUqRXRvaN7A', '', '', 0, '');
COMMIT;

-- ----------------------------
-- Table structure for user_id
-- ----------------------------
DROP TABLE IF EXISTS `user_id`;
CREATE TABLE `user_id` (
  `id` int NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of user_id
-- ----------------------------
BEGIN;
INSERT INTO `user_id` (`id`) VALUES (1005);
COMMIT;

-- ----------------------------
-- Procedure structure for reg_user
-- ----------------------------
DROP PROCEDURE IF EXISTS `reg_user`;
delimiter ;;
CREATE PROCEDURE `reg_user`(IN `new_name` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
                            IN `new_email` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
                            IN `new_pwd` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci,
                            OUT `result` INT)
BEGIN
    -- 如果在执行过程中遇到任何错误，则回滚事务
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
BEGIN
        -- 回滚事务
ROLLBACK;
-- 设置返回值为-1，表示错误
SET result = -1;
END;
    -- 开始事务
    START TRANSACTION;
-- 检查用户名是否已存在
IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
        SET result = 0; -- 用户名已存在
COMMIT;
ELSE
        -- 用户名不存在，检查email是否已存在
        IF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
            SET result = 0; -- email已存在
COMMIT;
ELSE
            -- email也不存在，更新user_id表
UPDATE `user_id` SET `id` = `id` + 1;
-- 获取更新后的id
SELECT `id` INTO @new_id FROM `user_id`;
-- 在user表中插入新记录
INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) VALUES (@new_id, new_name, new_email, new_pwd);
-- 设置result为新插入的uid
SET result = @new_id; -- 插入成功，返回新的uid
COMMIT;
END IF;
END IF;
END
;;
delimiter ;

SET FOREIGN_KEY_CHECKS = 1;
