#include "encrypt.hpp"
#include <stdexcept>
#include <vector>

const std::string encrypt::Crypto::base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                  "abcdefghijklmnopqrstuvwxyz"
                                                  "0123456789+/";

const int encrypt::Crypto::base64_dec_table[256] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55,
                                                    56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                                                    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32,
                                                    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1};

encrypt::Crypto::Crypto() { OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr); }

encrypt::Crypto::~Crypto()
{
    EVP_cleanup();
    ERR_free_strings();
}

void encrypt::Crypto::handleOpenSSLError() const
{
    char err_buf[1024] = {0};
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    OATPP_LOGe("MyApp", "OpenSSL Error: {}", err_buf);
}

bool encrypt::Crypto::generateKey(Algorithm alg, std::pair<std::string, std::string>& output)
{
    switch (alg)
    {
    case Algorithm::SM4:
    {
        output.first.resize(SM4_KEY_SIZE);
        output.second.resize(SM4_IV_SIZE);
        if (RAND_bytes(reinterpret_cast<unsigned char*>(output.first.data()), output.first.size()) != 1 ||
            RAND_bytes(reinterpret_cast<unsigned char*>(output.second.data()), output.second.size()) != 1)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to generate SM4 key");
        }
        break;
    }
    case Algorithm::RSA:
    {
        EVP_PKEY_CTX_Ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
        if (!ctx)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to initialize RSA context");
        }

        if (EVP_PKEY_keygen_init(ctx.get()) != 1)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to init RSA keygen");
        }

        if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), RSA_KEY_BITS) != 1)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to set RSA key bits");
        }

        EVP_PKEY* pkey_raw = nullptr;
        if (EVP_PKEY_keygen(ctx.get(), &pkey_raw) != 1)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to generate RSA key");
        }
        EVP_PKEY_Ptr pkey(pkey_raw);

        // 导出公钥 (PEM 格式)
        BIO_Ptr pub_bio(BIO_new(BIO_s_mem()));
        if (!pub_bio)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to create BIO for public key");
        }
        PEM_write_bio_PUBKEY(pub_bio.get(), pkey.get());
        char* pub_key_ptr = nullptr;
        long pub_key_len = BIO_get_mem_data(pub_bio.get(), &pub_key_ptr);
        output.first = std::string(pub_key_ptr, pub_key_len);

        // 导出私钥 (PEM 格式)
        BIO_Ptr priv_bio(BIO_new(BIO_s_mem()));
        if (!priv_bio)
        {
            handleOpenSSLError();
            throw std::runtime_error("Error: Failed to create BIO for private key");
        }
        PEM_write_bio_PrivateKey(priv_bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr);
        char* priv_key_ptr = nullptr;
        long priv_key_len = BIO_get_mem_data(priv_bio.get(), &priv_key_ptr);
        output.second = std::string(priv_key_ptr, priv_key_len);
        break;
    }
    default:
    {
        return false;
    }
    }
    return true;
}

std::string encrypt::Crypto::encrypt_RSA(const std::string& plaintext, const std::string& public_key)
{
    BIO_Ptr pub_bio(BIO_new_mem_buf(public_key.data(), public_key.size()));
    if (!pub_bio)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create BIO for public key");
    }

    EVP_PKEY* pkey_raw = PEM_read_bio_PUBKEY(pub_bio.get(), nullptr, nullptr, nullptr);
    if (!pkey_raw)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to read public key");
    }
    EVP_PKEY_Ptr pkey(pkey_raw);

    EVP_PKEY_CTX_Ptr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (!ctx)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create encryption context");
    }

    if (EVP_PKEY_encrypt_init(ctx.get()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to init encryption");
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to set RSA padding");
    }

    size_t out_len = 0;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr, &out_len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to get encrypt length");
    }

    std::vector<unsigned char> ciphertext(out_len);
    if (EVP_PKEY_encrypt(ctx.get(), ciphertext.data(), &out_len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to encrypt");
    }

    return std::string(reinterpret_cast<char*>(ciphertext.data()), out_len);
}

