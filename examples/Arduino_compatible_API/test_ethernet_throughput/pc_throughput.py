#!/usr/bin/env python3
"""Ethernet throughput, the PC side: run against a board running
test_ethernet_throughput.ino, on the same network.

    python3 pc_throughput.py <the board's address> [megabytes per TCP run, 1 by default]

Standard library only. Each TCP measurement runs 3 times and prints the
three rates and their median; the UDP ones print how many packets made it.
A measurement, with no pass or fail.
"""

import socket
import statistics
import sys
import time

CMD_PORT = 5001
SINK_PORT = 5002
SOURCE_PORT = 5003
UDP_SINK_PORT = 5004
RUNS = 3


def mbps(nbytes, seconds):
    return nbytes * 8 / seconds / 1e6 if seconds > 0 else 0.0


def command(sock, host, text, timeout=30.0):
    sock.settimeout(timeout)
    sock.sendto(text.encode(), (host, CMD_PORT))
    try:
        data, _ = sock.recvfrom(256)
        return data.decode()
    except socket.timeout:
        return None


def tcp_in(host, n):
    """PC to board: (PC-side Mbit/s, board-side Mbit/s)."""
    s = socket.create_connection((host, SINK_PORT), timeout=30)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    data = bytes(range(256)) * (n // 256) + bytes(n % 256)
    t0 = time.perf_counter()
    s.sendall(b"%d\n" % n + data)
    reply = b""
    while not reply.endswith(b"\n"):
        chunk = s.recv(64)
        if not chunk:
            break
        reply += chunk
    t1 = time.perf_counter()
    s.close()
    us = int(reply) if reply.strip().isdigit() else 0
    return mbps(n, t1 - t0), mbps(n, us / 1e6) if us else 0.0


def tcp_out(host, n, block):
    """Board to PC: Mbit/s from the request to the last byte."""
    s = socket.create_connection((host, SOURCE_PORT), timeout=30)
    t0 = time.perf_counter()
    s.sendall(b"%d %d\n" % (n, block))
    got = 0
    while got < n:
        chunk = s.recv(65536)
        if not chunk:
            break
        got += len(chunk)
    t1 = time.perf_counter()
    s.close()
    return mbps(got, t1 - t0) if got == n else 0.0


def show(label, rates):
    print("%-40s %s   median %.2f Mbit/s" % (label, "  ".join("%6.2f" % r for r in rates), statistics.median(rates)))


def main():
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        sys.exit(2)
    host = sys.argv[1]
    n = int(float(sys.argv[2]) * 1000000) if len(sys.argv) == 3 else 1000000
    print("=== Ethernet throughput, board at %s, %d bytes per TCP run ===" % (host, n))

    u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # ---- TCP ----
    pc_side, board_side = [], []
    for _ in range(RUNS):
        a, b = tcp_in(host, n)
        pc_side.append(a)
        board_side.append(b)
        time.sleep(0.3)
    show("TCP, PC to board (PC's clock)", pc_side)
    show("TCP, PC to board (board's clock)", board_side)

    for block in (1460, 512, 64):
        size = n if block >= 512 else n // 10
        rates = []
        for _ in range(RUNS):
            rates.append(tcp_out(host, size, block))
            time.sleep(0.3)
        show("TCP, board to PC, %d-byte write()s" % block, rates)

    # ---- UDP, board to PC ----
    r = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    r.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
    r.bind(("", 0))
    r.settimeout(0.5)
    count, size = 1000, 1472
    reply = command(u, host, "SEND %d %d %d" % (count, size, r.getsockname()[1]))
    got = 0
    try:
        while True:
            got += len(r.recv(2048)) > 0
    except socket.timeout:
        pass
    r.close()
    if reply:
        sent, us = (int(x) for x in reply.split())
        print("UDP, board to PC: %d of %d sent, %d received, %.2f Mbit/s sent (board's clock)"
              % (sent, count, got, mbps(sent * size, us / 1e6)))
    else:
        print("UDP, board to PC: no answer to SEND")

    # ---- UDP, PC to board ----
    for gap_ms in (0, 1, 2):
        command(u, host, "COUNT")
        payload = bytes(size)
        t0 = time.perf_counter()
        for i in range(count):
            u.sendto(payload, (host, UDP_SINK_PORT))
            if gap_ms:
                while time.perf_counter() < t0 + (i + 1) * gap_ms / 1000:
                    pass
        t1 = time.perf_counter()
        time.sleep(0.5)
        reply = command(u, host, "COUNT")
        packets = int(reply.split()[0]) if reply else -1
        print("UDP, PC to board, %s: %d of %d received, sent at %.2f Mbit/s"
              % ("one every %d ms" % gap_ms if gap_ms else "as fast as the PC sends", packets, count,
                 mbps(count * size, t1 - t0)))

    reply = command(u, host, "TICK")
    if reply == "-":
        print("longest 1 ms lwIP interrupt: -  (W5500)")
    elif reply:
        print("longest 1 ms lwIP interrupt: %s us" % reply)
    u.close()


if __name__ == "__main__":
    main()
