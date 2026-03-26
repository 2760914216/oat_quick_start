#pragma once
#include <oatpp/base/Log.hpp>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <string>
#include <memory>

namespace encrypt
{

// 常量定义
constexpr size_t SM4_KEY_SIZE = 16;
constexpr size_t SM4_IV_SIZE = 16;
constexpr int RSA_KEY_BITS = 2048;

class Crypto
{
public:
    enum class Algorithm
    {
        RSA = 1,
        SM4 = 2,
    };

public:
    Crypto(const Crypto&) = delete;
    Crypto& operator=(const Crypto&) = delete;
    
    static Crypto& initialize()
    {
        static Crypto instance;
        return instance;
    }
    
    bool generateKey(Algorithm alg, std::pair<std::string, std::string>& output);
    
    std::string encrypt_RSA(const std::string& plaintext, const std::string& public_key);
    std::string decrypt_RSA(const std::string& ciphertext, const std::string& private_key);
    
    // 修复：iv 改为 const 引用
    std::string encrypt_SM4CBC(const std::string& plaintext, const std::string& key, const std::string& iv);
    std::string decrypt_SM4CBC(const std::string& ciphertext, const std::string& key, const std::string& iv);
    
    // 建议使用 OpenSSL 原生 Base64 接口
    std::string base64Encode(const std::string& input);
    std::string base64Decode(const std::string& input);

private:
    Crypto();
    ~Crypto();
    
    void handleOpenSSLError() const;
    
    // RAII 包装类
    struct EVP_PKEY_Deleter { void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); } };
    struct EVP_PKEY_CTX_Deleter { void operator()(EVP_PKEY_CTX* p) const { EVP_PKEY_CTX_free(p); } };
    struct BIO_Deleter { void operator()(BIO* p) const { BIO_free_all(p); } };
    struct EVP_CIPHER_CTX_Deleter { void operator()(EVP_CIPHER_CTX* p) const { EVP_CIPHER_CTX_free(p); } };
    
    using EVP_PKEY_Ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;
    using EVP_PKEY_CTX_Ptr = std::unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_Deleter>;
    using BIO_Ptr = std::unique_ptr<BIO, BIO_Deleter>;
    using EVP_CIPHER_CTX_Ptr = std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter>;
    
    static const std::string base64_chars;
    static const int base64_dec_table[256];
};

} // namespace encrypt