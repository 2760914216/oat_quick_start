#include "ConfigManager.hpp"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace
{

std::string trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start))
    {
        start++;
    }
    auto end = s.end();
    while (end != s.begin() && std::isspace(*(end - 1)))
    {
        end--;
    }
    return std::string(start, end);
}

std::string get_current_timestamp()
{
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    return buf;
}

std::string escape_json_string(const std::string& s)
{
    std::string result;
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
        }
    }
    return result;
}

} // namespace

namespace config
{

bool ConfigManager::load(const std::string& path)
{
    std::unique_lock lock(mutex_);
    configPath_ = path;

    if (!fs::exists(path))
    {
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    size_t pos = 0;
    auto skip_whitespace_and_newlines = [&]()
    {
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n' || content[pos] == '\r'))
        {
            pos++;
        }
    };
    auto expect = [&](char c)
    {
        skip_whitespace_and_newlines();
        if (pos < content.size() && content[pos] == c)
        {
            pos++;
            return true;
        }
        return false;
    };
    auto parse_string = [&]()
    {
        skip_whitespace_and_newlines();
        if (pos < content.size() && content[pos] == '"')
        {
            pos++;
            std::string result;
            while (pos < content.size() && content[pos] != '"')
            {
                if (content[pos] == '\\' && pos + 1 < content.size())
                {
                    pos++;
                    switch (content[pos])
                    {
                    case '"':
                        result += '"';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case 'n':
                        result += '\n';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    default:
                        result += content[pos];
                    }
                }
                else
                {
                    result += content[pos];
                }
                pos++;
            }
            if (pos < content.size())
            {
                pos++;
            }
            return result;
        }
        return std::string();
    };
    auto parse_int = [&]()
    {
        skip_whitespace_and_newlines();
        std::string num;
        while (pos < content.size() && (std::isdigit(content[pos]) || content[pos] == '-'))
        {
            num += content[pos];
            pos++;
        }
        return num.empty() ? 0 : std::stoi(num);
    };

    if (!expect('{'))
    {
        return false;
    }

    bool in_server = false, in_jwt = false, in_admin = false, in_users = false;
    std::string current_key;

    while (pos < content.size() && content[pos] != '}')
    {
        skip_whitespace_and_newlines();
        if (content[pos] == '"')
        {
            std::string key = parse_string();
            skip_whitespace_and_newlines();
            if (pos < content.size() && content[pos] == ':')
            {
                pos++;
                skip_whitespace_and_newlines();

                if (key == "server")
                {
                    if (expect('{'))
                    {
                        in_server = true;
                        while (pos < content.size() && content[pos] != '}')
                        {
                            skip_whitespace_and_newlines();
                            if (content[pos] == '"')
                            {
                                std::string k = parse_string();
                                skip_whitespace_and_newlines();
                                if (pos < content.size() && content[pos] == ':')
                                {
                                    pos++;
                                    if (k == "host")
                                    {
                                        serverConfig_.host = parse_string();
                                    }
                                    else if (k == "port")
                                    {
                                        serverConfig_.port = parse_int();
                                    }
                                }
                            }
                            skip_whitespace_and_newlines();
                            if (content[pos] == '}')
                            {
                                pos++;
                                in_server = false;
                                break;
                            }
                            if (content[pos] == ',')
                            {
                                pos++;
                            }
                        }
                    }
                }
                else if (key == "jwt")
                {
                    if (expect('{'))
                    {
                        in_jwt = true;
                        while (pos < content.size() && content[pos] != '}')
                        {
                            skip_whitespace_and_newlines();
                            if (content[pos] == '"')
                            {
                                std::string k = parse_string();
                                skip_whitespace_and_newlines();
                                if (pos < content.size() && content[pos] == ':')
                                {
                                    pos++;
                                    if (k == "secret")
                                    {
                                        jwtConfig_.secret = parse_string();
                                    }
                                    else if (k == "access_token_expire")
                                    {
                                        jwtConfig_.access_token_expire = parse_int();
                                    }
                                    else if (k == "refresh_token_expire")
                                    {
                                        jwtConfig_.refresh_token_expire = parse_int();
                                    }
                                }
                            }
                            skip_whitespace_and_newlines();
                            if (content[pos] == '}')
                            {
                                pos++;
                                in_jwt = false;
                                break;
                            }
                            if (content[pos] == ',')
                            {
                                pos++;
                            }
                        }
                    }
                }
                else if (key == "admin")
                {
                    if (expect('{'))
                    {
                        in_admin = true;
                        while (pos < content.size() && content[pos] != '}')
                        {
                            skip_whitespace_and_newlines();
                            if (content[pos] == '"')
                            {
                                std::string k = parse_string();
                                skip_whitespace_and_newlines();
                                if (pos < content.size() && content[pos] == ':')
                                {
                                    pos++;
                                    if (k == "username")
                                    {
                                        adminConfig_.username = parse_string();
                                    }
                                    else if (k == "password_hash")
                                    {
                                        adminConfig_.password_hash = parse_string();
                                    }
                                }
                            }
                            skip_whitespace_and_newlines();
                            if (content[pos] == '}')
                            {
                                pos++;
                                in_admin = false;
                                break;
                            }
                            if (content[pos] == ',')
                            {
                                pos++;
                            }
                        }
                    }
                }
                else if (key == "users")
                {
                    if (expect('['))
                    {
                        in_users = true;
                        users_.clear();
                        while (pos < content.size() && content[pos] != ']')
                        {
                            skip_whitespace_and_newlines();
                            if (content[pos] == '{')
                            {
                                pos++;
                                UserInfo user;
                                while (pos < content.size() && content[pos] != '}')
                                {
                                    skip_whitespace_and_newlines();
                                    if (content[pos] == '"')
                                    {
                                        std::string k = parse_string();
                                        skip_whitespace_and_newlines();
                                        if (pos < content.size() && content[pos] == ':')
                                        {
                                            pos++;
                                            if (k == "username")
                                            {
                                                user.username = parse_string();
                                            }
                                            else if (k == "password_hash")
                                            {
                                                user.password_hash = parse_string();
                                            }
                                            else if (k == "created_at")
                                            {
                                                user.created_at = parse_string();
                                            }
                                        }
                                    }
                                    skip_whitespace_and_newlines();
                                    if (content[pos] == '}')
                                    {
                                        pos++;
                                        break;
                                    }
                                    if (content[pos] == ',')
                                    {
                                        pos++;
                                    }
                                }
                                if (!user.username.empty())
                                {
                                    users_.push_back(user);
                                }
                            }
                            skip_whitespace_and_newlines();
                            if (content[pos] == ']')
                            {
                                break;
                            }
                            if (content[pos] == ',')
                            {
                                pos++;
                            }
                        }
                        if (pos < content.size())
                        {
                            pos++;
                        }
                    }
                }
                else
                {
                    parse_string();
                }
            }
        }
        skip_whitespace_and_newlines();
        if (content[pos] == '}')
        {
            break;
        }
        if (content[pos] == ',')
        {
            pos++;
        }
    }

    return true;
}

bool ConfigManager::save()
{
    std::unique_lock lock(mutex_);
    std::ofstream file(configPath_);
    if (!file.is_open())
    {
        return false;
    }

    file << "{\n";
    file << "  \"server\": {\n";
    file << "    \"host\": \"" << escape_json_string(serverConfig_.host) << "\",\n";
    file << "    \"port\": " << serverConfig_.port << "\n";
    file << "  },\n";
    file << "  \"jwt\": {\n";
    file << "    \"secret\": \"" << escape_json_string(jwtConfig_.secret) << "\",\n";
    file << "    \"access_token_expire\": " << jwtConfig_.access_token_expire << ",\n";
    file << "    \"refresh_token_expire\": " << jwtConfig_.refresh_token_expire << "\n";
    file << "  },\n";
    file << "  \"admin\": {\n";
    file << "    \"username\": \"" << escape_json_string(adminConfig_.username) << "\",\n";
    file << "    \"password_hash\": \"" << escape_json_string(adminConfig_.password_hash) << "\"\n";
    file << "  },\n";
    file << "  \"users\": [\n";

    for (size_t i = 0; i < users_.size(); ++i)
    {
        const auto& user = users_[i];
        file << "    {\n";
        file << "      \"username\": \"" << escape_json_string(user.username) << "\",\n";
        file << "      \"password_hash\": \"" << escape_json_string(user.password_hash) << "\",\n";
        file << "      \"created_at\": \"" << escape_json_string(user.created_at) << "\"\n";
        file << "    }";
        if (i < users_.size() - 1)
        {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";
    file.close();
    return true;
}

std::string ConfigManager::get_jwt_secret() const
{
    std::shared_lock lock(mutex_);
    return jwtConfig_.secret;
}

int ConfigManager::get_access_token_expire() const
{
    std::shared_lock lock(mutex_);
    return jwtConfig_.access_token_expire;
}

int ConfigManager::get_refresh_token_expire() const
{
    std::shared_lock lock(mutex_);
    return jwtConfig_.refresh_token_expire;
}

int ConfigManager::get_server_port() const
{
    std::shared_lock lock(mutex_);
    return serverConfig_.port;
}

std::string ConfigManager::get_server_host() const
{
    std::shared_lock lock(mutex_);
    return serverConfig_.host;
}

bool ConfigManager::validate_user(const std::string& username, const std::string& password_hash)
{
    std::shared_lock lock(mutex_);
    if (adminConfig_.username == username)
    {
        return adminConfig_.password_hash == password_hash;
    }
    for (const auto& user : users_)
    {
        if (user.username == username)
        {
            return user.password_hash == password_hash;
        }
    }
    return false;
}

bool ConfigManager::validate_admin(const std::string& username, const std::string& password_hash)
{
    std::shared_lock lock(mutex_);
    return adminConfig_.username == username && adminConfig_.password_hash == password_hash;
}

bool ConfigManager::is_admin(const std::string& username) const
{
    std::shared_lock lock(mutex_);
    return adminConfig_.username == username;
}

std::vector<UserInfo> ConfigManager::get_all_users() const
{
    std::shared_lock lock(mutex_);
    return users_;
}

bool ConfigManager::add_user(const std::string& username, const std::string& password_hash)
{
    std::unique_lock lock(mutex_);
    if (adminConfig_.username == username)
    {
        return false;
    }
    for (const auto& user : users_)
    {
        if (user.username == username)
        {
            return false;
        }
    }
    UserInfo newUser;
    newUser.username = username;
    newUser.password_hash = password_hash;
    newUser.created_at = get_current_timestamp();
    users_.push_back(newUser);
    return save();
}

bool ConfigManager::remove_user(const std::string& username)
{
    std::unique_lock lock(mutex_);
    if (adminConfig_.username == username)
    {
        return false;
    }
    auto it = std::remove_if(users_.begin(), users_.end(), [&username](const UserInfo& user) { return user.username == username; });
    if (it == users_.end())
    {
        return false;
    }
    users_.erase(it, users_.end());
    return save();
}

bool ConfigManager::user_exists(const std::string& username) const
{
    std::shared_lock lock(mutex_);
    if (adminConfig_.username == username)
    {
        return true;
    }
    for (const auto& user : users_)
    {
        if (user.username == username)
        {
            return true;
        }
    }
    return false;
}

} // namespace config
