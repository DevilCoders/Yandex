#!/bin/sh

for key in $(gpg -k | grep ^pub | cut -d'/' -f2 | awk '{print $1};' 2>/dev/null); do
    printf "trust\n5\ny\nquit" | gpg --debug --no-tty --command-fd 0 --edit-key ${key};
done