std::string encrypt::Crypto::decrypt_RSA(const std::string& ciphertext, const std::string& private_key)
{
    BIO_Ptr priv_bio(BIO_new_mem_buf(private_key.data(), private_key.size()));
    if (!priv_bio)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create BIO for private key");
    }

    EVP_PKEY* pkey_raw = PEM_read_bio_PrivateKey(priv_bio.get(), nullptr, nullptr, nullptr);
    if (!pkey_raw)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to read private key");
    }
    EVP_PKEY_Ptr pkey(pkey_raw);

    EVP_PKEY_CTX_Ptr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (!ctx)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create decryption context");
    }

    if (EVP_PKEY_decrypt_init(ctx.get()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to init decryption");
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to set RSA padding");
    }

    size_t out_len = 0;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr, &out_len, reinterpret_cast<const unsigned char*>(ciphertext.data()), ciphertext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to get decrypt length");
    }

    std::vector<unsigned char> plaintext(out_len);
    if (EVP_PKEY_decrypt(ctx.get(), plaintext.data(), &out_len, reinterpret_cast<const unsigned char*>(ciphertext.data()), ciphertext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to decrypt");
    }

    return std::string(reinterpret_cast<char*>(plaintext.data()), out_len);
}

std::string encrypt::Crypto::encrypt_SM4CBC(const std::string& plaintext, const std::string& key, const std::string& iv)
{
    // 修复：统一抛异常
    if (key.size() != SM4_KEY_SIZE || iv.size() != SM4_IV_SIZE)
    {
        throw std::invalid_argument("Error: Key and IV must be 16 bytes for SM4-CBC");
    }

    EVP_CIPHER_CTX_Ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create cipher context");
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_sm4_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key.data()),
                           reinterpret_cast<const unsigned char*>(iv.data())) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to init encryption");
    }

    int block_size = EVP_CIPHER_CTX_block_size(ctx.get());
    std::vector<unsigned char> ciphertext(plaintext.size() + block_size);

    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to encrypt update");
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to encrypt final");
    }
    ciphertext_len += len;

    return std::string(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
}

std::string encrypt::Crypto::decrypt_SM4CBC(const std::string& ciphertext, const std::string& key, const std::string& iv)
{
    if (key.size() != SM4_KEY_SIZE || iv.size() != SM4_IV_SIZE)
    {
        throw std::invalid_argument("Error: Key and IV must be 16 bytes for SM4-CBC");
    }

    EVP_CIPHER_CTX_Ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to create cipher context");
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_sm4_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key.data()),
                           reinterpret_cast<const unsigned char*>(iv.data())) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to init decryption");
    }

    std::vector<unsigned char> plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH);

    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, reinterpret_cast<const unsigned char*>(ciphertext.data()), ciphertext.size()) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to decrypt update");
    }
    plaintext_len += len;

    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + plaintext_len, &len) != 1)
    {
        handleOpenSSLError();
        throw std::runtime_error("Error: Failed to decrypt final");
    }
    plaintext_len += len;

    return std::string(plaintext.begin(), plaintext.begin() + plaintext_len);
}

std::string encrypt::Crypto::base64Encode(const std::string& input)
{
    if (input.empty())
    {
        return "";
    }

    // 计算输出长度
    size_t encoded_len = 4 * ((input.size() + 2) / 3);
    std::string encoded(encoded_len, '\0');

    int result = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encoded.data()), reinterpret_cast<const unsigned char*>(input.data()), input.size());

    if (result < 0)
    {
        throw std::runtime_error("Error: Failed to base64 encode");
    }

    // 移除可能的尾随换行符
    while (!encoded.empty() && (encoded.back() == '\n' || encoded.back() == '\r' || encoded.back() == '\0'))
    {
        encoded.pop_back();
    }

    return encoded;
}

std::string encrypt::Crypto::base64Decode(const std::string& input)
{
    if (input.empty())
    {
        return "";
    }

    // 验证输入长度是否为 4 的倍数
    if (input.size() % 4 != 0)
    {
        throw std::invalid_argument("Invalid Base64 input length");
    }

    // 计算最大可能输出长度
    size_t max_decoded_len = (input.size() * 3) / 4;
    std::vector<unsigned char> decoded(max_decoded_len);

    int result = EVP_DecodeBlock(decoded.data(), reinterpret_cast<const unsigned char*>(input.data()), input.size());

    if (result < 0)
    {
        throw std::invalid_argument("Invalid Base64 character encountered");
    }

    // 根据填充符调整实际长度
    size_t padding = 0;
    if (input.size() >= 1 && input[input.size() - 1] == '=') padding++;
    if (input.size() >= 2 && input[input.size() - 2] == '=') padding++;

    return std::string(decoded.begin(), decoded.begin() + (result - padding));
}