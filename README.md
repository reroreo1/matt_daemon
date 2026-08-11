# MattDaemon

A robust daemon service that runs in the background and listens for client connections on port 4242.

## Features

- Runs as a system daemon in the background
- Logs all activities to `/var/log/matt_daemon/matt_daemon.log`
- Listens on port 4242 for client connections
- Supports up to 3 simultaneous client connections
- Handles signals gracefully (SIGTERM, SIGINT, SIGHUP, SIGQUIT)
- Prevents multiple instances with lock file mechanism
- Requires root privileges for security

## Requirements

- Linux system
- Root privileges
- C++ compiler with C++17 support

## Installation

1. Clone the repository
2. Compile the project:
```bash
make
```
3. Run as root:
```bash
sudo ./MattDaemon
```

## Usage

### Starting the daemon

The daemon must be run with root privileges:

```bash
sudo ./MattDaemon
```

### Connecting to the daemon

You can connect to the daemon using a tool like netcat:

```bash
nc localhost 4242
```

### Sending commands

- Send the text "quit" to close the connection
- Any other text will be logged in the daemon's log file

### Log file

All daemon activities are logged to:
```
/var/log/matt_daemon/matt_daemon.log
```

### Daemon lock file

When running, the daemon creates a lock file at:
```
/var/lock/matt_daemon.lock
```

This prevents multiple instances from running simultaneously.

## How it works

The daemon follows these steps:
1. Checks for root privileges
2. Creates a lock file to prevent multiple instances
3. Daemonizes itself (forks and runs in the background)
4. Sets up signal handlers
5. Creates a server that listens on port 4242
6. Accepts up to 3 client connections
7. Processes client messages
8. Logs all activities with timestamps
9. Cleans up resources when terminated

## Components

- **MattDaemon**: Main daemon class that handles daemonization and setup
- **Tintin_reporter**: Logging class for all daemon activities
- **Server**: Handles socket connections and client communications

## Signal Handling

The daemon properly handles the following signals:
- SIGTERM
- SIGINT
- SIGHUP
- SIGQUIT

When any of these signals are received, the daemon logs the event and shuts down gracefully.

# matt_daemon
