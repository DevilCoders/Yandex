#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    FIRST  = localhost,hosta,hostb xx,yy worker1..worker3 00..01
    SECOND = localhost,hosta,hostb 00..01 xx,yy aa,bb

    FIRST  first
    SECOND second: first [0->0,1->2,3->1]
}
EOF

# vim: set ts=4 sw=4 et:
