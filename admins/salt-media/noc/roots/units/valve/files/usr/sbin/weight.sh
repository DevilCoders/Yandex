cat /proc/cpuinfo | grep cores | head -n 1 | grep -o "[0-9]*" | awk '{print $1*5}'
