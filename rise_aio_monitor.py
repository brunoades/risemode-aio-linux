#!/usr/bin/env python3
"""Feed CPU temperature to the Rise Mode display through the kernel module."""

import argparse
import logging
import time
from pathlib import Path

HID_ROOT = Path("/sys/bus/hid/devices")
HWMON_ROOT = Path("/sys/class/hwmon")


def find_display() -> Path:
    for device in sorted(HID_ROOT.glob("0003:AA88:8666.*")):
        output = device / "display_temp"
        if output.exists():
            return output
    raise FileNotFoundError("módulo hid-risemode-aio não ligado ao dispositivo")


def find_sensor() -> Path:
    preferences = (("k10temp", "temp1_input"), ("coretemp", "temp1_input"))
    sensors = sorted(HWMON_ROOT.glob("hwmon*"))
    for name, filename in preferences:
        for sensor in sensors:
            try:
                if (sensor / "name").read_text().strip() == name:
                    path = sensor / filename
                    if path.exists():
                        return path
            except OSError:
                continue
    raise FileNotFoundError("sensor de temperatura de CPU não encontrado")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sensor", type=Path, help="caminho de temp*_input")
    parser.add_argument("--interval", type=float, default=1.0, help="segundos entre atualizações")
    parser.add_argument("--once", action="store_true", help="enviar uma medição e sair")
    args = parser.parse_args()
    if args.interval <= 0:
        parser.error("--interval deve ser positivo")

    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    sensor = args.sensor or find_sensor()
    logging.info("sensor: %s", sensor)
    while True:
        try:
            display = find_display()
            temp_mc = int(sensor.read_text().strip())
            temp_c = max(0, min(99, (temp_mc + 500) // 1000))
            display.write_text(f"{temp_c}\n")
            if args.once:
                logging.info("enviados %s °C para %s", temp_c, display)
                return
        except (OSError, ValueError) as exc:
            if args.once:
                raise SystemExit(str(exc)) from exc
            logging.warning("falha na atualização: %s", exc)
        time.sleep(args.interval)


if __name__ == "__main__":
    main()
