#!/usr/bin/env python3
"""Bench tool for the LUCAS FTH-XL heater: talks BLE directly to the firmware.

Speaks the same protobuf protocol as the app. One central at a time — close the
app's browser tab before using this.

Usage:
  bench.py monitor                     # connect and stream reports until Ctrl-C
  bench.py cold [--duration 15]        # pour without heating, stream reports
  bench.py heat --target 60 --duration 10 \
           [--preheat-dur-mult 30] [--preheat-pow-mult 1.0] \
           [--p 2.5] [--i 0.3] [--d 0.05]
  bench.py stop                        # send a stop command and exit

Every session appends CSV rows to bench/logs/<timestamp>-<mode>.csv.
"""

import argparse
import asyncio
import csv
import os
import sys
import time
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from bleak import BleakClient, BleakScanner

from proto.app.AppCommand_pb2 import AppCommand
from proto.firmware.FirmwareEvent_pb2 import FirmwareEvent
from proto.firmware.Alarm_pb2 import AlarmCode
from proto.firmware.heater.HeatingReport_pb2 import HeatingStage

# From firmware/main/maestro/Bridge.cpp (BLE_UUID128_INIT is little-endian).
CHARACTERISTIC_UUID = "5019aa43-e7f5-95be-9a44-8dd43473c349"
DEVICE_NAME_PREFIX = "PROTO-"

STAGE_NAMES = {
    HeatingStage.PreHeating: "preheat",
    HeatingStage.Heating: "heating",
    HeatingStage.Finished: "finished",
}

ALARM_NAMES = {
    AlarmCode.SENSOR_FAILURE: "SENSOR_FAILURE",
    AlarmCode.OVER_TEMPERATURE: "OVER_TEMPERATURE",
}


def build_start_command(args) -> bytes:
    command = AppCommand()
    start = command.heater_control.start
    start.target_temperature = args.target
    start.duration_ms = round(args.duration * 1000)
    start.preheat_duration_multiplier = args.preheat_dur_mult
    start.preheat_power_multiplier = args.preheat_pow_mult
    start.p = args.p
    start.i = args.i
    start.d = args.d
    return command.SerializeToString()


def build_stop_command() -> bytes:
    command = AppCommand()
    command.heater_control.stop.SetInParent()
    return command.SerializeToString()


async def find_device(timeout: float = 15.0):
    print(f"Procurando {DEVICE_NAME_PREFIX}* ...", flush=True)
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or "").startswith(DEVICE_NAME_PREFIX),
        timeout=timeout,
    )
    if device is None:
        print("Nenhum dispositivo PROTO-* encontrado. A placa está ligada? O app ainda está conectado?", flush=True)
        sys.exit(1)
    print(f"Encontrado: {device.name} ({device.address})", flush=True)
    return device


async def run(args):
    device = await find_device()

    finished = asyncio.Event()
    log_path = None
    writer = None
    log_file = None

    if args.mode != "stop":
        os.makedirs(os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs"), exist_ok=True)
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        log_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs", f"{stamp}-{args.mode}.csv")
        log_file = open(log_path, "w", newline="")
        writer = csv.writer(log_file)
        writer.writerow(["wall_time", "seconds_elapsed", "stage", "temperature", "pid", "power_pct"])

    def on_notify(_, data: bytearray):
        event = FirmwareEvent()
        try:
            event.ParseFromString(bytes(data))
        except Exception as error:
            print(f"[decode error] {error} ({bytes(data).hex()})", flush=True)
            return

        kind = event.WhichOneof("tag")
        if kind == "heating_report":
            r = event.heating_report
            stage = STAGE_NAMES.get(r.stage, str(r.stage))
            pid = "" if r.pid == -1 else f"{r.pid:6.1f}"
            print(f"t={r.seconds_elapsed:6.2f}s  [{stage:8s}]  temp={r.temperature:6.2f}C  pid={pid or '  --  '}  power={r.power:5.1f}%", flush=True)
            if writer:
                writer.writerow([f"{time.time():.3f}", f"{r.seconds_elapsed:.3f}", stage, f"{r.temperature:.3f}", f"{r.pid:.3f}", f"{r.power:.2f}"])
                log_file.flush()
            if r.stage == HeatingStage.Finished:
                finished.set()
        elif kind == "alarm":
            name = ALARM_NAMES.get(event.alarm.code, str(event.alarm.code))
            print(f"!!! ALARME: {name} — firmware cortou o aquecimento", flush=True)
            if writer:
                writer.writerow([f"{time.time():.3f}", "", f"ALARM:{name}", "", "", ""])
                log_file.flush()
            finished.set()

    async with BleakClient(device) as client:
        print("Conectado.", flush=True)
        await client.start_notify(CHARACTERISTIC_UUID, on_notify)

        if args.mode == "stop":
            await client.write_gatt_char(CHARACTERISTIC_UUID, build_stop_command(), response=True)
            print("Stop enviado.", flush=True)
            return

        if args.mode in ("heat", "cold"):
            payload = build_start_command(args)
            await client.write_gatt_char(CHARACTERISTIC_UUID, payload, response=True)
            print(f"Start enviado (target={args.target}C, duration={args.duration}s, "
                  f"preheat mult dur/pow={args.preheat_dur_mult}/{args.preheat_pow_mult}, "
                  f"PID={args.p}/{args.i}/{args.d})", flush=True)

        try:
            if args.mode == "monitor":
                while True:
                    await asyncio.sleep(1)
            else:
                # duration + preheat margin + grace
                await asyncio.wait_for(finished.wait(), timeout=args.duration + 30)
                await asyncio.sleep(0.5)
        except (asyncio.TimeoutError, asyncio.CancelledError, KeyboardInterrupt):
            pass
        finally:
            try:
                await client.write_gatt_char(CHARACTERISTIC_UUID, build_stop_command(), response=True)
                print("Stop enviado (garantia).", flush=True)
            except Exception:
                pass
            if log_file:
                log_file.close()
                print(f"Log salvo em {log_path}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="mode", required=True)

    sub.add_parser("monitor")
    sub.add_parser("stop")

    cold = sub.add_parser("cold")
    cold.add_argument("--duration", type=float, default=15.0)

    heat = sub.add_parser("heat")
    heat.add_argument("--target", type=int, required=True)
    heat.add_argument("--duration", type=float, required=True)
    heat.add_argument("--preheat-dur-mult", type=float, default=30.0)
    heat.add_argument("--preheat-pow-mult", type=float, default=1.0)
    heat.add_argument("--p", type=float, default=2.5)
    heat.add_argument("--i", type=float, default=0.3)
    heat.add_argument("--d", type=float, default=0.05)

    args = parser.parse_args()

    if args.mode == "cold":
        args.target = 0
        args.preheat_dur_mult = 0.0
        args.preheat_pow_mult = 0.0
        args.p = args.i = args.d = 0.0

    asyncio.run(run(args))


if __name__ == "__main__":
    main()
