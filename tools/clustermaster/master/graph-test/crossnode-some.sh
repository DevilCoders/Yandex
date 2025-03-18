#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    HOSTS = hosta,hostb,hostc

    HOSTS first:
    HOSTS second:
    # crossnode with second, local with first, all controlled on master
    HOSTS third: first second []
}
EOF

# vim: set ts=4 sw=4 et:
