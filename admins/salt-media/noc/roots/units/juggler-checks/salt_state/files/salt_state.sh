#!/bin/bash

me=${0##*/} # strip path
me=${me%.*} # strip extension
lock="/tmp/${me}.lock"
unable_get_lock='Unable to get lock'

while getopts "s:t:w:c:a:" OPTION; do
    case $OPTION in
    s)
        SLEEP="$OPTARG"
        ;;
    t)
        TIMEOUT="$OPTARG"
        ;;
    w)
        WARN="$OPTARG"
        ;;
    c)
        CRIT="$OPTARG"
        ;;
    a)
        ARGS="$OPTARG"
        ;;
    esac
done

SLEEP=${SLEEP:-300}
TIMEOUT=${TIMEOUT:-600}
WARN=${WARN:-4}
CRIT=${CRIT:-10}
SALT_ARGS=${ARGS:-'test=True'}

if [ $SLEEP -ne 0 ]; then
    sleep $((RANDOM % ${SLEEP} + 1))
fi

die() {
    die_internal "$@" | tee /dev/shm/monrun-salt-state.result.tmp 2>/dev/null
    mv -f /dev/shm/monrun-salt-state.result.tmp /dev/shm/monrun-salt-state.result 2>/dev/null
    # exit вызванный в pipe-е (например внутри die_internal) уничтожает процесс pipe-а
    # и не завершает выполнение основного shell-а
    # чтобы завершить текущий процесс, нужно избегать subshell-ов
    exit 0
}

die_internal() {
    case "$1" in
    LOCK)
        echo "PASSIVE-CHECK:salt_state;WARN;$2"
        ;;
    OK)
        echo "PASSIVE-CHECK:salt_state;OK;$2"
        >/dev/shm/monrun-salt-state
        ;;
    WARN)
        echo "diff" >>/dev/shm/monrun-salt-state
        STATE=$(cat /dev/shm/monrun-salt-state | wc -l)
        if [ $STATE -gt $CRIT ]; then
            echo "PASSIVE-CHECK:salt_state;CRIT;$2"
        else
            if [ $STATE -gt $WARN ]; then
                echo "PASSIVE-CHECK:salt_state;WARN;$2"
            else
                echo "PASSIVE-CHECK:salt_state;OK;$2"
            fi
        fi
        ;;
    CRIT)
        echo "PASSIVE-CHECK:salt_state;CRIT;$2"
        ;;
    esac
    if ! [[ "$2" =~ $unable_get_lock ]]; then
        rm -f "$lock"
    fi
}

exec 200>"$lock" 2>&1
flock -n 200 &>/dev/null || die LOCK "$unable_get_lock"
echo $$ >&200

until DIFFS="$(sudo /usr/bin/timeout $TIMEOUT \
    /usr/bin/salt-call state.highstate \
    $SALT_ARGS -l quiet --output=yaml 2>&1)"; do
    if test -n "$DIFFS"; then
        break # NOCDEV-6718 считаем, что диф получен и он нормальный!!!
    fi
    if ((_r += 1, _r > ${MAX_RETRIES:=3})); then
        die CRIT "Salt call failed (#${MAX_RETRIES} times)"
    fi
done

RESULTS="$(grep -E '^\s+result:' <<<"$DIFFS")"

NUM_DIFFS="$(grep -c -e false -e null <<<"$RESULTS" || :)"
NUM_STATES="$(wc -l <<<"$RESULTS")"

if ((NUM_STATES == 0)); then
    if [ -n "$(find /var/cache/salt/minion/proc/ -type f -cmin +60)" ]; then
        die CRIT "stale tasks found"
    fi
fi

if ((NUM_DIFFS == 0)); then
    die OK "OK"
else
    die WARN "Highstate is outdated, $NUM_DIFFS diffs"
fi
