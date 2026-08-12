# Matt_daemon — Bonus Tasks

## Context

The mandatory part of the **Matt_daemon** project is already completed.

Your task is to implement **exactly these 3 bonus features**:

1. **Advanced log archival**
2. **Authentication**
3. **Useful daemon commands**

Do **not** implement the other bonus ideas.

> **Important:** Do not rewrite the mandatory implementation. Build the bonuses on top of the existing code and preserve all existing behavior.

---

# 1. First: Inspect the Existing Project

Before changing anything:

* Read the entire repository.
* Understand the existing architecture.
* Understand the existing `Tintin_reporter` class.
* Understand the socket/server implementation.
* Understand client handling.
* Understand daemonization.
* Understand signal handling.
* Understand lock-file handling.
* Understand the Makefile.
* Build the project.
* Run the existing daemon.
* Verify that the mandatory part still works.

Do not make changes until you understand how the existing implementation works.

---

# 2. Bonus #1 — Advanced Log Archival

Improve the existing logging system.

The current log file is:

```text
/var/log/matt_daemon/matt_daemon.log
```

Implement automatic log rotation.

## Expected behavior

When the log reaches a configurable maximum size, rotate it:

```text
/var/log/matt_daemon/matt_daemon.log
/var/log/matt_daemon/matt_daemon.log.1
/var/log/matt_daemon/matt_daemon.log.2
/var/log/matt_daemon/matt_daemon.log.3
...
```

Keep a configurable number of archived logs.

For example:

```text
MAX_LOG_SIZE = 1 MB
MAX_LOG_FILES = 5
```

These values should preferably be configurable.

## Requirements

* Existing logging must continue to work.
* Existing timestamp formatting must remain intact.
* Rotation must not crash the daemon.
* Old logs must be preserved.
* The number of archived logs should be limited.
* File errors must be handled gracefully.
* Do not lose log messages unnecessarily during rotation.
* Do not break `Tintin_reporter`.

## Example

If:

```text
MAX_LOG_FILES = 3
```

the directory could contain:

```text
matt_daemon.log
matt_daemon.log.1
matt_daemon.log.2
matt_daemon.log.3
```

When another rotation happens, the oldest archive is removed and the others are shifted.

---

# 3. Bonus #2 — Authentication

Add authentication for clients connecting to the daemon.

The daemon currently accepts clients through port:

```text
4242
```

Add an authentication step before allowing access to protected commands.

## Example

A client could connect:

```text
nc localhost 4242
```

and receive:

```text
Username:
```

The client enters:

```text
admin
```

Then:

```text
Password:
```

After successful authentication:

```text
Authentication successful.
Welcome to Matt_daemon.
```

For invalid credentials:

```text
Authentication failed.
```

## Requirements

* Clients must authenticate before using protected commands.
* Authentication failures must be logged.
* Never log passwords.
* Do not store plaintext passwords if avoidable.
* Store authentication configuration securely.
* Do not expose sensitive information through error messages.
* Handle disconnected clients cleanly.
* Authentication must not break the existing `quit` behavior.
* The daemon must remain stable when receiving malformed authentication input.

## Credentials

Use a simple configuration mechanism appropriate for the project.

For example:

```text
/etc/matt_daemon/users.conf
```

If credentials are stored in a file:

* Do not make the file world-readable.
* Do not commit real credentials to Git.
* Do not hard-code passwords into the source code.

A simple setup is sufficient for the bonus. Do not turn this into a full authentication framework.

---

# 4. Bonus #3 — Useful Daemon Commands

Add useful commands that authenticated clients can execute.

Implement at least:

```text
help
status
uptime
clients
logs
quit
```

## `help`

Example:

```text
Available commands:
  help
  status
  uptime
  clients
  logs
  quit
```

## `status`

Display useful daemon information.

Example:

```text
Matt_daemon status: running
PID: 1234
Port: 4242
Connected clients: 2/3
```

Do not expose unnecessary sensitive information.

## `uptime`

Display how long the daemon has been running.

Example:

```text
Daemon uptime: 01:32:45
```

## `clients`

Display the number of currently connected clients.

Example:

```text
Connected clients: 2/3
```

## `logs`

Return a limited amount of recent log information.

For example:

```text
Last 10 log entries:
...
```

Do not allow clients to request arbitrary files from the filesystem.

## `quit`

Keep the existing mandatory behavior:

```text
quit
```

must shut down the daemon correctly.

---

# 5. Command Security

Because `Matt_daemon` runs with root privileges, do not create unnecessary security vulnerabilities.

Never allow commands to:

