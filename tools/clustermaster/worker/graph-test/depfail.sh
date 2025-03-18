#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    T1 = localhost 0..1 0..1
    T2 = localhost 0..1 0..1 0..1
    T3 = other 0..1

    T1 a:
    T1 b: a
    T1 c: a [0->0,1->2,2->1]
    T1 d: b c
    T2 e: d [0->0,1->1,2->2]
    T2 f: a [0->0,1->2,2->1]
    T3 g: c []
    T1 i: g
    T2 h: d [0->0,1->1]

    T1 t1: a ?FLAGS~turned_off_conditional_depend
    T1 t2: t1
}
EOF

# vim: set ts=4 sw=4 et:
