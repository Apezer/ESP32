"""
Send a JSON status file to the ESP32 BLE GATT status receiver.

Install dependency:
    python -m pip install bleak

Usage:
    python tools/send_status_ble.py status.json
    python tools/send_status_ble.py status.json --hold 30
    python tools/send_status_ble.py status.json --watch
    python tools/send_status_ble.py status.json --watch --keep-connected --no-ack
    python tools/send_status_ble.py status.json --address XX:XX:XX:XX:XX:XX
    python tools/send_status_ble.py status.json --no-pair
    python tools/send_status_ble.py status.json --no-ack
"""

import argparse
import asyncio
import json
from pathlib import Path
from typing import Optional

from bleak import BleakClient, BleakScanner


DEVICE_NAME = "Vico Keyboard ESP32-S3"
DEVICE_NAME_PREFIX = "Vico Keyboard"
STATUS_SERVICE_UUID = "7b6a0001-7c6e-4b3d-9f5f-7669636f0001"
STATUS_RX_UUID = "7b6a0002-7c6e-4b3d-9f5f-7669636f0002"
STATUS_TX_UUID = "7b6a0003-7c6e-4b3d-9f5f-7669636f0003"


def service_matches(advertisement_data) -> bool:
    service_uuids = [uuid.lower() for uuid in advertisement_data.service_uuids]
    return STATUS_SERVICE_UUID.lower() in service_uuids


def device_label(device, advertisement_data) -> str:
    name = device.name or advertisement_data.local_name or "<no name>"
    return f"{name} [{device.address}] RSSI={advertisement_data.rssi}"


def load_payload(path: Path) -> bytes:
    data = json.loads(path.read_text(encoding="utf-8"))
    text = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    payload = text.encode("utf-8")
    if len(payload) > 480:
        raise ValueError(f"Payload is too large for this simple test: {len(payload)} bytes")
    return payload


def file_signature(path: Path) -> Optional[tuple[int, int]]:
    try:
        stat = path.stat()
    except FileNotFoundError:
        return None
    return (stat.st_mtime_ns, stat.st_size)


async def find_device():
    print(f"Scanning for {DEVICE_NAME}...")
    found = []
    devices = await BleakScanner.discover(timeout=10.0, return_adv=True)
    for device, advertisement_data in devices.values():
        found.append((device, advertisement_data))
        names = [name for name in (device.name, advertisement_data.local_name) if name]
        if any(name == DEVICE_NAME or name.startswith(DEVICE_NAME_PREFIX) for name in names):
            return device
        if service_matches(advertisement_data):
            print("Found matching service UUID:", device_label(device, advertisement_data))
            return device

    print()
    print("Nearby BLE devices:")
    if not found:
        print("  <none>")
    for device, advertisement_data in sorted(
        found,
        key=lambda item: item[1].rssi if item[1].rssi is not None else -999,
        reverse=True,
    ):
        services = ",".join(advertisement_data.service_uuids) or "-"
        print(f"  {device_label(device, advertisement_data)} services={services}")

    return None


async def resolve_device(address: Optional[str]):
    device = address
    if device is None:
        device = await find_device()
        if device is None:
            raise RuntimeError(f"Could not find BLE device named {DEVICE_NAME}")
    return device


async def connect_client(args) -> BleakClient:
    device = await resolve_device(args.address)
    if isinstance(device, str):
        print(f"Connecting to {device}...")
    else:
        print(f"Connecting to {device.name} [{device.address}]...")

    client = BleakClient(device, pair=not args.no_pair, timeout=20.0)
    await client.connect()

    if not args.no_pair:
        try:
            await client.pair()
            print("Pair/encryption requested")
        except Exception as exc:
            print(f"Pair request skipped/failed: {exc}")

    return client


async def start_ack_notify(client: BleakClient, args) -> bool:
    if args.no_ack:
        return False

    def on_notify(_, data: bytearray):
        print("ACK:", data.decode("utf-8", errors="replace"))

    try:
        await client.start_notify(STATUS_TX_UUID, on_notify)
        return True
    except Exception as exc:
        print(f"ACK notify unavailable, continuing without ACK: {exc}")
        return False


async def write_payload(client: BleakClient, payload: bytes) -> None:
    print("Writing:", payload.decode("utf-8"))
    try:
        await client.write_gatt_char(STATUS_RX_UUID, payload, response=False)
    except Exception as exc:
        print(f"Write without response failed, retrying with response: {exc}")
        await client.write_gatt_char(STATUS_RX_UUID, payload, response=True)


