#include "PasswordHash.h"

#include <iostream>
#include <mutex>

#include <sodium.h>

namespace {
//Argon2id 的运算开销，取 libsodium 的"中等"档常量：
//兼顾口令破解成本与单次登录耗时，统一使用下面的计算量和内存开销参数
constexpr unsigned long long kOpsLimit = crypto_pwhash_OPSLIMIT_MODERATE;
constexpr std::size_t kMemLimit = crypto_pwhash_MEMLIMIT_MODERATE;

//libsodium 要求使用前先初始化，用 once_flag 保证只做一次且不依赖静态初始化顺序
void EnsureSodiumReady()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        //sodium_init 成功返回 0，失败返回 -1（此后哈希与校验都会失败）
        if (sodium_init() != 0) {
            std::cerr << "libsodium init failed" << std::endl;
        }
    });
}
}

std::string PasswordHash::Hash(const std::string& plain)
{
    EnsureSodiumReady();

    //crypto_pwhash_STRBYTES 已含结尾 '\0'，crypto_pwhash_str 保证以 '\0' 收尾
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(out, plain.data(), plain.size(), kOpsLimit, kMemLimit) != 0) {
        return {}; //参数非法或内存不足
    }
    return std::string(out);
}

bool PasswordHash::Verify(const std::string& plain, const std::string& stored)
{
    EnsureSodiumReady();

    //校验失败（含存储串格式不合法）返回非 0；比较内部为常量时间实现
    return crypto_pwhash_str_verify(stored.c_str(), plain.data(), plain.size()) == 0;
}
