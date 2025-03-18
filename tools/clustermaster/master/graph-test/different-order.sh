#!/bin/sh -e

cat >> /dev/null << EOF
_scenario() {
    FIRST  = hosta,hostb aa,bb
    SECOND = hostb,hosta bb,aa

    FIRST first
    SECOND second: ^ [0->0,1->1]
}
EOF

# vim: set ts=4 sw=4 et:
