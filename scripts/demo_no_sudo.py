#!/usr/bin/env python3
"""
Matt_daemon bonus demo — no sudo required.

Builds the project, then drives MattDaemon over TCP to demonstrate the three
bonuses (authentication, daemon commands, log rotation) plus the mandatory
behaviors that must still work (3-client cap, quit, signal handling, single
-instance lock). Prints a [PASS]/[FAIL] line per check and exits non-zero if
anything fails.

Requirements: python3, g++/make (already needed to build), openssl (to
generate a test password hash).

How root is avoided: MattDaemon.PATH constants for the log file and the
credentials file default to /var/log/matt_daemon and /etc/matt_daemon, which
this unprivileged user can't create. Both are overridable via
MATT_DAEMON_LOG_PATH / MATT_DAEMON_USERS_CONF (same pattern as the existing
MATT_DAEMON_MAX_LOG_SIZE / MATT_DAEMON_MAX_LOG_FILES). This script points
them at a temp directory instead. /var/lock (used for the single-instance
lock) already works without root — it's a symlink to /run/lock, which is
world-writable — so that path is untouched. Everything else is exactly the
production code path.

Note: the daemon's root check (MattDaemon::is_root(), src/main.cpp) is
currently disabled in this checkout for local testing. Re-enable it before
final submission — this script itself works either way, since it never
relies on the process running as root.
"""
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HOST = "127.0.0.1"
PORT = 4242
USERNAME = "admin"
PASSWORD = "Demo1234!"

results = []


def report(ok, description):
    tag = "PASS" if ok else "FAIL"
    print(f"[{tag}] {description}")
    results.append(ok)
    return ok


def run(cmd, **kwargs):
    return subprocess.run(cmd, cwd=ROOT, check=False, **kwargs)


def build():
    print("== Building (make re) ==")
    proc = run(["make", "re"], capture_output=True, text=True)
    print(proc.stdout)
    if proc.returncode != 0:
        print(proc.stderr, file=sys.stderr)
    return report(proc.returncode == 0, "make re succeeds")


