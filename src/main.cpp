#include "../include/Matt_daemon.hpp"

int main() {
    try{
        MattDaemon::is_root();  
        MattDaemon daemon;
        daemon.start();
        return EXIT_SUCCESS;
    } catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
    }
}