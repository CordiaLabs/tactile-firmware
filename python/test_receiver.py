#!/usr/bin/env python3
"""
Cordia Labs — 200 Hz Tactile Telemetry UDP Receiver & Slip Monitor.

Binds to UDP port 5006, decodes 24-byte binary TactilePackets, and prints
real-time contact forces, loop frequency, and slip differentials.
"""

import socket
import struct
import time
import argparse

PACKET_FORMAT = "<II4f"  # uint32 seq, uint32 t_us, float32[4] (left_fixed, left_moving, right_fixed, right_moving)
PACKET_SIZE = struct.calcsize(PACKET_FORMAT)

def run(port: int = 5006):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", port))
    print(f"Listening for 200 Hz tactile packets on 0.0.0.0:{port} ...")

    last_report = time.time()
    packet_count = 0
    last_seq = None

    while True:
        data, addr = sock.recvfrom(1024)
        if len(data) != PACKET_SIZE:
            continue

        seq, t_us, lf, lm, rf, rm = struct.unpack(PACKET_FORMAT, data)
        packet_count += 1

        now = time.time()
        if now - last_report >= 0.5:
            hz = packet_count / (now - last_report)
            drops = (seq - last_seq - packet_count) if last_seq is not None else 0
            print(f"[{hz:5.1f} Hz] Seq: {seq:<7d} | L-Jaw: {lf:4.0f}/{lm:4.0f} | R-Jaw: {rf:4.0f}/{rm:4.0f} | Drops: {drops}")
            packet_count = 0
            last_report = now
            last_seq = seq

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cordia Labs Tactile UDP Receiver")
    parser.add_argument("--port", type=int, default=5006, help="UDP port (default: 5006)")
    args = parser.parse_args()
    try:
        run(args.port)
    except KeyboardInterrupt:
        print("\nReceiver stopped.")
