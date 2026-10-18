/*
 Navicat Premium Dump SQL

 Source Server         : mysql
 Source Server Type    : MySQL
 Source Server Version : 90700 (9.7.0)
 Source Host           : localhost:3306
 Source Schema         : xigu

 Target Server Type    : MySQL
 Target Server Version : 90700 (9.7.0)
 File Encoding         : 65001

 Date: 18/10/2026 21:04:15
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

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
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of friend
-- ----------------------------
BEGIN;
INSERT INTO `friend` (`id`, `self_id`, `friend_id`, `back`) VALUES (1, 1055, 1054, 'sqy');
INSERT INTO `friend` (`id`, `self_id`, `friend_id`, `back`) VALUES (4, 1003, 1004, 'test');
INSERT INTO `friend` (`id`, `self_id`, `friend_id`, `back`) VALUES (5, 1004, 1003, '');
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
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE KEY `from_to_uid` (`from_uid`,`to_uid`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of friend_apply
-- ----------------------------
BEGIN;
INSERT INTO `friend_apply` (`id`, `from_uid`, `to_uid`, `status`) VALUES (1, 1023, 1002, 0);
INSERT INTO `friend_apply` (`id`, `from_uid`, `to_uid`, `status`) VALUES (2, 1004, 1005, 0);
INSERT INTO `friend_apply` (`id`, `from_uid`, `to_uid`, `status`) VALUES (4, 1004, 1003, 1);
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
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci ROW_FORMAT=DYNAMIC;

-- ----------------------------
-- Records of user
-- ----------------------------
BEGIN;
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (2, 1002, 'xigu', '13939927271@163.com', '$argon2id$v=19$m=262144,t=3,p=1$7XDT8OsoKjmeCf87mn08Uw$MdY5hLxz1xL9EeZ+yymr63HtoNYn34TNqg+jCgJCIeI', 'xigu', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (3, 1003, '宵宫', '3099154616@qq.com', '$argon2id$v=19$m=262144,t=3,p=1$QNa1wGxsL3NPeSU/OJQgeQ$1zsbjcUvJ1ZAlUnREyytNEaNANXS6c867S4uSfWwxn8', '烟花', '', 1, ':/res/head_1.jpg');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (4, 1004, 'test', '123@123.com', '$argon2id$v=19$m=262144,t=3,p=1$6Aqhng8GVsv4a74WMcBSBw$FIXS69UYC9CgLhHIl7UNyo0m+0hjFRXmESI90x2AQMI', '', '', 0, '');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (5, 1005, 'test2', '456@456.com', '$argon2id$v=19$m=262144,t=3,p=1$z3UTKM2OqOnbYbg8rnMCwQ$BLxn9snebYF/KXkaAS7JQdQKGkJ++bdinDinH3QIrXw', '', '', 0, '');
INSERT INTO `user` (`id`, `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`) VALUES (9, 1006, 'test3', '789@789.com', '$argon2id$v=19$m=262144,t=3,p=1$fwsRs0vMX4osdc3hM/omxA$M0iswUz4y1WOXNL6iSla78PC8wDtTEW99lPfIUcL+KY', '', '', 0, '');
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
INSERT INTO `user_id` (`id`) VALUES (1006);
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
