#!/bin/bash

for f in /var/lib/redis/temp*; do
    [ -f "$f" ] || break
    fuser -s "$f" || (rm "$f" && echo "removed $f")
done
