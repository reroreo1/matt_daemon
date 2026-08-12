#ifndef SERVER_HPP
#define SERVER_HPP
#include "Tintin_reporter.hpp"
#include "Auth.hpp"
#include <chrono>

class Server {
public:
    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);
    ~Server();

    bool init();
    bool run(volatile sig_atomic_t& signal_received);
    void shutdown();

private:
    static const int PORT = 4242;
    static const int MAX_CLIENTS = 3;

    enum class ClientState { AWAITING_USERNAME, AWAITING_PASSWORD, AUTHENTICATED };
    struct ClientInfo {
        ClientState state;
        std::string pending_username;
    };

    int server_fd;
    std::vector<struct pollfd> poll_fds;
    std::vector<ClientInfo> clients_;
    std::chrono::steady_clock::time_point start_time_;
    Auth auth_;
    bool accept_new_connection();
    void handle_client_message(int index);
    void handle_command(int index, const std::string& command);
    void disconnect_client(int index);
    void send_line(int fd, const std::string& text);
    std::string format_uptime() const;
    Tintin_reporter* reporter;
    bool should_quit;
};

#endif