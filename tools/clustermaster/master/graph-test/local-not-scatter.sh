#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = worker0..worker2 worker0..worker2
    SECOND = worker0..worker2

    FIRST first
    SECOND second
}
EOF

# vim: set ts=4 sw=4 et:
