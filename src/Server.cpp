#include "../include/Server.hpp"
#include <sstream>
#include <iomanip>



Server::Server() : server_fd(-1), should_quit(false) {
    reporter = &Tintin_reporter::instance();
    poll_fds.reserve(MAX_CLIENTS + 1);
    clients_.reserve(MAX_CLIENTS);
}

Server::Server(const Server& other) {
    (void)other;
}

Server& Server::operator=(const Server& other) {
    (void)other;
    return *this;
}

Server::~Server() {
    shutdown();
}

bool Server::init() {

    start_time_ = std::chrono::steady_clock::now();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        reporter->log("Socket creation failed", Tintin_reporter::Level::ERROR);
        return false;
    }


    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        reporter->log("setsockopt failed", Tintin_reporter::Level::ERROR);
        close(server_fd);
        server_fd = -1;
        return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        reporter->log("Bind failed", Tintin_reporter::Level::ERROR);
        close(server_fd);
        server_fd = -1;
        return false;
    }


    if (listen(server_fd, MAX_CLIENTS) < 0) {
        reporter->log("Listen failed", Tintin_reporter::Level::ERROR);
        close(server_fd);
        server_fd = -1;
        return false;
    }


    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);

    return true;
}

void Server::send_line(int fd, const std::string& text) {
    send(fd, text.c_str(), text.size(), MSG_NOSIGNAL);
}

bool Server::accept_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        reporter->log("Accept failed", Tintin_reporter::Level::ERROR);
        return false;
    }

    if (poll_fds.size() - 1 >= MAX_CLIENTS) {
        reporter->log("Maximum clients reached, rejecting connection", Tintin_reporter::Level::INFO);
        send_line(client_fd, "Server full. Try again later.\n");
        ::shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        return false;
    }

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);
    clients_.push_back({ClientState::AWAITING_USERNAME, ""});

    send_line(client_fd, "Username:\n");

    return true;
}

void Server::disconnect_client(int index) {
    close(poll_fds[index].fd);
    poll_fds.erase(poll_fds.begin() + index);
    clients_.erase(clients_.begin() + (index - 1));
}

void Server::handle_client_message(int index) {
    char buffer[1024] = {0};
    int client_fd = poll_fds[index].fd;

    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        disconnect_client(index);
        reporter->log("Client disconnected", Tintin_reporter::Level::INFO);
        return;
    }

    buffer[bytes_read] = '\0';


    std::string message(buffer);
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());

    ClientInfo& info = clients_[index - 1];

    switch (info.state) {
        case ClientState::AWAITING_USERNAME:
            info.pending_username = message;
            reporter->log("Client authentication attempt.", Tintin_reporter::Level::INFO);
            send_line(client_fd, "Password:\n");
            info.state = ClientState::AWAITING_PASSWORD;
            break;

        case ClientState::AWAITING_PASSWORD:
            if (auth_.verify(info.pending_username, message)) {
                info.state = ClientState::AUTHENTICATED;
                reporter->log("Client authenticated successfully.", Tintin_reporter::Level::INFO);
                send_line(client_fd, "Authentication successful.\nWelcome to Matt_daemon.\n");
            } else {
                reporter->log("Authentication failed.", Tintin_reporter::Level::WARN);
                send_line(client_fd, "Authentication failed.\n");
                disconnect_client(index);
            }
            break;

        case ClientState::AUTHENTICATED:
            handle_command(index, message);
            break;
    }
}

void Server::handle_command(int index, const std::string& command) {
    int fd = poll_fds[index].fd;

    if (command == "quit") {
        reporter->log("Request quit.", Tintin_reporter::Level::INFO);
        should_quit = true;
        return;
    }

    reporter->log("Command received: " + command + ".", Tintin_reporter::Level::LOG);

    if (command == "help") {
        send_line(fd,
            "Available commands:\n"
            "  help\n"
            "  status\n"
            "  uptime\n"
            "  clients\n"
            "  logs\n"
            "  quit\n");
    } else if (command == "status") {
        std::ostringstream oss;
        oss << "Matt_daemon status: running\n"
            << "PID: " << getpid() << "\n"
            << "Port: " << PORT << "\n"
            << "Connected clients: " << clients_.size() << "/" << MAX_CLIENTS << "\n";
        send_line(fd, oss.str());
    } else if (command == "uptime") {
        send_line(fd, "Daemon uptime: " + format_uptime() + "\n");
    } else if (command == "clients") {
        send_line(fd, "Connected clients: " + std::to_string(clients_.size())
                        + "/" + std::to_string(MAX_CLIENTS) + "\n");
    } else if (command == "logs") {
        std::ostringstream oss;
        oss << "Last 10 log entries:\n";
        for (const auto& line : reporter->tail(10))
            oss << line << "\n";
        send_line(fd, oss.str());
    } else {
        send_line(fd, "Unknown command. Type 'help' for available commands.\n");
    }
}

std::string Server::format_uptime() const {
    auto now = std::chrono::steady_clock::now();
    long secs = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    long hours = secs / 3600;
    long mins = (secs % 3600) / 60;
    long s = secs % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setw(2) << mins << ":" << std::setw(2) << s;
    return oss.str();
}

bool Server::run(volatile sig_atomic_t& signal_received) {
    if (server_fd < 0) {
        reporter->log("Server not initialized", Tintin_reporter::Level::ERROR);
        return false;
    }

    const int POLL_TIMEOUT = 1000;

    while (!should_quit && signal_received == 0) {
        int poll_result = poll(poll_fds.data(), poll_fds.size(), POLL_TIMEOUT);

        if (poll_result < 0) {
            if (errno == EINTR) {

                continue;
            }
            reporter->log("Poll failed: " + std::string(strerror(errno)), Tintin_reporter::Level::ERROR);
            break;
        }

        if (poll_result == 0) {

            continue;
        }

        if (poll_fds[0].revents & POLLIN) {
            accept_new_connection();
        }

        for (size_t i = 1; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLIN) {
                size_t before = poll_fds.size();
                handle_client_message(i);
                if (poll_fds.size() < before) {
                    i--;
                }
            }
        }
    }

    return true;
}

void Server::shutdown() {
    for (size_t i = 1; i < poll_fds.size(); i++) {
        send_line(poll_fds[i].fd, "Server is shutting down. Goodbye.\n");
        ::shutdown(poll_fds[i].fd, SHUT_RDWR);
        close(poll_fds[i].fd);
    }


    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    poll_fds.clear();
    clients_.clear();
}
