#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = host_abc res=
    SECOND = host_efg res=

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
