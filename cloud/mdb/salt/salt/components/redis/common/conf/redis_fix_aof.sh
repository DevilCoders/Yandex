#! /bin/bash

REDIS_VERSION_MAJOR=$(redis-server --version | awk '{print $3}' | awk -F= '{print $2}' | awk -F. '{print $1}')
if [ $REDIS_VERSION_MAJOR -lt 7 ]; then
    test -f /var/lib/redis/appendonly.aof && tail -n 10 /var/log/redis/redis-server.log | fgrep "Bad file format reading the append only file"
    if [ $? -ne 0 ]; then
        echo "aof is ok, nothing done" && exit 0
    fi

    echo 'y' | redis-check-aof --fix /var/lib/redis/appendonly.aof | fgrep -q "Successfully truncated AOF"
    if [ $? -ne 0 ]; then
        echo "aof fix failed" && exit 1
    fi
else
    test -d /var/lib/redis/appendonlydir && tail -n 100 /var/log/redis/redis-server.log | fgrep "Bad file format reading the append only file"
    if [ $? -ne 0 ]; then
        echo "aof is ok, nothing done" && exit 0
    fi

    test -f /var/lib/redis/appendonlydir/appendonly.aof
    if [ $? -eq 0 ]; then
        echo 'y' | redis-check-aof --fix /var/lib/redis/appendonlydir/appendonly.aof | grep -q "Successfully truncated AOF\|AOF .* is valid"
        if [ $? -ne 0 ]; then
            echo "aof fix failed with appendonly.aof" && exit 1
        fi
    fi
    for file in /var/lib/redis/appendonlydir/*aof*base*
    do
        echo 'y' | redis-check-aof --fix "$file" | grep -q "Successfully truncated AOF\|AOF .* is valid"
        if [ $? -ne 0 ]; then
            echo "aof fix failed with $file" && exit 1
        fi
    done
    for file in /var/lib/redis/appendonlydir/*aof*incr*
    do
        echo 'y' | redis-check-aof --fix "$file" | grep -q "Successfully truncated AOF\|AOF .* is valid"
        if [ $? -ne 0 ]; then
            echo "aof fix failed with $file" && exit 1
        fi
    done
fi
echo "aof fixed" && exit 0
