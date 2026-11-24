# QNX Health Monitor

Compare process and memory snapshots from a QNX 8.0 guest with a Linux CLI. The CLI starts QEMU, reads the guest's serial console, and runs `uname -sr`, `pidin info`, `pidin ar`, and `pidin mem`. It saves two JSON snapshots locally and reports changes in process data memory and system free memory.

See the [measured diff](qemu-results/diff.txt) and the [QEMU demo](https://youtu.be/40fRHvP-kcQ).

## Reproduce the QEMU demo

Install Python 3.10 or later, `pexpect` (on Ubuntu, `sudo apt install python3-pexpect`), and QEMU. The demo was tested with QNX SDP 8.0, QEMU 10.2.1 with TCG, and the official x86-64 QNX 8.0.0 Quick Start Target Image.

1. Use QNX Software Center to install QNX SDP 8.0 and `com.qnx.qnx800.quickstart.qemu`. Follow the [QNX Quick Start guide](https://www.qnx.com/developers/docs/qnxeverywhere/com.qnx.doc.target_images/topic/qsti_qemu/getting_started.html) to unpack the image. The SDK and guest image are distributed separately by QNX.
2. Set absolute paths: `QNX_SDP` to your installed SDK, and `QNX_IFS` and `QNX_DISK` to the unpacked image's `output/ifs.bin` and `output/disk-qemu`. Use a volume with space for the expanded guest image.
3. Cross-compile the workload and make its small QNX filesystem image from this directory.

~~~sh
source "$QNX_SDP/qnxsdp-env.sh"
qcc -Vgcc_ntox86_64 -O2 -Wall -Wextra -Werror qnx-health-load.c -o qnx-health-load
printf '[num_sectors=*8192]\n[perms=755 uid=0 gid=0] qnx-health-load=%s/qnx-health-load\n' "$PWD" > workload.build
mkqnx6fsimg workload.build workload.qnx6
~~~

4. Run the demo in Bash with a fresh output directory. Use the credentials of your test guest if it prompts for a serial login.

~~~bash
read -rsp "QNX serial password: " QHM_GUEST_PASSWORD; printf '\n'
export QHM_GUEST_PASSWORD

python3 ./qnx-health-monitor run --user root --output qemu-demo \
  --prepare 'mkdir -p /data/w && mount -t qnx6 /dev/hd1 /data/w' \
  --workload '/data/w/qnx-health-load >/data/qhm-load.log 2>&1 & sleep 2' -- \
  qemu-system-x86_64 -accel tcg -cpu max -smp 4 -m 4096 -nic none \
  -display none -vga none -device virtio-vga \
  -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 \
  -serial stdio -monitor none -snapshot -kernel "$QNX_IFS" \
  -drive "file=$QNX_DISK,if=ide,format=raw" \
  -drive "file=$PWD/workload.qnx6,if=ide,format=raw,index=1"

unset QHM_GUEST_PASSWORD
python3 ./qnx-health-monitor diff qemu-demo/before.json qemu-demo/after.json
~~~

The CLI closes its VM after collecting the snapshots. The demo uses an isolated guest and QEMU snapshot mode. Use trusted guest shell commands for `--prepare` and `--workload`. Login options are optional when the serial console already provides a shell.

## Reading the report

- `before.json` and `after.json` contain system counters and per-process measurements. `diff.txt` lists new and removed processes and changes in per-process data memory.
- Free memory is the system counter from `pidin info`. Process data memory comes from `pidin mem`, with duplicate thread rows counted once.
- Process identity uses the PID and executable token. Reuse of the same PID by the same executable cannot be distinguished. Brief processes can change between the individual reads.

See [LICENSE](LICENSE) for the MIT license.
