# Test Exploit for CVE-2019-8956

## Prerequisites:
### Applicable Kernel versions:
* 4.20.x before 4.20.8
* 4.19.x before 4.19.21
* 4.18.x
* 4.17.x

### Required packages (Ubuntu):
* `libsctp1`
* `libsctp-dev`

### Other requirements:
* `sudo modprobe sctp`
* (for using `./checklogs.sh`) Read access to `/var/log/syslog`, via `dmesg` or similar

## Compiling and testing:
Compile with `./build.sh`. Run with `./sctp_uaf <PORT>` and `./sctp_uaf_spam <PORT>`.  The latter attempts to allocate memory in the space where `asoc` used to reside.

You can check if `./sctp_uaf_spam` succeeds by using `./checklogs.sh`, which follows the syslogs for any abnormalities in syslog output.  Note that you need to enter a new port number with every execution.  For example: `for i in 3000..30000; do ./sctp_uaf_spam $i; done`