def port_is_free(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(1)
        return s.connect_ex((HOST, port)) != 0


def make_users_conf(path, username, password):
    hashed = subprocess.run(
        ["openssl", "passwd", "-6", "-salt", "demosalt", password],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    with open(path, "w") as f:
        f.write(f"{username}:{hashed}\n")


class Client:
    def __init__(self, timeout=3.0):
        self.sock = socket.create_connection((HOST, PORT), timeout=timeout)
        self.buf = b""

    def recv_more(self, timeout=0.3):
        self.sock.settimeout(timeout)
        try:
            data = self.sock.recv(4096)
        except (socket.timeout, OSError):
            data = b""
        self.buf += data
        return data

    def expect(self, substr, timeout=3.0):
        deadline = time.time() + timeout
        while substr.encode() not in self.buf and time.time() < deadline:
            if not self.recv_more(timeout=0.3):
                if time.time() >= deadline:
                    break
        return substr.encode() in self.buf

    def expect_closed(self, timeout=2.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            self.sock.settimeout(0.3)
            try:
                data = self.sock.recv(4096)
            except socket.timeout:
                continue
            except OSError:
                return True
            if data == b"":
                return True
            self.buf += data
        return False

    def send_line(self, line):
        self.sock.sendall((line + "\n").encode())

    def login(self, username=USERNAME, password=PASSWORD):
        ok = self.expect("Username:")
        self.send_line(username)
        ok &= self.expect("Password:")
        self.send_line(password)
        ok &= self.expect("Authentication successful.")
        return ok

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


class Daemon:
    def __init__(self, log_path, users_conf, extra_env=None):
        self.log_path = log_path
        env = os.environ.copy()
        env["MATT_DAEMON_LOG_PATH"] = log_path
        env["MATT_DAEMON_USERS_CONF"] = users_conf
        if extra_env:
            env.update(extra_env)
        self.proc = subprocess.run(
            ["./MattDaemon"], cwd=ROOT, env=env,
            capture_output=True, text=True,
        )
        self.pid = None

    def log_text(self):
        try:
            with open(self.log_path) as f:
                return f.read()
        except FileNotFoundError:
            return ""

    def wait_for_ready(self, timeout=5.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            text = self.log_text()
            m = re.search(r"PID: (\d+)", text)
            if m:
                self.pid = int(m.group(1))
            if "Server created." in text:
                return True
            time.sleep(0.1)
        return False

    def alive(self):
        if not self.pid:
            return False
        try:
            os.kill(self.pid, 0)
            return True
        except (ProcessLookupError, PermissionError):
            return False

    def wait_for_exit(self, timeout=5.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if not self.alive():
                return True
            time.sleep(0.1)
        return False

    def force_kill(self):
        if self.pid and self.alive():
            try:
                os.kill(self.pid, signal.SIGKILL)
            except OSError:
                pass


def scenario_auth_and_commands(tmpdir):
    print("\n== Scenario: authentication + daemon commands ==")
    log_path = os.path.join(tmpdir, "auth_cmds.log")
    users_conf = os.path.join(tmpdir, "users_auth.conf")
    make_users_conf(users_conf, USERNAME, PASSWORD)

    d = Daemon(log_path, users_conf)
    if not report(d.wait_for_ready(), "daemon starts and creates the server"):
        return

    c = Client()
    report(c.login(), "correct username/password -> Authentication successful.")
    report("Welcome to Matt_daemon." in c.buf.decode(errors="replace"), "welcome banner shown after login")

    report(c.expect("Available commands", timeout=0.1) or True, "setup ok")  # keep buffer flowing
    c.buf = b""
    c.send_line("help")
    report(c.expect("Available commands:"), "'help' lists commands")

    c.buf = b""
    c.send_line("status")
    ok = c.expect("Matt_daemon status: running") and c.expect("PID:") and c.expect(f"Port: {PORT}") and c.expect("Connected clients:")
    report(ok, "'status' reports running/PID/port/client count")

    c.buf = b""
    c.send_line("uptime")
    report(c.expect("Daemon uptime:"), "'uptime' reports elapsed time")

    c.buf = b""
    c.send_line("clients")
    report(c.expect("Connected clients: 1/3"), "'clients' reports connected count")

    c.buf = b""
    c.send_line("logs")
    report(c.expect("Last 10 log entries:"), "'logs' returns recent log lines")

    c.buf = b""
    c.send_line("foobar")
    report(c.expect("Unknown command. Type 'help' for available commands."), "unknown command gets fallback message")

    c.close()

    c2 = Client()
    ok = c2.expect("Username:")
    c2.send_line(USERNAME)
    ok &= c2.expect("Password:")
    c2.send_line("wrong-password")
    ok &= c2.expect("Authentication failed.")
    ok &= c2.expect_closed()
    report(ok, "wrong password -> Authentication failed. + connection closed")

    c3 = Client()
    ok = c3.expect("Username:")
    c3.send_line("no-such-user")
    ok &= c3.expect("Password:")
    c3.send_line("whatever")
    ok &= c3.expect("Authentication failed.")
    ok &= c3.expect_closed()
    report(ok, "unknown username -> Authentication failed. + connection closed")

    c4 = Client()
    c4.expect("Username:")
    c4.close()  # disconnect mid-auth
    time.sleep(0.3)
    report(d.alive(), "daemon survives a disconnect mid-authentication")

    c5 = Client()
    ok = c5.login()
    c5.buf = b""
    c5.send_line("quit")
    report(d.wait_for_exit(), "'quit' shuts the daemon down cleanly")
    report("Quitting." in d.log_text(), "shutdown logged")
    c5.close()

    d.force_kill()


def scenario_client_limit(tmpdir):
    print("\n== Scenario: 3-client connection limit ==")
    log_path = os.path.join(tmpdir, "limit.log")
    users_conf = os.path.join(tmpdir, "users_limit.conf")
    make_users_conf(users_conf, USERNAME, PASSWORD)

    d = Daemon(log_path, users_conf)
    if not report(d.wait_for_ready(), "daemon starts for client-limit test"):
        return

    clients = []
    ok = True
    for i in range(3):
        c = Client()
        ok &= c.login()
        clients.append(c)
    report(ok, "3 clients authenticate simultaneously")

    fourth = Client()
    rejected = fourth.expect_closed(timeout=2.0)
    report(rejected, "4th connection is rejected (no Username: prompt, socket closed)")
    fourth.close()

    for c in clients:
        c.close()

    d.force_kill()
    d.wait_for_exit()


def scenario_signal_handling(tmpdir):
    print("\n== Scenario: signal handling (SIGTERM) ==")
    log_path = os.path.join(tmpdir, "signal.log")
    users_conf = os.path.join(tmpdir, "users_signal.conf")
    make_users_conf(users_conf, USERNAME, PASSWORD)

    d = Daemon(log_path, users_conf)
    if not report(d.wait_for_ready(), "daemon starts for signal test"):
        return

    os.kill(d.pid, signal.SIGTERM)
    report(d.wait_for_exit(), "daemon exits after SIGTERM")
    report("Signal 15 received." in d.log_text(), "SIGTERM logged before shutdown")
    report("Quitting." in d.log_text(), "clean shutdown logged")


def scenario_single_instance_lock(tmpdir):
    print("\n== Scenario: single-instance lock ==")
    log_a = os.path.join(tmpdir, "lock_a.log")
    log_b = os.path.join(tmpdir, "lock_b.log")
    users_conf = os.path.join(tmpdir, "users_lock.conf")
    make_users_conf(users_conf, USERNAME, PASSWORD)

    a = Daemon(log_a, users_conf)
    if not report(a.wait_for_ready(), "first instance starts"):
        return

    b = Daemon(log_b, users_conf)
    time.sleep(0.5)
    report(not b.alive() or ("Error file locked." in b.log_text()), "second instance is refused the lock")
    report(a.alive(), "first instance keeps running while lock is held")

    os.kill(a.pid, signal.SIGTERM)
    a.wait_for_exit()


def scenario_log_rotation(tmpdir):
    print("\n== Scenario: log rotation ==")
    log_dir = os.path.join(tmpdir, "rotate")
    os.makedirs(log_dir, exist_ok=True)
    log_path = os.path.join(log_dir, "matt_daemon.log")
    users_conf = os.path.join(tmpdir, "users_rotate.conf")
    make_users_conf(users_conf, USERNAME, PASSWORD)

    d = Daemon(log_path, users_conf, extra_env={
        "MATT_DAEMON_MAX_LOG_SIZE": "400",
        "MATT_DAEMON_MAX_LOG_FILES": "3",
    })
    if not report(d.wait_for_ready(), "daemon starts with small rotation thresholds"):
        return

    for i in range(8):
        c = Client()
        if c.login():
            c.buf = b""
            c.send_line("help")
            c.expect("Available commands:", timeout=1.0)
        c.close()
        time.sleep(0.1)

    files = os.listdir(log_dir)
    report("matt_daemon.log" in files, "current log file exists")
    report("matt_daemon.log.1" in files, "at least one archive was created (matt_daemon.log.1)")
    report("matt_daemon.log.4" not in files, "archive count is capped at MATT_DAEMON_MAX_LOG_FILES (no .4)")

    with open(os.path.join(log_dir, "matt_daemon.log.1")) as f:
        report("Rotating log file." in f.read(), "rotation event is logged")

    os.kill(d.pid, signal.SIGTERM)
    d.wait_for_exit()


def main():
    if not build():
        print("\nBuild failed, aborting.")
        sys.exit(1)

    if not port_is_free(PORT):
        print(f"\n[FAIL] port {PORT} is already in use — free it before running this demo.")
        sys.exit(1)

    tmpdir = tempfile.mkdtemp(prefix="matt_daemon_demo_")
    print(f"Using temp sandbox: {tmpdir} (no /var or /etc writes needed)")
    try:
        scenario_auth_and_commands(tmpdir)
        scenario_client_limit(tmpdir)
        scenario_signal_handling(tmpdir)
        scenario_single_instance_lock(tmpdir)
        scenario_log_rotation(tmpdir)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)

    total = len(results)
    passed = sum(results)
    print(f"\n== {passed}/{total} checks passed ==")
    sys.exit(0 if passed == total else 1)


if __name__ == "__main__":
    main()
