# this docker produces a reliable and reproducible valkey memory layout across different host machines through QEMU TCG
# only STAGE0 is relevant for the challenge itself, the next are for local-remote reproducibility
# run with: docker run --rm -p6379:6379 $(docker build -q .)

# STAGE0: pull and build valkey
FROM debian:13.7-slim AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential pkg-config git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /valkey
RUN git clone https://github.com/valkey-io/valkey /valkey
RUN git checkout 7be7d564ff0b27b20df8debab78822e4a4d57a62^

RUN make OPTIMIZATION="-O0" CFLAGS="-g" valkey-server

# STAGE1: prepare a guest image
FROM debian:13.7-slim AS guest

# INITRD=No: keep the kernel package from generating its own initramfs.
ENV DEBIAN_FRONTEND=noninteractive \
    INITRD=No

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		linux-image-cloud-amd64 busybox-static kmod cpio gzip xz-utils zstd \
	&& rm -rf /var/lib/apt/lists/*

COPY --from=build /valkey/src/valkey-server /rootfs/bin/valkey-server
COPY ./flag.txt /

COPY <<"EOF" /rootfs/etc/passwd
root:x:0:0:root:/root:/bin/sh
ctf:x:999:999:ctf:/home/valkey:/bin/sh
EOF

COPY <<"EOF" /rootfs/etc/group
root:x:0:
ctf:x:999:
EOF

COPY --chmod=0755 <<"EOF" /rootfs/init
#!/bin/sh
export PATH=/bin

mount -t devtmpfs devtmpfs /dev
exec </dev/console >/dev/console 2>&1
mount -t proc  proc  /proc
mount -t sysfs sysfs /sys

# virtio modules
for ko in /lib/modules/*.ko; do
  [ -e "$ko" ] && insmod "$ko"
done

until [ -d /sys/class/net/eth0 ]; do sleep 1;  done
ip link set lo up
ip addr add 10.0.2.15/24 dev eth0
ip link set eth0 up
ip route add default via 10.0.2.2

ulimit -n 65536

# perms
chown 0:999 /bin/valkey-server /flag.txt
chmod 0550  /bin/valkey-server
chmod 0040  /flag.txt

exec su -s /bin/sh ctf -c 'valkey-server --protected-mode no --save ""'
poweroff -f
EOF

RUN <<GUEST
set -eux
R=/rootfs
mkdir -p /out $R/bin $R/dev $R/proc $R/sys $R/tmp $R/lib/modules

# --- busybox: init shell + the few applets init uses
cp /bin/busybox $R/bin/busybox
for a in sh mount insmod ip env sleep poweroff; do
  /bin/busybox --list | grep -qx "$a"
  ln -s busybox $R/bin/$a
done

# --- shared libs of valkey-server (+ dynamic loader)
ldd $R/bin/valkey-server 2>/dev/null | grep -o '/[^ ]*' | while read -r lib; do
  cp -L --parents "$lib" $R
done

# --- flag
cp /flag.txt $R

# --- kernel
set -- /boot/vmlinuz-*;  [ $# -eq 1 ]; cp "$1" /out/vmlinuz; chmod 0644 /out/vmlinuz
set -- /lib/modules/*;   [ $# -eq 1 ]; KVER=${1##*/}
for c in BLK_DEV_INITRD RD_GZIP DEVTMPFS SERIAL_8250_CONSOLE; do
  grep -qx "CONFIG_$c=y" /boot/config-$KVER
done
grep -Eqx 'CONFIG_VIRTIO_NET=[ym]' /boot/config-$KVER
depmod -a "$KVER"

# --- virtio PCI + net modules
i=0
for m in virtio_pci virtio_net; do modprobe -S "$KVER" --show-depends "$m"; done \
 | awk '$1=="insmod" && !seen[$2]++ {print $2}' \
 | while read -r ko; do
     i=$((i+1))
     out=$(printf '%s/lib/modules/%02d-%s.ko' "$R" "$i" "$(basename "${ko%%.ko*}")")
     case "$ko" in
       *.xz)  xz   -dc "$ko" ;;
       *.zst) zstd -dc "$ko" ;;
       *.gz)  gzip -dc "$ko" ;;
       *)     cat "$ko" ;;
     esac > "$out"
   done

# --- pack
cd $R
find . -mindepth 1 | LC_ALL=C sort | cpio -o -H newc -R 0:0 --quiet | gzip -9n > /out/initramfs.gz
chmod 0644 /out/initramfs.gz
GUEST

# STAGE2: run the actual QEMU vm
FROM debian:13.7-slim

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
	&& apt-get install -y --no-install-recommends qemu-system-x86 \
	&& rm -rf /var/lib/apt/lists/*

COPY --from=guest /out/vmlinuz /out/initramfs.gz /guest/

EXPOSE 6379
USER 65534:65534

ENTRYPOINT ["qemu-system-x86_64", \
	"-nodefaults", "-no-reboot", \
	"-display", "none", "-monitor", "none", "-serial", "stdio", \
	"-machine", "pc-q35-10.0", \
	"-accel", "tcg,thread=single", \
	"-cpu", "qemu64", \
	"-smp", "1", \
	"-m", "1024", \
	"-kernel", "/guest/vmlinuz", \
	"-initrd", "/guest/initramfs.gz", \
	"-append", "console=ttyS0 transparent_hugepage=never panic=1 quiet", \
	"-netdev", "user,id=n0,net=10.0.2.0/24,host=10.0.2.2,hostfwd=tcp::6379-:6379", \
	"-device", "virtio-net-pci,netdev=n0,romfile="]