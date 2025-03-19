#!/bin/sh

for key in $(gpg -k | grep ^pub -A1 | tail -n1 | awk '{print $1}' 2>/dev/null); do
    printf "trust\n5\ny\nquit" | gpg --no-tty --command-fd 0 --edit-key ${key};
done
