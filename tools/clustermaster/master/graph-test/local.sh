#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST = hostaaa res=
    SECOND = hostaaa res=

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
