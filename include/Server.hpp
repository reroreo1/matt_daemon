#ifndef SERVER_HPP
#define SERVER_HPP
#include "Tintin_reporter.hpp"

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
    
    int server_fd;
    std::vector<struct pollfd> poll_fds;
    bool accept_new_connection();
    void handle_client_message(int index);
    Tintin_reporter* reporter;
    bool should_quit;
};

#endif