#include "../include/Tintin_reporter.hpp"

Tintin_reporter& Tintin_reporter::instance() {
    static Tintin_reporter instance;
    return instance;
}

Tintin_reporter::Tintin_reporter() {
    check_directory();
    log_file_.open(PATH, std::ios::app);
    if (!log_file_.is_open()) {
        throw std::runtime_error("Failed to open log file");
    }
}

Tintin_reporter::~Tintin_reporter() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Tintin_reporter::check_directory() {
    std::string dir = PATH;
    dir = dir.substr(0, dir.find_last_of('/'));
    if (mkdir(dir.c_str(), 0755) && errno != EEXIST) {
        throw std::runtime_error("Failed to create log directory: " + std::string(strerror(errno)));
    }
}

std::string Tintin_reporter::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%d/%m/%Y %H:%M:%S");
    return ss.str();
}

void Tintin_reporter::log(const std::string& message, Level level) {
    if (!log_file_.is_open())
        return;

    std::string level_str;
    switch (level) {
        case Level::INFO:  level_str = "INFO"; break;
        case Level::ERROR: level_str = "ERROR"; break;
        case Level::LOG:   level_str = "LOG"; break;
        default:           level_str = "UNKNOWN"; break;
    }

    int fd = ::open(PATH, O_WRONLY | O_APPEND);
    if (fd != -1) {
        flock(fd, LOCK_EX);
    }

    log_file_ << "[" << get_timestamp() << "] "
              << "[" << level_str << "] " << "- Matt_daemon: "
              << message << std::endl;
    log_file_.flush();

    if (fd != -1) {
        flock(fd, LOCK_UN);
        ::close(fd);
    }
}