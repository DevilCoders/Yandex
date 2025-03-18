#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = hostaaa
    SECOND = hostaaa
    THIRD = hostccc

    FIRST first:
    SECOND second:
    THIRD third: first second
}
EOF

# vim: set ts=4 sw=4 et:
