#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = localhost,otherhost aaa,bbb res=
    SECOND = localhost,otherhost aaa,bbb res=

    FIRST first
    SECOND second_cn: ^ []
}
EOF

# vim: set ts=4 sw=4 et:
