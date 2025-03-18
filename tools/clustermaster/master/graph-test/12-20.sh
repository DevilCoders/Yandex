#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST  = hosta,hostb xx,yy worker1..worker3
    SECOND = worker1..worker3 00..01 xx,yy

    FIRST  first
    SECOND second: first [1->2,2->0]
}
EOF

# vim: set ts=4 sw=4 et:
