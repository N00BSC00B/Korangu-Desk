import socket
import time

from .config import BEACON_MESSAGE, BEACON_MESSAGE_LEGACY, DISCOVERY_MESSAGE, DISCOVERY_PORT, SERVER_PORT
from .telemetry import TELEMETRY


def get_local_ip() -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("10.255.255.255", 1))
        return sock.getsockname()[0]
    except Exception:
        return "127.0.0.1"
    finally:
        sock.close()


def broadcast_presence() -> None:
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    udp_socket.settimeout(0.5)

    try:
        udp_socket.bind(("", DISCOVERY_PORT))
    except Exception as ex:
        print(f"UDP discovery bind warning on port {DISCOVERY_PORT}: {ex}")

    local_ip = get_local_ip()
    subnet_broadcast = ".".join(local_ip.split(".")[:-1]) + ".255"
    print(f"Broadcasting Korangu Beacon to: {subnet_broadcast} (ws port {SERVER_PORT})")

    last_beacon_at = 0.0

    while True:
        now = time.time()
        if now - last_beacon_at >= 3.0:
            try:
                udp_socket.sendto(BEACON_MESSAGE, (subnet_broadcast, DISCOVERY_PORT))
                udp_socket.sendto(BEACON_MESSAGE, ("<broadcast>", DISCOVERY_PORT))
                udp_socket.sendto(BEACON_MESSAGE_LEGACY, (subnet_broadcast, DISCOVERY_PORT))
                udp_socket.sendto(BEACON_MESSAGE_LEGACY, ("<broadcast>", DISCOVERY_PORT))
                TELEMETRY.inc("udp_beacon_broadcast_cycles")
            except Exception:
                pass
            last_beacon_at = now

        try:
            packet, remote = udp_socket.recvfrom(255)
            if packet.strip() == DISCOVERY_MESSAGE:
                TELEMETRY.inc("udp_discovery_probe_received")
                udp_socket.sendto(BEACON_MESSAGE, (remote[0], DISCOVERY_PORT))
                udp_socket.sendto(BEACON_MESSAGE_LEGACY, (remote[0], DISCOVERY_PORT))
                TELEMETRY.inc("udp_discovery_probe_replied")
        except socket.timeout:
            pass
        except Exception:
            pass
