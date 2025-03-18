#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = hostaaa,hostbbb
    SECOND = hostaaa

    FIRST first
    SECOND second
}
EOF

first() {
    echo "first"
}

second() {
    echo "second"
}

# vim: set ts=4 sw=4 et:
