#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    T1 = localhost 0..1 0..1

    T1 a0:

    T1 a:

    T1 b: a
    T1 c: a

    T1 d: b
    T1 e: b c ?FLAGS~turned_off_conditional_depend
    T1 f: c a0

    T1 g: d e
    T1 h: e f ?FLAGS~turned_off_conditional_depend

    T1 i: g h [0->0,1->2,2->1] a0
}
EOF

# vim: set ts=4 sw=4 et:
