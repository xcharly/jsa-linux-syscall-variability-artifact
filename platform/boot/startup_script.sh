#!/bin/bash

sleep 10

mount -t tracefs tracefs /sys/kernel/tracing
mount 10.0.0.1:/srv/nano2 /nfsclient
mount 10.0.0.1:/srv/runtime-monitor /home/nano1/runtime-monitor

echo 1500000 > /sys/kernel/tracing/per_cpu/cpu1/buffer_size_kb

cd /nfsclient/src
#python3 test_client.py
chrt -f 98 python3 test_client.py
