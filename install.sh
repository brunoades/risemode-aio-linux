#!/usr/bin/env bash
set -euo pipefail

if [[ $(id -u) -ne 0 ]]; then
	echo "Execute este script como root (por exemplo, pkexec bash install.sh)." >&2
	exit 1
fi

PATH=/usr/sbin:/usr/bin:/sbin:/bin
DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
KVER=$(uname -r)
BUILT_FOR=$(modinfo -F vermagic "$DIR/hid-risemode-aio.ko" | cut -d' ' -f1)
if [[ $BUILT_FOR != "$KVER" ]]; then
	echo "Módulo compilado para $BUILT_FOR, mas o kernel atual é $KVER. Execute make novamente." >&2
	exit 1
fi

install -Dm0644 "$DIR/hid-risemode-aio.ko" "/lib/modules/$KVER/extra/hid-risemode-aio.ko"
depmod -a "$KVER"
install -Dm0755 "$DIR/rise_aio_monitor.py" /usr/local/libexec/rise-aio-monitor.py
install -Dm0644 "$DIR/rise-aio-monitor.service" /etc/systemd/system/rise-aio-monitor.service
printf 'hid-risemode-aio\n' > /etc/modules-load.d/hid-risemode-aio.conf

# Replace the transient test service, if present, with the installed unit.
systemctl stop rise-aio-monitor.service 2>/dev/null || true
systemctl daemon-reload
modprobe hid-risemode-aio
systemctl enable --now rise-aio-monitor.service

echo "Instalado. Verifique com: systemctl status rise-aio-monitor.service"
