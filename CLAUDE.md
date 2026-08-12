# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

MattDaemon is a 42-school-style C++17 Unix daemon (the "matt_daemon" project). It daemonizes itself, prevents duplicate instances via a lock file, listens on TCP port 4242 for up to 3 simultaneous clients, and logs all activity with timestamps to a fixed log file.

## Build commands

```bash
make        # build ./MattDaemon (obj/*.o via incremental compile)
make clean  # remove obj/
make fclean # remove obj/ and the MattDaemon binary
make re     # fclean + all
make run    # build, then `sudo ./MattDaemon`
```

There is no test suite. Compilation uses `g++ -Wall -Wextra -Werror -std=c++17`, so any warning fails the build — treat warnings as errors when editing.

## Running / manual verification

The daemon requires root (it checks `getuid() != 0` and throws if not root) and true daemonization (double fork, `setsid`, closes std fds, redirects them to `/dev/null`, `chdir("/")`), so it cannot be exercised as a normal foreground process or driven interactively from a non-privileged shell.

```bash
sudo ./MattDaemon          # start (detaches into background)
nc localhost 4242          # connect as a client
```

- Any line sent by a client other than `quit` is logged as a `LOG` entry (`User input: <message>`).
- Sending `quit` closes that client's session and signals the server loop to shut down.
- The daemon also shuts down cleanly on SIGTERM, SIGINT, SIGHUP, SIGQUIT.
- Logs are appended to `/var/log/matt_daemon/matt_daemon.log` (directory auto-created if missing).
- A lock file at `/var/lock/matt_deamon.lock` (note: literal typo "deamon" in the actual path used by the code, inconsistent with the log directory's correctly-spelled "daemon" and with what the README documents) prevents a second instance from starting; it's acquired with `flock(LOCK_EX | LOCK_NB)`.

Because runtime behavior depends on root privileges, actual daemonization, and a fixed system path outside the repo, verifying a change generally means: build, run under `sudo`, connect with `nc`, and check the log file — plain unit testing isn't set up for this project.

## Architecture

Three classes, each in `include/*.hpp` + `src/*.cpp`, wired together by composition (`main.cpp` → `MattDaemon` owns a `Server`, both use the `Tintin_reporter` singleton):

- **`Tintin_reporter`** (`Tintin_reporter.hpp/.cpp`): Meyer's-singleton logger (`Tintin_reporter::instance()`), not constructible directly. Opens the log file once in its constructor and keeps it open for `<<` writes, but each `log()` call additionally opens the same path a second time via a raw fd purely to take an `flock` around the write — a cross-process write lock layered on top of the long-lived `ofstream`. Levels are `INFO` / `ERROR` / `LOG`.

- **`Server`** (`Server.hpp/.cpp`): a single-threaded `poll()`-based TCP server. `poll_fds[0]` is always the listening socket; indices `1..N` are connected clients (max 3, enforced in `accept_new_connection`). `run()` loops on `poll()` with a 1s timeout, checking both its own `should_quit` flag and the caller-owned `signal_received` (passed by reference) each iteration so a signal can interrupt the loop. Client disconnects and reads are both handled in `handle_client_message` by checking `bytes_read <= 0`.

- **`MattDaemon`** (`Matt_daemon.hpp/.cpp`): orchestrates startup order — `is_root()` (static, called before construction in `main.cpp`) → acquire lock file → `deamonize()` (double fork + `setsid` + fd redirection to `/dev/null`) → `setup_signals()` (installs a `sigaction` handler that just records the signum into the static `volatile sig_atomic_t signal_received`) → `server.init()` → `server.run(signal_received)` → `server.shutdown()`. The destructor logs "Quitting.", closes the lock fd, and unlinks the lock file.

All three classes declare copy constructor/assignment but implement them as no-ops (`(void)other`) — this is boilerplate typically required by the 42 coding norm, not real copy semantics; don't rely on copies actually copying state.

## Known inconsistencies to be aware of

- Lock file path has two different spellings in play: the code uses `/var/lock/matt_deamon.lock` (missing an "n") while `README.md` documents `/var/lock/matt_daemon.lock`. If asked to "fix the lock path," clarify which spelling is intended before changing it, since the misspelling is used consistently by both the acquire and cleanup code paths.
