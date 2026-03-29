#include "JwtHelper.hpp"
#include <cstring>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <vector>

namespace auth
{

std::string base64_encode_bytes(const unsigned char* bytes, size_t len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char3[3];
    unsigned char char4[4];

    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    while (len--)
    {
        char3[i++] = *(bytes++);
        if (i == 3)
        {
            char4[0] = (char3[0] & 0xfc) >> 2;
            char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
            char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
            char4[3] = char3[2] & 0x3f;

            for (i = 0; i < 4; i++)
            {
                ret += base64_chars[char4[i]];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
        {
            char3[j] = '\0';
        }
        char4[0] = (char3[0] & 0xfc) >> 2;
        char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
        char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
        {
            ret += base64_chars[char4[j]];
        }
        while ((i++ < 3))
        {
            ret += '=';
        }
    }

    return ret;
}

std::string base64_decode_str(const std::string& input)
{
    size_t len = input.size();
    int i = 0;
    int j = 0;
    int in = 0;
    unsigned char char4[4];
    std::string ret;

    const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";

    while (len-- && (input[in] != '='))
    {
        char c = input[in];
        int val = -1;
        for (int k = 0; k < 64 && val == -1; k++)
        {
            if (base64_chars[k] == c)
            {
                val = k;
            }
        }
        if (val == -1)
        {
            in++;
            continue;
        }
        char4[i++] = val;
        if (i == 4)
        {
            char4[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
            char4[1] = ((char4[1] & 0xf) << 4) + ((char4[2] & 0x3c) >> 2);
            char4[2] = ((char4[2] & 0x3) << 6) + char4[3];

            for (i = 0; i < 3; i++)
            {
                ret += char4[i];
            }
            i = 0;
        }
        in++;
    }

    if (i)
    {
        for (j = 0; j < i; j++)
        {
            char4[j] = 0;
        }
        char4[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
        char4[1] = ((char4[1] & 0xf) << 4) + ((char4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++)
        {
            ret += char4[j];
        }
    }

    return ret;
}

std::string replace_all(const std::string& str, const std::string& from, const std::string& to)
{
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos)
    {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

void JwtHelper::set_secret(const std::string& secret) { secret_ = secret; }

std::string JwtHelper::create_token(const std::string& username, const std::string& role, int expire_seconds)
{
    if (secret_.empty())
    {
        return "";
    }

    int64_t now = std::time(nullptr);
    int64_t exp = now + expire_seconds;

    std::string header_json = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string payload_json = "{"
                               "\"sub\":\"" +
                               username +
                               "\","
                               "\"role\":\"" +
                               role +
                               "\","
                               "\"iat\":" +
                               std::to_string(now) +
                               ","
                               "\"exp\":" +
                               std::to_string(exp) + "}";

    std::string header_b64 = base64_encode_bytes(reinterpret_cast<const unsigned char*>(header_json.c_str()), header_json.length());
    std::string payload_b64 = base64_encode_bytes(reinterpret_cast<const unsigned char*>(payload_json.c_str()), payload_json.length());

    header_b64 = replace_all(header_b64, "+", "-");
    header_b64 = replace_all(header_b64, "/", "_");
    header_b64 = replace_all(header_b64, "=", "");

    payload_b64 = replace_all(payload_b64, "+", "-");
    payload_b64 = replace_all(payload_b64, "/", "_");
    payload_b64 = replace_all(payload_b64, "=", "");

    std::string signing_input = header_b64 + "." + payload_b64;

    unsigned char* digest = HMAC(EVP_sha256(), secret_.c_str(), secret_.length(), reinterpret_cast<const unsigned char*>(signing_input.c_str()),
                                 signing_input.length(), nullptr, nullptr);

    std::string signature = base64_encode_bytes(digest, 32);
    signature = replace_all(signature, "+", "-");
    signature = replace_all(signature, "/", "_");
    signature = replace_all(signature, "=", "");

    return signing_input + "." + signature;
}

std::optional<TokenPayload> JwtHelper::verify_token(const std::string& token)
{
    if (secret_.empty())
    {
        return std::nullopt;
    }

    size_t dot1 = token.find('.');
    size_t dot2 = token.find('.', dot1 + 1);

    if (dot1 == std::string::npos || dot2 == std::string::npos)
    {
        return std::nullopt;
    }

    std::string header_b64 = token.substr(0, dot1);
    std::string payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string signature = token.substr(dot2 + 1);

    std::string signing_input = header_b64 + "." + payload_b64;

    unsigned char* digest = HMAC(EVP_sha256(), secret_.c_str(), secret_.length(), reinterpret_cast<const unsigned char*>(signing_input.c_str()),
                                 signing_input.length(), nullptr, nullptr);

    std::string expected_sig = base64_encode_bytes(digest, 32);
    expected_sig = replace_all(expected_sig, "+", "-");
    expected_sig = replace_all(expected_sig, "/", "_");
    expected_sig = replace_all(expected_sig, "=", "");

    if (signature != expected_sig)
    {
        return std::nullopt;
    }

    std::string decoded_payload = base64_decode_str(payload_b64 + std::string((4 - payload_b64.length() % 4) % 4, '='));

    TokenPayload payload;
    payload.username = "";
    payload.role = "";
    payload.iat = 0;
    payload.exp = 0;

    size_t pos = 0;
    auto parse_value = [&](const std::string& key) -> std::string
    {
        std::string search = "\"" + key + "\":";
        size_t key_pos = decoded_payload.find(search, pos);
        if (key_pos == std::string::npos)
        {
            return "";
        }
        size_t val_start = decoded_payload.find("\"", key_pos + search.length());
        if (val_start == std::string::npos)
        {
            return "";
        }
        val_start++;
        size_t val_end = decoded_payload.find("\"", val_start);
        if (val_end == std::string::npos)
        {
            return "";
        }
        pos = val_end + 1;
        return decoded_payload.substr(val_start, val_end - val_start);
    };

    auto parse_int = [&](const std::string& key) -> int64_t
    {
        std::string search = "\"" + key + "\":";
        size_t key_pos = decoded_payload.find(search, pos);
        if (key_pos == std::string::npos)
        {
            return 0;
        }
        size_t val_start = key_pos + search.length();
        while (val_start < decoded_payload.length() && (decoded_payload[val_start] == ' ' || decoded_payload[val_start] == '\t'))
        {
            val_start++;
        }
        size_t val_end = val_start;
        while (val_end < decoded_payload.length() && std::isdigit(decoded_payload[val_end]))
        {
            val_end++;
        }
        pos = val_end;
        return std::stoll(decoded_payload.substr(val_start, val_end - val_start));
    };

    payload.username = parse_value("sub");
    payload.role = parse_value("role");
    payload.iat = parse_int("iat");
    payload.exp = parse_int("exp");

    int64_t now = std::time(nullptr);
    if (payload.exp < now)
    {
        return std::nullopt;
    }

    return payload;
}

} // namespace auth
