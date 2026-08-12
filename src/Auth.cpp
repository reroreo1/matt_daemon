#include "../include/Auth.hpp"
#include "../include/Tintin_reporter.hpp"
#include <fstream>
#include <sstream>
#include <crypt.h>
#include <cstdlib>

Auth::Auth()
    : path_(std::getenv("MATT_DAEMON_USERS_CONF") ? std::getenv("MATT_DAEMON_USERS_CONF") : DEFAULT_PATH) {
    load();
}

void Auth::load() {
    std::ifstream file(path_);
    if (!file.is_open()) {
        Tintin_reporter::instance().log(
            "Failed to load user credentials file.", Tintin_reporter::Level::ERROR);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos || line[start] == '#')
            continue;

        size_t sep = line.find(':', start);
        if (sep == std::string::npos)
            continue;

        std::string username = line.substr(start, sep - start);
        std::string hash = line.substr(sep + 1);
        size_t end = hash.find_last_not_of(" \t\r");
        if (end != std::string::npos)
            hash.erase(end + 1);

        if (!username.empty() && !hash.empty())
            credentials_[username] = hash;
    }
}

bool Auth::verify(const std::string& username, const std::string& password) const {
    auto it = credentials_.find(username);
    if (it == credentials_.end())
        return false;

    const std::string& stored_hash = it->second;
    char* result = crypt(password.c_str(), stored_hash.c_str());
    if (!result)
        return false;

    return stored_hash == result;
}
