#ifndef AUTH_HPP
#define AUTH_HPP
#include <string>
#include <unordered_map>

class Auth {
public:
    Auth();
    bool verify(const std::string& username, const std::string& password) const;

private:
    void load();
    std::unordered_map<std::string, std::string> credentials_;
    std::string path_;
    static constexpr const char* DEFAULT_PATH = "/etc/matt_daemon/users.conf";
};

#endif
