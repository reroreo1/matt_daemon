#include "../include/Tintin_reporter.hpp"
#include <cstdio>
#include <cstdlib>

Tintin_reporter& Tintin_reporter::instance() {
    static Tintin_reporter instance;
    return instance;
}

Tintin_reporter::Tintin_reporter()
    : path_(std::getenv("MATT_DAEMON_LOG_PATH") ? std::getenv("MATT_DAEMON_LOG_PATH") : DEFAULT_PATH),
      max_log_size_(DEFAULT_MAX_LOG_SIZE), max_log_files_(DEFAULT_MAX_LOG_FILES) {
    if (const char* size_env = std::getenv("MATT_DAEMON_MAX_LOG_SIZE")) {
        long size = std::atol(size_env);
        if (size > 0)
            max_log_size_ = static_cast<size_t>(size);
    }
    if (const char* files_env = std::getenv("MATT_DAEMON_MAX_LOG_FILES")) {
        int files = std::atoi(files_env);
        if (files > 0)
            max_log_files_ = files;
    }

    check_directory();
    log_file_.open(path_, std::ios::app);
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
    std::string dir = path_;
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
        case Level::WARN:  level_str = "WARN"; break;
        default:           level_str = "UNKNOWN"; break;
    }

    int fd = ::open(path_.c_str(), O_WRONLY | O_APPEND);
    if (fd != -1) {
        flock(fd, LOCK_EX);
    }

    struct stat st;
    if (::stat(path_.c_str(), &st) == 0 && static_cast<size_t>(st.st_size) >= max_log_size_) {
        rotate_logs();
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

void Tintin_reporter::rotate_logs() {
    if (log_file_.is_open())
        log_file_.close();

    std::string oldest = path_ + "." + std::to_string(max_log_files_);
    std::remove(oldest.c_str());

    for (int i = max_log_files_ - 1; i >= 1; --i) {
        std::string from = path_ + "." + std::to_string(i);
        std::string to = path_ + "." + std::to_string(i + 1);
        std::rename(from.c_str(), to.c_str());
    }

    std::rename(path_.c_str(), (path_ + ".1").c_str());

    log_file_.open(path_, std::ios::app);
    if (!log_file_.is_open())
        return;

    log_file_ << "[" << get_timestamp() << "] "
              << "[INFO] - Matt_daemon: Rotating log file." << std::endl;
    log_file_.flush();
}

std::vector<std::string> Tintin_reporter::tail(size_t n) {
    std::vector<std::string> lines;

    int fd = ::open(path_.c_str(), O_RDONLY);
    if (fd == -1)
        return lines;
    flock(fd, LOCK_SH);

    std::ifstream in(path_);
    if (in.is_open()) {
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
            if (lines.size() > n)
                lines.erase(lines.begin());
        }
    }

    flock(fd, LOCK_UN);
    ::close(fd);
    return lines;
}