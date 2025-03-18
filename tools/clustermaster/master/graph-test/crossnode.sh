#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    HOSTS = hosta,hostb,hostc

    HOSTS first
    HOSTS second: first []
}
EOF

# vim: set ts=4 sw=4 et:
