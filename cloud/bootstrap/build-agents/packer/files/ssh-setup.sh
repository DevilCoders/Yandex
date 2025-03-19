#!/bin/sh

DIR="${0%/*}"
mkdir -p "$HOME/.ssh"
chmod 700 "$HOME/.ssh"
echo "" >> ./.ssh/config
sed -i -e "$ a ForwardAgent yes\nHashKnownHosts no\nStrictHostKeyChecking no" \
    -e "/^\(ForwardAgent\|HashKnownHosts\|StrictHostKeyChecking\)/ d;" "${HOME}"/.ssh/config
