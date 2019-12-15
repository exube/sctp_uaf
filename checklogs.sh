#!/bin/sh
tail -f /var/log/syslog | grep -E 'RSP: [0-9a-fA-F]{4}:[fF]{2}' -A1 | grep 'RBX:' | grep -v dead
