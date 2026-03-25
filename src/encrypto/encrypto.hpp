#pragma once
#include <string>

namespace encrypt
{
class Crypto
{
    bool generateKey(/**/);
    std::string encrypt(const std::string& plaintext/*key*/);
    std::string decrypt(const std::string& ciphertextBase64/*key*/);
    std::string getBase64Key();
};
} // namespace encrypt