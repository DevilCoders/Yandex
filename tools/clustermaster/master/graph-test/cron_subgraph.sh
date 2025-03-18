#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    W1clusters = w1 0,1
    W2clusters = w2 0,1
    GLOBALseghosts = ya w7,w8,w9
    W789clusters = w7,w8,w9 0..3
    W789w789 = w7,w8,w9 w7,w8,w9
    
    GLOBALseghosts u0:
    W789clusters u1: u0
    W789w789  u: u1 [0->1]
    W789clusters  v: u [1->0] restart_on_success="* * * * *"

    W2clusters  s:
    W1clusters  t: s restart_on_success="* * * * *"
}
EOF

# vim: set ts=4 sw=4 et:
