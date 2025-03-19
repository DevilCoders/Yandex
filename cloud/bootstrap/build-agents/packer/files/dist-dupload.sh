#!/bin/sh
# Usage $0 REPO .../*.changes

# Fix files' perms so dupload don't try to chmod it on server side
for f in "$@"; do
        dcmd chmod 644 "$f"
done

tries=3
while ! dupload --to "$@"; do
        tries=$(( $tries - 1))
        if [ $tries -le 0 ]; then
                exit 1
        fi
        sleep $tries
done
exit 0
