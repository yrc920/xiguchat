#pragma once
// 密码的哈希与校验（libsodium / Argon2id）
//
// 数据库中 user.pwd 存的是本类产出的字符串，形如：
// $argon2id$v=19$m=65536,t=2,p=1$<盐>$<哈希值>
// 盐与运算参数都内嵌在串里，因此不需要额外的盐列，也不必改表结构。

#include <string>

class PasswordHash {
public:
    //由明文密码生成可存储的字符串，失败返回空串
    static std::string Hash(const std::string& plain);
    //校验明文密码是否与该存储串匹配
    static bool Verify(const std::string& plain, const std::string& stored);
};
