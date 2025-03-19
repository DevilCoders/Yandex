#! /bin/bash

USAGE="Usage: $(basename "$0") [--dry-run --from-hosts=fqdn1,fqdn2]"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run)
            if mdb-mongo-get is_primary; then
                echo "YES"
            else
                echo "NO"
            fi
            exit 0;;
        --from-hosts=*)
            break;;
        -*)
            echo $USAGE; exit 1;;
        *)
            break
    esac
done


if mdb-mongo-get is_ha; then
    mdb-mongod-stepdown
    exit $?
fi

mdb-mongod-stepdown --force
