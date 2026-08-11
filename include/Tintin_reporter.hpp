#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <sys/file.h>
#include <chrono>
#include <iomanip>
#include <stdexcept>
#include <memory>
#include <csignal> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <poll.h>
#include <algorithm>
class Tintin_reporter {
public:
    static Tintin_reporter& instance(); 
    enum class Level { INFO, ERROR, LOG };
    Tintin_reporter(const Tintin_reporter& other);
    Tintin_reporter& operator=(const Tintin_reporter& other);
    void log(const std::string& message, Level level);
    ~Tintin_reporter();
    
private:
    Tintin_reporter();
    std::string get_timestamp();
    std::ofstream log_file_;
    void check_directory();
    static constexpr const char* PATH = "/var/log/matt_daemon/matt_daemon.log";
};

#endif
