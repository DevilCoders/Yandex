#! /bin/bash
export HOME=/root
USAGE="Usage: `basename $0` [--dry-run] [--attempts] [--from-hosts=fqdn1,fqdn2]"
ATTEMPTS=10
while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run)
            /usr/local/yandex/ensure_not_leader.py --dry-run
            if [ $? -ne 0 ]; then
                echo "YES"
            else
                echo "NO"
            fi
            exit 0;;
        --attempts)
            ATTEMPTS=$2
            shift
            ;;
        --from-hosts=*)
            ;;
        -*)
            echo $USAGE; exit 1;;
        *)
            break
    esac
    shift
done

/usr/local/yandex/ensure_not_leader.py --attempts $ATTEMPTS || exit 1
