#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    QUX = hosta,hostb 11,22,33 aa,bb

    QUX first
    QUX second: first []
}
EOF

# vim: set ts=4 sw=4 et:
