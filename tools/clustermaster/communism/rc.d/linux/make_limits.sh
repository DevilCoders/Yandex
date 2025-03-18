#!/usr/bin/env bash

set -e -u -o pipefail

: ${DISK_ROOT:="/place"}

platform=$(uname)

case "$platform" in
    Linux)
        echo -n "cpu="
        grep -c processor /proc/cpuinfo
        echo -n "mem="
        free -m | awk '$1=="Mem:"{print $2}'
        echo -n "disk="
        df --block-size=1073741824 $DISK_ROOT | awk 'END{print $2}'
        echo "net=1"
        ;;
    FreeBSD)
        echo -n "cpu="
        /sbin/sysctl hw.ncpu | awk '{print $NF}'
        echo -n "mem="
        /sbin/sysctl hw.physmem | awk '{print int($NF/1048576)}'
        echo -n "disk="
        df -g $DISK_ROOT | awk 'END{print $2}'
        echo "net=1"
        ;;
    *)
        echo "Unknown platform: $platform" >&2
        exit 1
        ;;
esac

