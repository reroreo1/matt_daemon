#ifndef MATT_DAEMON_HPP
#define MATT_DAEMON_HPP
#include "Tintin_reporter.hpp"
#include "Server.hpp"

class MattDaemon {
public:
    MattDaemon();
    MattDaemon(const MattDaemon& other);
    MattDaemon& operator=(const MattDaemon& other);
    void start();
    ~MattDaemon();
    static void is_root();
    


private:
    // pid_t pid; 
    int lock_fd; 
    void deamonize();
    int lock_deamon();
    void setup_signals();
    void run();
    static  volatile sig_atomic_t signal_received;
    static void signal_handler(int signum);
    Tintin_reporter* reporter; 
    Server server;
};

#endif 