async def disconnect_client(client: BleakClient, notify_started: bool) -> None:
    try:
        if notify_started and client.is_connected:
            await client.stop_notify(STATUS_TX_UUID)
    except Exception as exc:
        print(f"Stop notify skipped/failed: {exc}")
    finally:
        if client.is_connected:
            await client.disconnect()


async def send_payload(payload: bytes, args) -> None:
    client = await connect_client(args)
    notify_started = False
    try:
        notify_started = await start_ack_notify(client, args)
        await write_payload(client, payload)

        if args.hold > 0:
            print(f"Holding connection for {args.hold:.1f}s...")
            await asyncio.sleep(args.hold)
    finally:
        await disconnect_client(client, notify_started)


async def send_file_once(path: Path, args) -> None:
    payload = load_payload(path)
    await send_payload(payload, args)


async def watch_file(path: Path, args) -> int:
    print(f"Watching {path} for changes. Press Ctrl+C to stop.")
    last_signature = None

    while True:
        signature = file_signature(path)
        if signature is None:
            if last_signature is not None:
                print(f"Waiting for {path} to reappear...")
            last_signature = None
            await asyncio.sleep(args.interval)
            continue

        if signature != last_signature:
            last_signature = signature
            try:
                await asyncio.sleep(args.debounce)
                print()
                print(f"Detected change: {path}")
                await send_file_once(path, args)
                print("Send complete")
            except Exception as exc:
                print(f"Send failed: {exc}")

        await asyncio.sleep(args.interval)


async def watch_file_keep_connected(path: Path, args) -> int:
    print(f"Watching {path} with a persistent GATT connection. Press Ctrl+C to stop.")
    last_signature = None
    client = None
    notify_started = False

    while True:
        try:
            if client is None or not client.is_connected:
                if client is not None:
                    await disconnect_client(client, notify_started)
                notify_started = False
                client = await connect_client(args)
                notify_started = await start_ack_notify(client, args)
                print("Persistent GATT connection is ready")

            signature = file_signature(path)
            if signature is None:
                if last_signature is not None:
                    print(f"Waiting for {path} to reappear...")
                last_signature = None
                await asyncio.sleep(args.interval)
                continue

            if signature != last_signature:
                last_signature = signature
                await asyncio.sleep(args.debounce)
                print()
                print(f"Detected change: {path}")
                payload = load_payload(path)
                await write_payload(client, payload)
                print("Send complete")

            await asyncio.sleep(args.interval)
        except KeyboardInterrupt:
            raise
        except Exception as exc:
            print(f"Persistent connection error: {exc}")
            if client is not None:
                await disconnect_client(client, notify_started)
            client = None
            notify_started = False
            print(f"Retrying in {args.reconnect_delay:.1f}s...")
            await asyncio.sleep(args.reconnect_delay)


async def main() -> int:
    parser = argparse.ArgumentParser(description="Send JSON status to Vico BLE GATT receiver.")
    parser.add_argument("json_path", nargs="?", default="status.json", help="JSON status file")
    parser.add_argument(
        "--address",
        help="Connect directly by BLE address/Windows BLE device id instead of scanning",
    )
    parser.add_argument(
        "--no-pair",
        action="store_true",
        help="Do not request BLE pairing/encryption before using GATT",
    )
    parser.add_argument(
        "--no-ack",
        action="store_true",
        help="Do not subscribe to ACK notifications before writing",
    )
    parser.add_argument(
        "--hold",
        type=float,
        default=1.0,
        help="Seconds to keep the BLE connection open after writing",
    )
    parser.add_argument(
        "--watch",
        action="store_true",
        help="Continuously watch the JSON file and send it whenever it changes",
    )
    parser.add_argument(
        "--keep-connected",
        action="store_true",
        help="In --watch mode, keep one GATT connection open and write changes immediately",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.5,
        help="Polling interval in seconds for --watch",
    )
    parser.add_argument(
        "--debounce",
        type=float,
        default=0.2,
        help="Delay after a file change before reading JSON",
    )
    parser.add_argument(
        "--reconnect-delay",
        type=float,
        default=2.0,
        help="Seconds to wait before reconnecting after a persistent connection error",
    )
    args = parser.parse_args()

    path = Path(args.json_path)
    if args.watch:
        if args.keep_connected:
            return await watch_file_keep_connected(path, args)
        return await watch_file(path, args)

    await send_file_once(path, args)
    print("Done")
    return 0


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
