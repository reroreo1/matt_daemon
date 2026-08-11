#include "../include/Server.hpp"



Server::Server() : server_fd(-1), should_quit(false) {
    reporter = &Tintin_reporter::instance();
    poll_fds.reserve(MAX_CLIENTS + 1); 
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
        close(client_fd);
        return false;
    }
    
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);
    
    return true;
}

void Server::handle_client_message(int index) {
    char buffer[1024] = {0};
    int client_fd = poll_fds[index].fd;
    
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        close(client_fd);
        poll_fds.erase(poll_fds.begin() + index);
        reporter->log("Client disconnected", Tintin_reporter::Level::INFO);
        return;
    }
    
    buffer[bytes_read] = '\0';
    
   
    std::string message(buffer);
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
    
    
    if (message == "quit") {
        reporter->log("Request quit.", Tintin_reporter::Level::INFO);
        should_quit = true;
        return;
    }
    
    
    reporter->log("User input: " + message, Tintin_reporter::Level::LOG);
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
                handle_client_message(i);
            }
        }
    }
    
    return true;
}

void Server::shutdown() {
    for (size_t i = 1; i < poll_fds.size(); i++) {
        close(poll_fds[i].fd);
    }
    

    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    poll_fds.clear();
}