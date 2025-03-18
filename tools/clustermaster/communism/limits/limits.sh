#! /bin/sh

set -ue

: ${DISK_ROOT:="/place"}

platf=$(uname)

case "$platf" in
    Linux)
        file=$(mktemp -t communism.XXXXXXXX)

        cpu="$(grep processor /proc/cpuinfo | wc -l)"

        printf 'cpu=%s\n' ${cpu:-0} >> $file
        printf 'mem=%s\n' $(free -m | awk 'NR==2 {print $2}') >> $file
        disk=$(df -P $DISK_ROOT 2>/dev/null | awk 'NR==2 {print int($4/1048576)}')
        printf 'disk=%s\n' ${disk:-"0"} >> $file
        printf 'net=1\n' >> $file

        echo -n $file
        ;;

    FreeBSD)
        file=$(mktemp -t communism)

        printf 'cpu=%s\n' $(/sbin/sysctl hw.ncpu | egrep -o '\w+$') >> $file
        printf 'mem=%s\n' $(($(/sbin/sysctl hw.physmem | egrep -o '\w+$') / 1048576)) >> $file
        disk=$(df -P $DISK_ROOT 2>/dev/null | awk 'NR==2 {print int($4/1048576)}')
        printf 'disk=%s\n' ${disk:-"0"} >> $file
        printf 'net=1\n' >> $file

        echo -n $file
        ;;

    *)
        printf 'Unknown platform: %s\n' "$platf" >&2
        exit 1
        ;;

esac
