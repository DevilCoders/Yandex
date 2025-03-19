#!/bin/bash
echo "===== Instance address" > /dev/ttyS0
ip addr show dev eth0 | sed -e 's/.*\(inet6 [^ ]*\).*/\1/;t;d' > /dev/ttyS0
