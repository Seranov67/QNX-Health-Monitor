# QNX Health Monitor

Compare process and memory snapshots from a QNX 8.0 guest with a Linux CLI. This is useful when you want to see what appeared or changed after starting an application in a test VM.

The CLI starts a QEMU process, opens the guest's serial console, and runs `uname -sr`, `pidin info`, `pidin ar`, and `pidin mem` in QNX. It saves two JSON snapshots and a human-readable diff. Process entries include the executable token and observed data memory. Command-line arguments are omitted from the snapshots.

## Try it without a VM

Use Python 3.10 or later and install `pexpect` (on Ubuntu, `sudo apt install python3-pexpect`). Replay the checked-in measurements from a real QNX guest:

```sh
./qnx-health-monitor diff qemu-results/before.json qemu-results/after.json
```

## Reproduce the QEMU demo

The demo was tested with QNX SDP 8.0, the official x86-64 QNX 8.0.0 Quick Start Target Image, QEMU 10.2.1 with TCG, and the QNX `qcc` x86-64 toolchain.

1. Use QNX Software Center to install QNX SDP 8.0 and `com.qnx.qnx800.quickstart.qemu`. Follow the [QNX Quick Start guide](https://www.qnx.com/developers/docs/qnxeverywhere/com.qnx.doc.target_images/topic/qsti_qemu/getting_started.html) to unpack the image. The QNX SDK and guest image are distributed separately by QNX.
2. Set `QNX_SDP` to your SDK directory. Set `QNX_IFS` and `QNX_DISK` to the unpacked image's `output/ifs.bin` and `output/disk-qemu`, respectively. Use absolute paths and a volume with space for the expanded guest image.
3. From the repository directory, cross-compile the 64 MiB test workload and create its small QNX filesystem image:

```sh
source "$QNX_SDP/qnxsdp-env.sh"
qcc -Vgcc_ntox86_64 -O2 -Wall -Wextra -Werror qnx-health-load.c -o qnx-health-load
printf '[num_sectors=*8192]\n[perms=755 uid=0 gid=0] qnx-health-load=%s/qnx-health-load\n' "$PWD" > workload.build
mkqnx6fsimg workload.build workload.qnx6
```

4. Set the guest serial login if required, then run the demo. Use the credentials for your test guest and choose a fresh output directory for each run.

```bash
read -rsp "QNX serial password: " QHM_GUEST_PASSWORD; printf '\n'
export QHM_GUEST_PASSWORD

./qnx-health-monitor run --user root --output qemu-demo \
  --prepare 'mkdir -p /data/w && mount -t qnx6 /dev/hd1 /data/w' \
  --workload '/data/w/qnx-health-load >/data/qhm-load.log 2>&1 & sleep 2' -- \
  qemu-system-x86_64 -accel tcg -cpu max -smp 4 -m 4096 -nic none \
  -display none -vga none -device virtio-vga \
  -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
  -serial stdio -monitor none -snapshot -kernel "$QNX_IFS" \
  -drive "file=$QNX_DISK,if=ide,format=raw" \
  -drive "file=$PWD/workload.qnx6,if=ide,format=raw,index=1"

unset QHM_GUEST_PASSWORD
```

The CLI closes the VM it starts after collecting the snapshots. The demo uses an isolated guest and QEMU snapshot mode. Pass only trusted guest shell commands to `--prepare` and `--workload`. If the guest already provides a shell on the serial console, the login options are optional.

## Reading the report

- `before.json` and `after.json` contain the system counters and per-process measurements. `diff.txt` shows new and removed processes and data-memory changes.
- System free memory comes from `pidin info`. Process memory comes from the `data` column of `pidin mem`, without summing duplicate thread rows. The two measures can differ because the system counter includes other activity.
- Process identity is matched using PID and the executable token. PID reuse by the same executable between snapshots cannot be distinguished. Brief processes may appear or disappear between the individual `pidin` commands.

See [LICENSE](LICENSE) for the MIT license covering the project code and documentation.
