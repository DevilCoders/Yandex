#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = localhost res=
    SECOND = localhost res=

    FIRST first
    SECOND second
}
EOF

# vim: set ts=4 sw=4 et:
