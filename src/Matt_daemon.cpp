#include "../include/Matt_daemon.hpp"
#include <signal.h>



volatile sig_atomic_t MattDaemon::signal_received = 0;

MattDaemon::MattDaemon(){
}
MattDaemon::MattDaemon(const MattDaemon& other) {
    (void)other;
}
  
MattDaemon& MattDaemon::operator=(const MattDaemon& other) { 
    (void)other;
    return *this; 
}
    
MattDaemon::~MattDaemon() {
    if (reporter) {
        reporter->log("Quitting.", Tintin_reporter::Level::INFO);
    }
    if (lock_fd > 0) {
        close(lock_fd);
    }
    unlink("/var/lock/matt_deamon.lock");
}

void MattDaemon::is_root() {
    if (getuid() != 0) {
        throw std::runtime_error("This daemon must be run as root.");
    }
}


void  MattDaemon::start() {
    reporter = &Tintin_reporter::instance();  
    reporter->log("Started.", Tintin_reporter::Level::INFO);  
    lock_fd = lock_deamon();
    deamonize();
    pid_t pid = getpid();
    reporter->log("started. PID: " + std::to_string(pid), Tintin_reporter::Level::INFO);
    run();
}


void MattDaemon::signal_handler(int signum) {
    signal_received = signum;
}

void MattDaemon::deamonize() {

    if (auto pid = fork(); pid < 0) {
        throw std::runtime_error("First fork failed: " + std::string(strerror(errno)));
    } else if (pid > 0) {
        std::exit(EXIT_SUCCESS); 
    }

    if (setsid() < 0)
        throw std::runtime_error("setsid failed: " + std::string(strerror(errno)));

    if (auto pid = fork(); pid < 0) {
        throw std::runtime_error("Second fork failed: " + std::string(strerror(errno)));
    } else if (pid > 0) {
        std::exit(EXIT_SUCCESS); 
    }

    if (chdir("/") < 0)
        throw std::runtime_error("chdir failed: " + std::string(strerror(errno)));

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY); 
    open("/dev/null", O_WRONLY); 
    open("/dev/null", O_WRONLY);
    reporter->log("Entering Daemon mode.", Tintin_reporter::Level::INFO);
}


void MattDaemon::setup_signals() {
    
    struct sigaction sa;
    sa.sa_handler = &MattDaemon::signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    
}

void MattDaemon::run() {
    reporter->log("Creating server.", Tintin_reporter::Level::INFO);
    
    if (!server.init()) {
        reporter->log("Failed to initialize server", Tintin_reporter::Level::ERROR);
        return;
    }
    
    setup_signals();
    reporter->log("Server created.", Tintin_reporter::Level::INFO);
    
    server.run(signal_received);
    
    if (signal_received > 0) {
        reporter->log("Signal " + std::to_string(signal_received) + " received.", Tintin_reporter::Level::INFO);
    }
    
    server.shutdown();
}

int MattDaemon::lock_deamon() {

    int fd = open("/var/lock/matt_deamon.lock", O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        std::cerr << "Can't open :/var/lock/matt_daemon.lock" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        std::cerr << "Can't open :/var/lock/matt_daemon.lock" << std::endl;
        reporter->log("Error file locked.", Tintin_reporter::Level::ERROR);
        reporter->log("Quitting.", Tintin_reporter::Level::INFO);
        close(fd);
        exit(EXIT_FAILURE);
    }
    return fd;
}