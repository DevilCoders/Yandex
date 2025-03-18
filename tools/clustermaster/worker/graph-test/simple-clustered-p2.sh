#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = localhost,otherhost aaa,bbb 001..003 res=
    SECOND = localhost,otherhost aaa,bbb 001..003 res=

    FIRST first
    SECOND second
}
EOF

# vim: set ts=4 sw=4 et:
