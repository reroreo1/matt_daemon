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

## Bonus features

### Authentication

Every new connection must authenticate before it can run any command.

```
$ nc localhost 4242
Username:
admin
Password:
changeme
Authentication successful.
Welcome to Matt_daemon.
```

A wrong username or password gets `Authentication failed.` and the connection is closed immediately (no retry) to limit brute-forcing.

Credentials are configured in `/etc/matt_daemon/users.conf`, one `username:hash` pair per line (`#`-comments and blank lines are ignored). Passwords are never stored in plaintext — hashes use glibc's SHA-512 `crypt()` format (`$6$...`). Set it up from the shipped template:

```bash
sudo mkdir -p /etc/matt_daemon
sudo cp config/users.conf.example /etc/matt_daemon/users.conf
sudo chmod 600 /etc/matt_daemon/users.conf
sudo chown root:root /etc/matt_daemon/users.conf
```

Generate a hash for a new user and edit the file to add/replace the line:

```bash
openssl passwd -6 -salt "$(openssl rand -hex 8)" 'your-password-here'
```

Authentication attempts and outcomes are logged via `Tintin_reporter` (`Client authentication attempt.`, `Client authenticated successfully.`, `Authentication failed.`) — passwords themselves are never logged.

**To test:** connect with `nc localhost 4242` and try the correct username/password, a wrong password, an unknown username, and disconnecting (Ctrl+C) mid-prompt — the daemon should keep running in all cases.

### Daemon commands

Once authenticated, a client can run:

```
help      List available commands
status    Show running state, PID, port, connected client count
uptime    Show how long the daemon has been running (HH:MM:SS)
clients   Show the number of currently connected clients (X/3)
logs      Show the last 10 lines of the log file
quit      Shut down the daemon (unchanged mandatory behavior)
```

Any other input gets `Unknown command. Type 'help' for available commands.` The command set is a fixed, explicit list — there is no way to run shell commands or read arbitrary files through it.

**To test:** after authenticating, run each command in turn and check the output; try an unrecognized command (e.g. `foobar`) and confirm the fallback message.

### Log rotation

`/var/log/matt_daemon/matt_daemon.log` rotates automatically once it reaches a size limit (default 1 MB), keeping a bounded number of numbered archives (default 5):

```
matt_daemon.log
matt_daemon.log.1
matt_daemon.log.2
...
```

On rotation, the oldest archive is dropped, existing archives shift up by one, and the current log becomes `matt_daemon.log.1`; a fresh `matt_daemon.log` starts with a `Rotating log file.` entry. Both limits are configurable via environment variables read at daemon startup:

```bash
MATT_DAEMON_MAX_LOG_SIZE=1048576   # bytes
MATT_DAEMON_MAX_LOG_FILES=5        # archived files to keep
sudo -E ./MattDaemon
```

**To test:** start the daemon with a small `MATT_DAEMON_MAX_LOG_SIZE` (e.g. `100`), generate a few log lines (connect/disconnect a client, run a couple of commands), and confirm `matt_daemon.log.1` appears; keep going past `MATT_DAEMON_MAX_LOG_FILES` and confirm the oldest archive gets removed.

### Testing all three bonuses without root

The daemon normally needs root because its fixed paths — `/var/log/matt_daemon`, `/etc/matt_daemon`, `/var/lock` — aren't writable by a regular user. Two of those are now overridable at startup, same pattern as the log-rotation limits above:

```bash
MATT_DAEMON_LOG_PATH=/tmp/matt_daemon_demo/matt_daemon.log
MATT_DAEMON_USERS_CONF=/tmp/matt_daemon_demo/users.conf
```

(`/var/lock` needs no override — it's a symlink to `/run/lock`, which is world-writable already.) Leave both unset and the daemon behaves exactly as before, using the real `/var`/`/etc` paths.

`scripts/demo_no_sudo.py` builds the project and exercises every bonus and every mandatory behavior end-to-end using these overrides — no `sudo` anywhere:

```bash
python3 scripts/demo_no_sudo.py
```

It prints one `[PASS]`/`[FAIL]` line per check (authentication success/failure/unknown-user/mid-auth disconnect, all six commands, the 3-client cap, `quit`, `SIGTERM`, the single-instance lock, and log rotation) and exits non-zero if anything fails.

# matt_daemon
