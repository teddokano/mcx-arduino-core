#!/usr/bin/env python3
"""Release check 25, the PC side: run against FRDM-MCXN947 running
25_ethernet_pc_N947.ino, on the same network.

    python3 pc_check.py <the board's address>

Standard library only. Prints each check and "ALL OK" or "N FAILED"; the
exit status is the number of failures.
"""

import os
import platform
import socket
import subprocess
import sys
import threading
import time

TCP_PORT = 5000
UDP_PORT = 5001

fail_count = 0


def check(label, ok, detail=""):
    global fail_count
    print("%s: %s%s" % (label, "OK" if ok else "FAIL", ("  (" + detail + ")") if detail else ""))
    if not ok:
        fail_count += 1


def ping(host, count):
    """Return (all answered, the text ping printed)."""
    flag = "-n" if platform.system() == "Windows" else "-c"
    try:
        r = subprocess.run(["ping", flag, str(count), host], capture_output=True, text=True, timeout=count * 2 + 10)
    except (OSError, subprocess.TimeoutExpired) as e:
        return False, str(e)
    return r.returncode == 0, r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr.strip()


def udp_request(sock, host, payload, timeout=1.0):
    sock.settimeout(timeout)
    sock.sendto(payload, (host, UDP_PORT))
    try:
        data, _ = sock.recvfrom(2048)
        return data
    except socket.timeout:
        return None


def tcp_echo(host, payload, timeout=10.0):
    """Send payload while a thread reads the echo back; return what came back."""
    s = socket.create_connection((host, TCP_PORT), timeout=timeout)
    got = bytearray()

    def reader():
        try:
            while len(got) < len(payload):
                chunk = s.recv(65536)
                if not chunk:
                    return
                got.extend(chunk)
        except OSError:
            return

    t = threading.Thread(target=reader)
    t.start()
    s.sendall(payload)
    t.join(timeout)
    s.close()
    return bytes(got)


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    host = sys.argv[1]
    print("=== release_check 25: Ethernet with a PC, board at %s ===" % host)

    # ---- ICMP ----
    ok, text = ping(host, 5)
    check("ping, 5 of 5 answered", ok, text)

    u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # ---- UDP echo ----
    for size in (1, 100, 1000, 1472):
        payload = os.urandom(size).replace(b"C", b"c").replace(b"D", b"d").replace(b"S", b"s")
        back = udp_request(u, host, payload)
        check("UDP echo, %d bytes" % size, back == payload,
              "no answer" if back is None else ("%d bytes back" % len(back) if back != payload else ""))

    lost = 0
    for i in range(50):
        payload = b"seq %d" % i
        if udp_request(u, host, payload, 0.5) != payload:
            lost += 1
    check("UDP echo, 50 in a row", lost == 0, "%d lost" % lost if lost else "")

    # ---- TCP echo ----
    for size in (10, 1460, 65536):
        payload = os.urandom(size)
        try:
            back = tcp_echo(host, payload)
            check("TCP echo, %d bytes" % size, back == payload,
                  "" if back == payload else "%d of %d bytes back intact" % (len(back), size))
        except OSError as e:
            check("TCP echo, %d bytes" % size, False, str(e))

    # several connections at once, written in turn
    try:
        conns = [socket.create_connection((host, TCP_PORT), timeout=5) for _ in range(4)]
        ok = True
        for round_no in range(5):
            for i, c in enumerate(conns):
                c.sendall(b"conn %d round %d|" % (i, round_no))
        for i, c in enumerate(conns):
            want = b"".join(b"conn %d round %d|" % (i, r) for r in range(5))
            got = b""
            c.settimeout(3)
            while len(got) < len(want):
                chunk = c.recv(4096)
                if not chunk:
                    break
                got += chunk
            ok = ok and got == want
            c.close()
        check("TCP echo, 4 connections at once", ok)
    except OSError as e:
        check("TCP echo, 4 connections at once", False, str(e))

    # the board frees a connection once the PC closes it: more than MAX_SOCK_NUM (8) in turn
    good = 0
    for i in range(20):
        try:
            if tcp_echo(host, b"again %d" % i, 3) == b"again %d" % i:
                good += 1
        except OSError:
            pass
        time.sleep(0.05)
    check("TCP echo, 20 connections one after another", good == 20, "%d of 20" % good)

    # ---- the board connects to the PC ----
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("", 0))
    listener.listen(1)
    listener.settimeout(5)
    port = listener.getsockname()[1]
    ack = udp_request(u, host, b"CONNECT %d" % port)
    received = bytearray()
    try:
        conn, _ = listener.accept()
        conn.settimeout(5)
        while True:
            chunk = conn.recv(65536)
            if not chunk:
                break
            received.extend(chunk)
        conn.close()
    except OSError:
        pass
    listener.close()
    want = bytes(((i * 7) & 0xFF) for i in range(8192))
    check("board connects to the PC and sends 8192 bytes", ack == b"OK" and bytes(received) == want,
          "" if bytes(received) == want else "%d bytes" % len(received)
          + (", if nothing came, check the PC's firewall for incoming connections" if not received else ""))

    # ---- lwIP keeps answering while the sketch sits in delay() ----
    ack = udp_request(u, host, b"DELAY 4000")
    t0 = time.time()
    ok, text = ping(host, 2)
    connected = False
    try:
        s = socket.create_connection((host, TCP_PORT), timeout=2)
        connected = True
        s.close()
    except OSError:
        pass
    during = time.time() - t0 < 3.5
    check("ping while the sketch is in delay()", ack == b"OK" and ok and during, text)
    check("TCP connection accepted while the sketch is in delay()", ack == b"OK" and connected and during)
    time.sleep(max(0, 4.5 - (time.time() - t0)))

    # ---- the 1 ms interrupt ----
    stats = udp_request(u, host, b"STATS")
    try:
        tick = int(stats)
        check("longest 1 ms lwIP interrupt under 1000 us", tick < 1000, "%d us" % tick)
    except (TypeError, ValueError):
        check("longest 1 ms lwIP interrupt under 1000 us", False, "no answer to STATS")
    u.close()

    print("ALL OK" if fail_count == 0 else "%d FAILED" % fail_count)
    sys.exit(fail_count)


if __name__ == "__main__":
    main()