* Execute arbitrary shell commands.
* Read arbitrary files.
* Delete arbitrary files.
* Modify arbitrary system files.
* Escape the intended command interface.

For example, do **not** implement:

```text
exec <user input>
```

and pass it directly to:

```cpp
system()
```

The command interface must use an explicit list of supported commands.

---

# 6. Logging Bonus Activity

Use the existing `Tintin_reporter` for bonus-related events.

Examples:

```text
[12/08/2026-13:30:10] [ INFO ] - Client authentication attempt.
[12/08/2026-13:30:11] [ INFO ] - Client authenticated successfully.
[12/08/2026-13:30:15] [ LOG ] - Command received: status.
[12/08/2026-13:30:20] [ WARN ] - Authentication failed.
[12/08/2026-13:30:25] [ INFO ] - Rotating log file.
```

Never log:

* Passwords
* Authentication secrets
* Private credentials

---

# 7. Preserve the Mandatory Requirements

After implementing the bonuses, verify that all mandatory functionality still works.

The following must remain functional:

* Executable named `Matt_daemon`
* Root privilege requirement
* Daemon mode
* Port `4242`
* Maximum 3 simultaneous clients
* `Tintin_reporter`
* `/var/log/matt_daemon/matt_daemon.log`
* `/var/lock/matt_daemon.lock`
* Single daemon instance
* `quit`
* Signal handling
* Proper shutdown
* Required `fork`
* Required `chdir`
* Required `flock`
* C++ implementation
* Makefile

Do not unnecessarily refactor these components.

---

# 8. Testing

Test every bonus individually.

## Log Rotation

Generate enough log data to exceed the configured limit.

Verify:

```text
matt_daemon.log
matt_daemon.log.1
matt_daemon.log.2
...
```

Verify that old files are removed once the maximum number is reached.

---

## Authentication

Test:

### Correct credentials

```text
Username: admin
Password: correct-password

Authentication successful.
```

### Incorrect password

```text
Username: admin
Password: wrong-password

Authentication failed.
```

### Unknown user

```text
Username: unknown
Password: whatever

Authentication failed.
```

### Disconnect during authentication

The daemon must handle the disconnect without crashing.

---

## Commands

After authentication, test:

```text
help
status
uptime
clients
logs
quit
```

Verify that each command produces sensible output.

Test unknown commands:

```text
foobar
```

The daemon should respond with something similar to:

```text
Unknown command. Type 'help' for available commands.
```

---

# 9. Regression Testing

After all 3 bonuses are implemented:

1. Build the project from a clean state.
2. Start `Matt_daemon`.
3. Verify port `4242`.
4. Connect with `nc`.
5. Test authentication.
6. Test every command.
7. Connect multiple clients.
8. Test the 3-client limit.
9. Test log rotation.
10. Test `quit`.
11. Test signal handling.
12. Restart the daemon.
13. Verify the lock file works correctly.
14. Check for crashes, leaks, or unexpected behavior.

---

# 10. Makefile

Keep the existing Makefile structure whenever possible.

Make sure:

```bash
make
```

still produces:

```text
Matt_daemon
```

Also ensure:

```bash
make clean
make fclean
make re
```

continue to work correctly.

---

# 11. Final Requirements

At the end, the project should contain:

```text
Matt_daemon
Makefile
assignment.md
bonus.md
```

plus the existing source files and any configuration/documentation required by the implementation.

Update `README.md` if necessary to explain:

* How to configure authentication.
* How to test authentication.
* How log rotation works.
* Which commands are available.
* How to test all three bonuses.

---

# 12. Instructions to Claude

Follow this exact workflow:

### Step 1 — Inspect

Read and understand the existing repository.

### Step 2 — Test

Build and run the existing mandatory implementation.

### Step 3 — Plan

Explain briefly how you will add the 3 bonuses without breaking the existing implementation.

### Step 4 — Implement

Implement only:

* Advanced log archival
* Authentication
* Useful daemon commands

Do not implement:

* Graphical client
* Remote shell
* Encryption

### Step 5 — Test

Test each bonus independently.

### Step 6 — Regression Test

Confirm that the mandatory requirements still work.

### Step 7 — Review

Inspect the final changes and remove unnecessary modifications.

### Step 8 — Document

Update the README with instructions for the three bonuses.

---

# Final Rule

**The mandatory part is already done.**

Do not rewrite the project.

Do not redesign working components without a reason.

Make the smallest clean changes necessary to add the three bonuses.

The final result should be stable, secure, easy to demonstrate, and easy to explain during peer evaluation.
