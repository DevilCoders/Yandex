#! /bin/bash
export HOME=/root
USAGE="Usage: `basename $0` [--dry-run --from-hosts=fqdn1,fqdn2 --skip-starting-check]"
ATTEMPTS={{ salt.mdb_redis.ensure_no_primary_attempts() }}
CMD="/usr/local/yandex/ensure_not_master.py --wait"
REDIS_STARTING_FLAG="{{ salt.mdb_redis.get_redis_starting_flag() }}"
REDIS_STOPPING_FLAG="{{ salt.mdb_redis.get_redis_stopping_flag() }}"
REDIS_PID="{{ salt.mdb_redis.get_redis_pidfile() }}"
CHECK_RESTART=true
while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run)
            $CMD --dry-run
            if [ $? -ne 0 ]; then
                echo "YES"
            else
                echo "NO"
            fi
            exit 0;;
        --skip-restart-check)
            CHECK_RESTART=false
            shift 1;;
        --from-hosts=*)
            shift 1;;
        -*)
            echo $USAGE; exit 1;;
        *)
            break
    esac
done

if [ ! -f "$REDIS_PID" ]; then
    echo "redis-server is stopped now, skipping"
    exit 0
fi

if [ "$CHECK_RESTART" == true ]; then
    if [ -f "$REDIS_STARTING_FLAG" ]; then
        echo "redis-server is starting now, skipping"
        exit 0
    fi
    if [ -f "$REDIS_STOPPING_FLAG" ]; then
        echo "redis-server is stopping now, skipping"
        exit 0
    fi
fi

$CMD --dry-run && exit 0
$CMD --attempts $ATTEMPTS || exit 1
